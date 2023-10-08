// Render Example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Render/Render.h"
#include "Utils/GltfLoader.h"
#include "Utils/HighResolutionClock.h"
#include "Utils/KeyCodes.h"
#include "Utils/SurfMath.h"

#include "ThirdParty/imgui/imgui.h"
#include "ImGui/imgui_impl_render.h"
#include "ImGui/imgui_impl_win32.h"


///////////////////////////////////////////////////////////////////////////////
// Render data
///////////////////////////////////////////////////////////////////////////////
struct
{
	u32 w = 0;
	u32 h = 0;
	float nearZ = 0.1f;
	float farZ = 10'000.0f;
	float fov = 45.0f;
	float aspectRatio = 0.0f;
	matrix projection;
	Texture_t DepthTex = Texture_t::INVALID;
} screenData;

struct
{
	float3 position;
	float3 lookDir;
	float camPitch = 0.0f;
	float camYaw = 0.0f;
	matrix view;
} viewData;

static void ResizeTargets(u32 w, u32 h)
{
	w = Max(w, 1u);
	h = Max(h, 1u);

	if (w == screenData.w && h == screenData.h)
		return;

	screenData.w = w;
	screenData.h = h;

	screenData.aspectRatio = (float)w / (float)h;

	screenData.projection = MakeMatrixPerspectiveFovLH(ConvertToRadians(screenData.fov), screenData.aspectRatio, screenData.nearZ, screenData.farZ);

	Render_Release(screenData.DepthTex);

	TextureCreateDesc desc = {};
	desc.width = w;
	desc.height = h;
	desc.format = RenderFormat::D32_FLOAT;
	desc.flags = RenderResourceFlags::DSV;
	screenData.DepthTex = CreateTexture(desc);
}

static void UpdateView(const float3& position, float pitch, float yaw)
{
	viewData.position = position;

	if (yaw > 360.0f)
		yaw -= 360.0f;

	if (yaw < -360.0f)
		yaw += 360.0f;

	viewData.camPitch = pitch;
	viewData.camYaw = yaw;

	pitch = Clamp(pitch, -89.9f, 89.9f);

	yaw = ConvertToRadians(yaw);
	pitch = ConvertToRadians(pitch);

	float cosPitch = cosf(pitch);

	viewData.lookDir = float3{ cosf(yaw) * cosPitch, sinf(pitch), sinf(yaw) * cosPitch };

	viewData.view = MakeMatrixLookToLH(position, viewData.lookDir, float3{ 0, 1, 0 });
}

static void CameraUpdate(float delta)
{
	ImGuiIO& io = ImGui::GetIO();

	float camPitch = viewData.camPitch;
	float camYaw = viewData.camYaw;

	if (!io.WantCaptureMouse && io.MouseDown[1])
	{
		float yaw = ImGui::GetIO().MouseDelta.x;
		float pitch = ImGui::GetIO().MouseDelta.y;

		camPitch -= pitch * 25.0f * delta;
		camYaw -= yaw * 25.0f * delta;
	}

	float3 translation = { 0.0f };

	if (!io.WantCaptureKeyboard)
	{
		float3 fwd = viewData.lookDir;
		float3 rgt = CrossF3(float3{ 0, 1, 0 }, viewData.lookDir);

		constexpr float speed = 5.0f;

		float moveSpeed = speed * delta;

		float3 translateDir = 0.0f;

		if (io.KeysDown[KeyCode::W]) translateDir += fwd;
		if (io.KeysDown[KeyCode::S]) translateDir -= fwd;

		if (io.KeysDown[KeyCode::D]) translateDir += rgt;
		if (io.KeysDown[KeyCode::A]) translateDir -= rgt;

		if (io.KeyShift)
			moveSpeed *= 4.0f;

		translation = NormalizeF3(translateDir) * moveSpeed;

		if (io.KeysDown[KeyCode::E]) translation.y += moveSpeed;
		if (io.KeysDown[KeyCode::Q]) translation.y -= moveSpeed;
	}

	UpdateView(viewData.position + translation, camPitch, camYaw);
}

GraphicsPipelineState_t CreateMaterial()
{
	GraphicsPipelineStateDesc desc = {};
	desc.RasterizerDesc(PrimitiveTopologyType::Triangle, FillMode::Solid, CullMode::None);
	desc.DepthDesc(true, ComparisionFunc::LessEqual);
	desc.numRenderTargets = 1;
	desc.blendMode[0].None();

	const char* shaderPath = "Gltf Viewer/Mesh.hlsl";

	desc.vs = CreateVertexShader(shaderPath);
	desc.ps = CreatePixelShader(shaderPath);

	InputElementDesc inputDesc[] =
	{
		{"POSITION", 0, RenderFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PerVertex, 0 },
		{"NORMAL", 0, RenderFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PerVertex, 0 },
	};

	return CreateGraphicsPipelineState(desc, inputDesc, ARRAYSIZE(inputDesc));
}

///////////////////////////////////////////////////////////////////////////////
// Assets
///////////////////////////////////////////////////////////////////////////////

struct Material
{
	GraphicsPipelineState_t pso;
	std::vector<Texture_t> boundTextures;
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
	std::vector<BindVertexBuffer> texcoordBufs;
	std::vector<BindVertexBuffer> colorBufs;
	std::vector<BindVertexBuffer> jointBufs;
	std::vector<BindVertexBuffer> weightBufs;
	BindIndexBuffer indexBuf;
};

struct Model
{
	matrix transform;
	std::vector<uint32_t> meshes;
};

std::vector<Texture_t> loadedTextures;
std::vector<Mesh> loadedMeshes;
std::vector<Model> loadedModels;
std::vector<Material> loadedMaterials;

///////////////////////////////////////////////////////////////////////////////
// Asset Processing
///////////////////////////////////////////////////////////////////////////////

struct GltfProcessor
{
	const Gltf& _gltf;

	explicit GltfProcessor(const Gltf& gltf) : _gltf(gltf) {}

	uint32_t ProcessMesh(const GltfMeshPrimitive& prim);
	uint32_t ProcessNode(int32_t nodeIdx);
	uint32_t ProcessMaterial(int32_t materialIdx);

	void Process();
};

uint32_t GltfProcessor::ProcessMesh(const GltfMeshPrimitive& prim)
{
	uint32_t loadedMeshIdx = (uint32_t)loadedMeshes.size();
	loadedMeshes.push_back({});
	Mesh& m = loadedMeshes.back();

	{
		const GltfAccessor& accessor = _gltf.accessors[prim.indices];
		const GltfBufferView& bufView = _gltf.bufferViews[accessor.bufferView];
		const GltfBuffer& buf = _gltf.buffers[bufView.buffer];

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
		else continue;
		
		const GltfAccessor& accessor = _gltf.accessors[attr.index];
		const GltfBufferView& bufView = _gltf.bufferViews[accessor.bufferView];
		const GltfBuffer& buf = _gltf.buffers[bufView.buffer];

		targetBuf->offset = 0;
		targetBuf->stride = GltfLoader_SizeOfComponent(accessor.componentType) * GltfLoader_ComponentCount(accessor.type);
		targetBuf->buf = CreateVertexBuffer(_gltf.data.get() + accessor.byteOffset + bufView.byteOffset, accessor.count * targetBuf->stride);
	}

	return loadedMeshIdx;
}

uint32_t GltfProcessor::ProcessNode(int32_t nodeIdx)
{
	const GltfNode& node = _gltf.nodes[nodeIdx];

	if (node.mesh < 0)
		return 0;

	const GltfMesh& mesh = _gltf.meshes[node.mesh];
	if (mesh.primitives.empty())
		return 0;

	uint32_t modelIdx = (uint32_t)loadedModels.size();
	loadedModels.push_back({});
	Model& m = loadedModels.back();

	{
		m.transform = matrix((float)node.matrix.m[0], (float)node.matrix.m[4], (float)node.matrix.m[8], (float)node.matrix.m[12],
			(float)node.matrix.m[1], (float)node.matrix.m[5], (float)node.matrix.m[9], (float)node.matrix.m[13],
			(float)node.matrix.m[2], (float)node.matrix.m[6], (float)node.matrix.m[10], (float)node.matrix.m[14],
			(float)node.matrix.m[3], (float)node.matrix.m[7], (float)node.matrix.m[11], (float)node.matrix.m[15]);

		matrix translate = MakeMatrixTranslation(float3{ (float)node.translation.x, (float)node.translation.y, (float)node.translation.z });
		matrix rotate = MakeMatrixRotationFromQuaternion(float4{ (float)node.rotation.x, (float)node.rotation.y, (float)node.rotation.z, (float)node.rotation.w });
		matrix scale = MakeMatrixScaling((float)node.scale.x, (float)node.scale.y, (float)node.scale.z);

		matrix t = translate * rotate * scale;

		m.transform = m.transform * t;
	}

	for (const GltfMeshPrimitive& prim : mesh.primitives)
		m.meshes.push_back(ProcessMesh(prim));

	return 0;
}

uint32_t GltfProcessor::ProcessMaterial(int32_t materialIdx)
{
	const GltfMaterial& material = _gltf.materials[materialIdx];

	return 0;
}

void GltfProcessor::Process()
{
	for (int32_t nodeIt = 0; nodeIt < _gltf.nodes.size(); nodeIt++)
		ProcessNode(nodeIt);
}

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main()
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Render Example", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, L"Render Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	loadedTextures.resize(1);
	loadedMeshes.resize(1);
	loadedModels.resize(1);

	Gltf gltfModel;
	if (!GltfLoader_Load("../Content/x-wing.glb", &gltfModel))
		return 1;

	if (!Render_Init())
	{
		Render_ShutDown();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	GltfProcessor processor{gltfModel};
	processor.Process();

	{
		std::vector<SamplerDesc> samplers(1);
		samplers[0].AddressModeUVW(SamplerAddressMode::Wrap).FilterModeMinMagMip(SamplerFilterMode::Point);

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

	UpdateView(float3{ -2, 6, -2 }, 0.0f, 45.0f);

	GraphicsPipelineState_t pso = CreateMaterial();

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

		CameraUpdate(delta);

		ImGui_ImplRender_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();

		Render_NewFrame();
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
			float pad;
		} viewBufData;

		viewBufData.viewProjMat = viewData.view * screenData.projection;
		viewBufData.camPos = viewData.position;

		DynamicBuffer_t viewBuf = CreateDynamicConstantBuffer(&viewBufData, sizeof(viewBufData));

		cl->BindVertexCBVs(0, 1, &viewBuf);
		cl->BindPixelCBVs(0, 1, &viewBuf);

		// Draw meshes

		cl->SetPipelineState(pso);

		for (const auto& model : loadedModels)
		{
			DynamicBuffer_t transformBuf = CreateDynamicConstantBuffer(&model.transform, sizeof(model.transform));
			cl->BindVertexCBVs(1, 1, &transformBuf);

			for (uint32_t meshId : model.meshes)
			{
				const Mesh& mesh = loadedMeshes[meshId];
				cl->SetVertexBuffers(0, 1, &mesh.positionBuf.buf, &mesh.positionBuf.stride, &mesh.positionBuf.offset);
				cl->SetVertexBuffers(1, 1, &mesh.normalBuf.buf, &mesh.normalBuf.stride, &mesh.normalBuf.offset);
				cl->SetIndexBuffer(mesh.indexBuf.buf, mesh.indexBuf.format, mesh.indexBuf.offset);
				cl->DrawIndexedInstanced(mesh.indexBuf.count, 1, 0, 0, 0);
			}
		}

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


