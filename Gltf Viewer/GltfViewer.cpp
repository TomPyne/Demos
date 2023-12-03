// Render Example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Render/Render.h"
#include "Utils/Camera/FlyCamera.h"
#include "Utils/GltfLoader.h"
#include "Utils/HighResolutionClock.h"
#include "Utils/KeyCodes.h"
#include "Utils/Logging.h"
#include "Utils/SurfMath.h"
#include "Utils/TextureLoader.h"

#include "ThirdParty/imgui/imgui.h"
#include "ImGui/imgui_impl_render.h"
#include "ImGui/imgui_impl_win32.h"

#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Render data
///////////////////////////////////////////////////////////////////////////////
struct
{
	u32 w = 0;
	u32 h = 0;
	FlyCamera cam;
	Texture_t DepthTex = Texture_t::INVALID;
} screenData;

struct
{
	float2 sunPitchYaw = float2{70.0f, 0.0f};
	float3 radiance = float3{5.0f};
	float3 ambient = float3{0.02f, 0.02f, 0.04f};
} lightData;

static void ResizeTargets(u32 w, u32 h)
{
	w = Max(w, 1u);
	h = Max(h, 1u);

	if (w == screenData.w && h == screenData.h)
		return;

	screenData.w = w;
	screenData.h = h;

	screenData.cam.Resize(w, h);

	Render_Release(screenData.DepthTex);

	TextureCreateDesc desc = {};
	desc.width = w;
	desc.height = h;
	desc.format = RenderFormat::D32_FLOAT;
	desc.flags = RenderResourceFlags::DSV;
	screenData.DepthTex = CreateTexture(desc);
}

union MaterialID
{
	struct
	{
		u8 doubleSided : 1;
		u8 blendMode : 1;
		u8 unused : 6;
	};
	u8 opaque = 0;
};

GraphicsPipelineState_t pipelines[1u << (1u + 2u)];

void InitPipelines()
{
	const char* shaderPath = "Gltf Viewer/Mesh.hlsl";

	VertexShader_t vs = CreateVertexShader(shaderPath);
	PixelShader_t blendPs = CreatePixelShader(shaderPath);
	PixelShader_t maskPs = CreatePixelShader(shaderPath, {"ALPHA_MASK"});

	InputElementDesc inputDesc[] =
	{
		{"POSITION", 0, RenderFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PerVertex, 0 },
		{"NORMAL", 0, RenderFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PerVertex, 0 },
		{"TANGENT", 0, RenderFormat::R32G32B32A32_FLOAT, 2, 0, InputClassification::PerVertex, 0 },
		{"TEXCOORD", 0, RenderFormat::R32G32_FLOAT, 3, 0, InputClassification::PerVertex, 0 },
	};

	GraphicsPipelineStateDesc desc = {};
	desc.DepthDesc(true, ComparisionFunc::LessEqual);
	desc.numRenderTargets = 1;

	for (u8 doubleSided = 0; doubleSided <= 1; doubleSided++)
	{
		MaterialID curId;
		curId.doubleSided = doubleSided;

		desc.RasterizerDesc(PrimitiveTopologyType::Triangle, FillMode::Solid, doubleSided == 1 ? CullMode::None : CullMode::Back);

		desc.vs = vs;
		desc.ps = blendPs;

		curId.blendMode = 0; // opaque
		desc.blendMode[0].None();

		pipelines[curId.opaque] = CreateGraphicsPipelineState(desc, inputDesc, ARRAYSIZE(inputDesc));

		curId.blendMode = 1; // blend
		desc.blendMode[0].Default();

		pipelines[curId.opaque] = CreateGraphicsPipelineState(desc, inputDesc, ARRAYSIZE(inputDesc));
	}
}

///////////////////////////////////////////////////////////////////////////////
// Assets
///////////////////////////////////////////////////////////////////////////////

struct MaterialInstance
{
	MaterialID pipeline;

	float4 baseColorFactor = float4{1.0f};
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;

	Texture_t baseColorTexture = Texture_t::INVALID;
	Texture_t normalTexture = Texture_t::INVALID;
	Texture_t metallicRoughnessTexture = Texture_t::INVALID;

	bool alphaMask = false;
	float alphaCutoff = 0.5f;
};

struct BindVertexBuffer
{
	VertexBuffer_t buf = VertexBuffer_t::INVALID;
	uint32_t stride = 0;
	uint32_t offset = 0;
};

struct BindIndexBuffer
{
	IndexBuffer_t buf = IndexBuffer_t::INVALID;
	RenderFormat format;
	uint32_t offset = 0;
	uint32_t count = 0;
};

struct Mesh
{
	BindVertexBuffer positionBuf;
	BindVertexBuffer normalBuf;
	BindVertexBuffer tangentBuf;
	BindVertexBuffer texcoordBufs[4];
	BindVertexBuffer colorBufs[4];
	BindVertexBuffer jointBufs[4];
	BindVertexBuffer weightBufs[4];
	BindIndexBuffer indexBuf;

	MaterialInstance material;
};

struct Model
{
	matrix transform;
	std::vector<uint32_t> meshes;
};

std::vector<Texture_t> loadedTextures;
std::vector<Mesh> loadedMeshes;
std::vector<Model> loadedModels;

///////////////////////////////////////////////////////////////////////////////
// Asset Processing
///////////////////////////////////////////////////////////////////////////////

struct GltfProcessor
{
	const Gltf& _gltf;

	std::vector<Texture_t> textures;

	explicit GltfProcessor(const Gltf& gltf) : _gltf(gltf) {}

	uint32_t ProcessMesh(const GltfMeshPrimitive& prim);
	uint32_t ProcessNode(int32_t nodeIdx, uint32_t parentIdx);
	Texture_t ProcessTexture(const GltfTextureInfo& tex);
	Texture_t ProcessNormalTexture(const GltfNormalTextureInfo& tex);
	void ProcessScenes();
};

Texture_t GltfProcessor::ProcessTexture(const GltfTextureInfo& texInfo)
{
	if (texInfo.index < textures.size() && textures[texInfo.index] != Texture_t::INVALID)
		return textures[texInfo.index];

	textures.resize(texInfo.index + 1, Texture_t::INVALID);

	const GltfTexture& tex = _gltf.textures[texInfo.index];
	const GltfImage& img = _gltf.images[tex.source];
	const GltfBufferView& bufView = _gltf.bufferViews[img.bufferView];

	if (img.mimeType == "image/png")
	{
		uint32_t w, h;
		textures[texInfo.index] = TextureLoader_LoadPngTextureFromMemory((_gltf.data.get() + bufView.byteOffset), bufView.byteLength, &w, &h);
	}

	return textures[texInfo.index];
}

Texture_t GltfProcessor::ProcessNormalTexture(const GltfNormalTextureInfo& texInfo)
{
	if (texInfo.index < textures.size() && textures[texInfo.index] != Texture_t::INVALID)
		return textures[texInfo.index];

	textures.resize(texInfo.index + 1, Texture_t::INVALID);

	const GltfTexture& tex = _gltf.textures[texInfo.index];
	const GltfImage& img = _gltf.images[tex.source];
	const GltfBufferView& bufView = _gltf.bufferViews[img.bufferView];

	if (img.mimeType == "image/png")
	{
		uint32_t w, h;
		textures[texInfo.index] = TextureLoader_LoadPngTextureFromMemory((_gltf.data.get() + bufView.byteOffset), bufView.byteLength, &w, &h);
	}

	return textures[texInfo.index];
}

uint32_t GltfProcessor::ProcessMesh(const GltfMeshPrimitive& prim)
{
	uint32_t loadedMeshIdx = (uint32_t)loadedMeshes.size();
	loadedMeshes.push_back({});
	Mesh& m = loadedMeshes.back();

	{
		PrimitiveTopologyType topo = PrimitiveTopologyType::Undefined;

		switch (prim.mode)
		{
		case GltfMeshMode::POINTS:
			topo = PrimitiveTopologyType::Point;
			break;
		case GltfMeshMode::LINES:
			topo = PrimitiveTopologyType::Line;
			break;
		case GltfMeshMode::TRIANGLES:
			topo = PrimitiveTopologyType::Triangle;
			break;
		default:
			LOGERROR("Unsupported mesh mode %d", (int)prim.mode);
			topo = PrimitiveTopologyType::Undefined;
		};

		const GltfMaterial& mat = _gltf.materials[prim.material];

		m.material.pipeline.blendMode = mat.alphaMode == GltfAlphaMode::BLEND ? 1 : 0;
		m.material.pipeline.doubleSided = mat.doubleSided;

		m.material.baseColorFactor = float4{ (float)mat.pbr.baseColorFactor.x, (float)mat.pbr.baseColorFactor.y,(float)mat.pbr.baseColorFactor.z,(float)mat.pbr.baseColorFactor.w };		

		m.material.metallicFactor = mat.pbr.metallicFactor;
		m.material.roughnessFactor = mat.pbr.roughnessFactor;

		m.material.baseColorTexture = mat.pbr.hasBaseColorTexture ? ProcessTexture(mat.pbr.baseColorTexture) : Texture_t::INVALID;
		m.material.normalTexture = mat.hasNormalTexture ? ProcessNormalTexture(mat.normalTexture) : Texture_t::INVALID;
		m.material.metallicRoughnessTexture = mat.pbr.hasMetallicRoughnessTexture ? ProcessTexture(mat.pbr.metallicRoughnessTexture) : Texture_t::INVALID;

		m.material.alphaCutoff = mat.alphaCutoff;
		m.material.alphaMask = mat.alphaMode == GltfAlphaMode::MASK;
	}

	{
		const GltfAccessor& accessor = _gltf.accessors[prim.indices];
		const GltfBufferView& bufView = _gltf.bufferViews[accessor.bufferView];

		const size_t offset = accessor.byteOffset + bufView.byteOffset;
		m.indexBuf.buf = CreateIndexBuffer(_gltf.data.get() + offset, accessor.count * GltfLoader_SizeOfComponent(accessor.componentType) * GltfLoader_ComponentCount(accessor.type));
		m.indexBuf.count = accessor.count;
		m.indexBuf.offset = 0;
		m.indexBuf.format = GltfLoader_SizeOfComponent(accessor.componentType) == 2 ? RenderFormat::R16_UINT : RenderFormat::R32_UINT;
	}		
		
	for (const GltfMeshAttribute& attr : prim.attributes)
	{
		BindVertexBuffer* targetBuf = nullptr;
		if (attr.semantic == "POSITION") targetBuf = &m.positionBuf;
		else if (attr.semantic == "NORMAL") targetBuf = &m.normalBuf;
		else if (attr.semantic == "TANGENT") targetBuf = &m.tangentBuf;
		else if (attr.semantic == "TEXCOORD_0") targetBuf = &m.texcoordBufs[0];
		else
		{
			LOGWARNING("Unsupported buffer in ProcessMesh %s", attr.semantic.c_str());
		}
		
		const GltfAccessor& accessor = _gltf.accessors[attr.index];
		const GltfBufferView& bufView = _gltf.bufferViews[accessor.bufferView];
		const GltfBuffer& buf = _gltf.buffers[bufView.buffer];

		targetBuf->offset = 0;
		targetBuf->stride = GltfLoader_SizeOfComponent(accessor.componentType) * GltfLoader_ComponentCount(accessor.type);
		targetBuf->buf = CreateVertexBuffer(_gltf.data.get() + accessor.byteOffset + bufView.byteOffset, accessor.count * targetBuf->stride);
	}

	return loadedMeshIdx;
}

uint32_t GltfProcessor::ProcessNode(int32_t nodeIdx, uint32_t parentIdx)
{
	const GltfNode& node = _gltf.nodes[nodeIdx];

	uint32_t modelIdx = (uint32_t)loadedModels.size();
	loadedModels.push_back({});
	Model& m = loadedModels.back();

	m.transform = parentIdx != 0 ? loadedModels[parentIdx].transform : MakeMatrixIdentity();

	m.transform = m.transform * (matrix((float)node.matrix.m[0], (float)node.matrix.m[4], (float)node.matrix.m[8], (float)node.matrix.m[12],
		(float)node.matrix.m[1], (float)node.matrix.m[5], (float)node.matrix.m[9], (float)node.matrix.m[13],
		(float)node.matrix.m[2], (float)node.matrix.m[6], (float)node.matrix.m[10], (float)node.matrix.m[14],
		(float)node.matrix.m[3], (float)node.matrix.m[7], (float)node.matrix.m[11], (float)node.matrix.m[15]));

	if (node.mesh >= 0)
	{
		const GltfMesh& mesh = _gltf.meshes[node.mesh];
		for (const GltfMeshPrimitive& prim : mesh.primitives)
			m.meshes.push_back(ProcessMesh(prim));
	}

	for (const uint32_t child : node.children)
		ProcessNode(child, modelIdx);

	return modelIdx;
}

void GltfProcessor::ProcessScenes()
{
	for (const GltfScene& scene : _gltf.scenes)
	{
		for (const uint32_t nodeIdx : scene.nodes)
		{
			ProcessNode(nodeIdx, 0);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// UI
///////////////////////////////////////////////////////////////////////////////

void DrawUI()
{
	if (!ImGui::Begin("Gltf Viewer"))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::SliderFloat("Sun Pitch", &lightData.sunPitchYaw.x, -90.0f, 90.0f);
	ImGui::SliderFloat("Sun Yaw", &lightData.sunPitchYaw.y, -180.0f, 180.0f);
	ImGui::DragFloat3("Radiance", lightData.radiance.v);
	ImGui::DragFloat3("Ambient", lightData.ambient.v);

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		LOGERROR("Requires a path");
		return 1;
	}

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Render Example", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, L"Gltf Viewew", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	loadedTextures.resize(1);
	loadedMeshes.resize(1);
	loadedModels.resize(1);

	Gltf gltfModel;
	if (!GltfLoader_Load(argv[1], &gltfModel))
		return 1;

	if (!Render_Init())
	{
		Render_ShutDown();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	GltfProcessor processor{gltfModel};
	processor.ProcessScenes();

	{
		std::vector<SamplerDesc> samplers(2);
		samplers[0].AddressModeUVW(SamplerAddressMode::Wrap).FilterModeMinMagMip(SamplerFilterMode::Point);
		samplers[1].AddressModeUVW(SamplerAddressMode::Wrap).FilterModeMinMagMip(SamplerFilterMode::Linear);

		InitSamplers(samplers.data(), samplers.size());
	}

	RenderViewPtr view = CreateRenderViewPtr((intptr_t)hwnd);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplRender_Init();

	HighResolutionClock updateClock;

	screenData.cam.SetView(float3{ -2, 6, -2 }, 0.0f, 45.0f);

	InitPipelines();

	// Main loop
	bool bQuit = false;
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (bQuit == false && msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		updateClock.Tick();
		const float delta = (float)updateClock.GetDeltaSeconds();

		screenData.cam.UpdateView(delta);

		{
			ImGui_ImplRender_NewFrame();
			ImGui_ImplWin32_NewFrame();

			ImGui::NewFrame();

			DrawUI();

			ImGui::Render();
		}

		Render_NewFrame();

		struct MeshProxy
		{
			MaterialID pipeline;
			DynamicBuffer_t meshBuf;
			float dist;
			uint32_t meshId;
		};

		std::vector<MeshProxy> opaqueMeshes;
		std::vector<MeshProxy> translucentMeshes;

		opaqueMeshes.resize(loadedMeshes.size());
		translucentMeshes.resize(loadedMeshes.size());

		u32 opaqueMeshIt = 0;
		u32 translucentMeshIt = 0;

		for (const auto& model : loadedModels)
		{
			struct MeshConstants
			{
				matrix transform;

				float4 albedoTint;

				float metallicFactor;
				float roughnessFactor;
				u32 useAlbedoTex = 0;
				u32 useNormalTex = 0;

				u32 useMetallicRoughnessTex = 0;
				u32 alphaMask = 0;
				float blendCutoff = 0;
				u32 pad;
			} meshConsts;

			meshConsts.transform = model.transform;

			for (uint32_t meshId : model.meshes)
			{
				const Mesh& mesh = loadedMeshes[meshId];

				MeshProxy& proxy = (mesh.material.pipeline.blendMode == 1) ? translucentMeshes[translucentMeshIt++] : opaqueMeshes[opaqueMeshIt++];

				proxy.pipeline = mesh.material.pipeline;

				proxy.meshId = meshId;

				meshConsts.albedoTint = mesh.material.baseColorFactor;
				meshConsts.metallicFactor = mesh.material.metallicFactor;
				meshConsts.roughnessFactor = mesh.material.roughnessFactor;

				meshConsts.useAlbedoTex = mesh.material.baseColorTexture != Texture_t::INVALID;
				meshConsts.useNormalTex = mesh.material.normalTexture != Texture_t::INVALID;
				meshConsts.useMetallicRoughnessTex = mesh.material.metallicRoughnessTexture != Texture_t::INVALID;
				meshConsts.alphaMask = mesh.material.alphaMask;
				meshConsts.blendCutoff = mesh.material.alphaCutoff;

				proxy.meshBuf = CreateDynamicConstantBuffer(&meshConsts, sizeof(meshConsts));

				proxy.dist = LengthSqrF3(float3(model.transform._14, model.transform._24, model.transform._34) - screenData.cam.GetPosition() );
			}
		}

		opaqueMeshes.resize(opaqueMeshIt);
		translucentMeshes.resize(translucentMeshIt);

		std::sort(opaqueMeshes.begin(), opaqueMeshes.end(), [](const MeshProxy& a, const MeshProxy& b) {return a.dist < b.dist; });
		std::sort(translucentMeshes.begin(), translucentMeshes.end(), [](const MeshProxy& a, const MeshProxy& b) {return a.dist > b.dist; });

		CommandListPtr cl = CommandList::Create();

		view->ClearCurrentBackBufferTarget(cl.get());

		DepthStencilView_t dsv = GetTextureDSV(screenData.DepthTex);

		if (dsv != DepthStencilView_t::INVALID)
			cl->ClearDepth(dsv, 1.0f);

		RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();
		cl->SetRenderTargets(&backBufferRtv, 1, dsv);

		Viewport vp;
		vp.width = static_cast<float>(screenData.w);
		vp.height = static_cast<float>(screenData.h);
		vp.minDepth = 0;
		vp.maxDepth = 1;
		vp.topLeftX = 0;
		vp.topLeftY = 0;
		
		cl->SetViewports(&vp, 1);
		cl->SetDefaultScissor();

		struct
		{
			matrix viewProjMat;
			float3 camPos;
			float pad0;
			float3 lightDir;
			float pad1;
			float3 lightRadiance;
			float pad2;
			float3 lightAmbient;
			float pad3;
		} viewBufData;

		viewBufData.viewProjMat = screenData.cam.GetView() * screenData.cam.GetProjection();
		viewBufData.camPos = screenData.cam.GetPosition();

		const float pitchRad = ConvertToRadians(lightData.sunPitchYaw.x);
		const float yawRad = ConvertToRadians(lightData.sunPitchYaw.y);

		viewBufData.lightDir = NormalizeF3(float3{ sinf(yawRad), sinf(-pitchRad), cosf(yawRad) });
		viewBufData.lightRadiance = lightData.radiance;
		viewBufData.lightAmbient = lightData.ambient;

		DynamicBuffer_t viewBuf = CreateDynamicConstantBuffer(&viewBufData, sizeof(viewBufData));

		cl->BindVertexCBVs(0, 1, &viewBuf);
		cl->BindPixelCBVs(0, 1, &viewBuf);

		auto DrawProxies = [&](const std::vector<MeshProxy>& proxies)
		{
			for (const auto& p : proxies)
			{
				cl->SetPipelineState(pipelines[p.pipeline.opaque]);

				cl->BindVertexCBVs(1, 1, &p.meshBuf);
				cl->BindPixelCBVs(1, 1, &p.meshBuf);

				const Mesh& mesh = loadedMeshes[p.meshId];

				Texture_t textures[] =
				{
					mesh.material.baseColorTexture,
					mesh.material.normalTexture,
					mesh.material.metallicRoughnessTexture,
				};
				cl->BindPixelTextures(0, ARRAYSIZE(textures), textures);

				cl->SetVertexBuffers(0, 1, &mesh.positionBuf.buf, &mesh.positionBuf.stride, &mesh.positionBuf.offset);
				cl->SetVertexBuffers(1, 1, &mesh.normalBuf.buf, &mesh.normalBuf.stride, &mesh.normalBuf.offset);
				cl->SetVertexBuffers(2, 1, &mesh.tangentBuf.buf, &mesh.tangentBuf.stride, &mesh.tangentBuf.offset);
				cl->SetVertexBuffers(3, 1, &mesh.texcoordBufs[0].buf, &mesh.texcoordBufs[0].stride, &mesh.texcoordBufs[0].offset);
				cl->SetIndexBuffer(mesh.indexBuf.buf, mesh.indexBuf.format, mesh.indexBuf.offset);
				cl->DrawIndexedInstanced(mesh.indexBuf.count, 1, 0, 0, 0);
			}
			
		};

		DrawProxies(opaqueMeshes);

		DrawProxies(translucentMeshes);

		ImGui_ImplRender_RenderDrawData(ImGui::GetDrawData(), cl.get());

		CommandList::Execute(cl);
		view->Present(true);
	}

	ImGui_ImplRender_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Render_ShutDown();

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	RenderView* rv = GetRenderViewForHwnd((intptr_t)hWnd);

	switch (msg)
	{
	case WM_MOVE:
	{
		RECT r;
		GetWindowRect(hWnd, &r);
		const int x = (int)(r.left);
		const int y = (int)(r.top);
		break;
	}
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			const int w = (int)LOWORD(lParam);
			const int h = (int)HIWORD(lParam);

			if (rv)	rv->Resize(w, h);

			ResizeTargets(w, h);
			return 0;
		}

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


