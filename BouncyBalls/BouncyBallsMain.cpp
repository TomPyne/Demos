// Render Example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "../Render/Render.h"

#include "ThirdParty/imgui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_render.h"

#include "Utils/Camera/FlyCamera.h"
#include "Utils/HighResolutionClock.h"
#include "Utils/Logging.h"
#include "Utils/SurfMath.h"

#include <entt/entt.hpp>

struct
{
	u32 w = 0;
	u32 h = 0;
	FlyCamera cam;
	Texture_t DepthTex = Texture_t::INVALID;
} screenData;

struct
{
	float2 sunPitchYaw = float2{ 70.0f, 0.0f };
	float3 radiance = float3{ 5.0f };
	float3 ambient = float3{ 0.02f, 0.02f, 0.04f };
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
	const char* shaderPath = "BouncyBalls/Shaders/Mesh.hlsl";

	VertexShader_t vs = CreateVertexShader(shaderPath);
	PixelShader_t blendPs = CreatePixelShader(shaderPath);
	PixelShader_t maskPs = CreatePixelShader(shaderPath, { "ALPHA_MASK" });

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

struct MaterialParams
{
	float4 baseColorFactor = float4{ 1.0f };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;

	bool alphaMask = false;
	float alphaCutoff = 0.5f;
};

struct MaterialInstance
{
	MaterialID pipeline;

	MaterialParams params;

	Texture_t baseColorTexture = Texture_t::INVALID;
	Texture_t normalTexture = Texture_t::INVALID;
	Texture_t metallicRoughnessTexture = Texture_t::INVALID;
};

struct MeshVertexBuffer
{
	VertexBuffer_t buffer = VertexBuffer_t::INVALID;
	uint32_t offset = 0;
	uint32_t stride = 0;
};

struct MeshIndexBuffer
{
	IndexBuffer_t buffer = IndexBuffer_t::INVALID;
	RenderFormat format = RenderFormat::UNKNOWN;
	uint32_t offset = 0;
	uint32_t count = 0;
};

struct MeshBuffers
{
	MeshVertexBuffer positionBuf = {};
	MeshVertexBuffer normalBuf = {};
	MeshVertexBuffer tangentBuf = {};
	MeshVertexBuffer texcoordBuf = {};
	MeshIndexBuffer indexBuf = {};
};

MeshBuffers MakeSphereMesh(u32 slices, u32 stacks)
{
	u32 indexCount = slices * 6u + slices * (stacks - 2u) * 6u;
	u32 vertexCount = slices * (stacks - 1) + 2u;

	ASSERTMSG(indexCount <= (u16)0xfff7, "Too many indices for 16 bit buffer");

	std::vector<float3> posVerts(vertexCount);
	std::vector<float3> normVerts(vertexCount);
	std::vector<float4> tangentVerts(vertexCount);
	std::vector<float2> texCoordVerts(vertexCount);
	std::vector<u16> indices(indexCount);

	float3* posIter = &posVerts.front();
	float3* normIter = &normVerts.front();
	float4* tanIter = &tangentVerts.front();
	float2* tcIter = &texCoordVerts.front();
	u16* idxIter = &indices.front();

	*posIter++ = float3{0, 0.5f, 0};
	*normIter++ = float3{0, 1.0f, 0};
	*tanIter++ = float4{1.0f, 0, 0, 0};
	*tcIter++ = float4{0, 0};

	const float stacksRcp = 1.0f / float(stacks);
	const float slicesRcp = 1.0f / float(slices);

	for (u32 i = 0; i < stacks - 1; i++)
	{
		const float u = float(i) * stacksRcp;
		const float phi = K_PI * float(i + 1) * stacksRcp;
		const float sinPhi = sinf(phi);
		const float cosPhi = cosf(phi);

		for (u32 j = 0; j < slices; j++)
		{
			const float v = float(j) * slicesRcp;
			const float theta = 2.0f * K_PI * float(j) * slicesRcp;
			const float sinTheta = sinf(theta);
			const float cosTheta = cosf(theta);

			float3 pos = float3(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta);
			float3 normal = NormalizeF3(pos);

			*posIter++ = pos * 0.5f;
			*normIter++ = normal;
			*tanIter++ = CrossF3(normal, float3(0, 1, 0));
			*tcIter++ = float2(u, v);
		}
	}

	*posIter++ = float3{ 0.0f, -0.5f, 0.0f };
	*normIter++ = float3{ 0.0f, -1.0f, 0.0f };
	*tanIter++ = float4{ -1.0f, 0.0f, 0.0f, 0.0f };
	*tcIter++ = float4{ 1.0f, 1.0f };

	MeshBuffers buffers = {};

	buffers.positionBuf.buffer = CreateVertexBuffer(posVerts.data(), posVerts.size() * sizeof(float3));
	buffers.positionBuf.offset = 0;
	buffers.positionBuf.stride = sizeof(float3);

	buffers.normalBuf.buffer = CreateVertexBuffer(normVerts.data(), normVerts.size() * sizeof(float3));
	buffers.normalBuf.offset = 0;
	buffers.normalBuf.stride = sizeof(float3);

	buffers.tangentBuf.buffer = CreateVertexBuffer(tangentVerts.data(), tangentVerts.size() * sizeof(float4));
	buffers.tangentBuf.offset = 0;
	buffers.tangentBuf.stride = sizeof(float4);

	buffers.texcoordBuf.buffer = CreateVertexBuffer(texCoordVerts.data(), texCoordVerts.size() * sizeof(float2));
	buffers.texcoordBuf.offset = 0;
	buffers.texcoordBuf.stride = sizeof(float2);

	for (u32 i = 0; i < slices; i++)
	{
		*idxIter++ = (i + 1) % slices + 1;
		*idxIter++ = i + 1;
		*idxIter++ = 0;
		*idxIter++ = vertexCount - 1;
		*idxIter++ = i + slices * (stacks - 2) + 1;
		*idxIter++ = (i + 1) % slices + slices * (stacks - 2) + 1;
	}

	for (u32 j = 0; j < stacks - 2; j++)
	{
		u16 j0 = j * slices + 1;
		u16 j1 = (j + 1) * slices + 1;
		for (u32 i = 0; i < slices; i++)
		{
			u16 i0 = j0 + i;
			u16 i1 = j0 + (i + 1) % slices;
			u16 i2 = j1 + (i + 1) % slices;
			u16 i3 = j1 + i;

			*idxIter++ = i0;
			*idxIter++ = i1;
			*idxIter++ = i2;
			*idxIter++ = i2;
			*idxIter++ = i3;
			*idxIter++ = i0;
		}
	}

	buffers.indexBuf.buffer = CreateIndexBuffer(indices.data(), indices.size() * sizeof(uint16_t));
	buffers.indexBuf.format = RenderFormat::R16_UINT;
	buffers.indexBuf.offset = 0;
	buffers.indexBuf.count = indices.size();

	return buffers;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main()
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Render Example", NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, L"Render Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	if (!Render_Init())
	{
		Render_ShutDown();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	{
		std::vector<SamplerDesc> samplers(3);
		samplers[0].AddressModeUVW(SamplerAddressMode::Wrap).FilterModeMinMagMip(SamplerFilterMode::Point);
		samplers[1].AddressModeUVW(SamplerAddressMode::Wrap).FilterModeMinMagMip(SamplerFilterMode::Linear);
		samplers[2].AddressModeUVW(SamplerAddressMode::Clamp).FilterModeMinMagMip(SamplerFilterMode::Linear);

		InitSamplers(samplers.data(), samplers.size());
	}

	RenderViewPtr view = CreateRenderViewPtr((intptr_t)hwnd);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplRender_Init();

	entt::registry registry;

	struct HierarchyComponent
	{
		entt::entity parent = {};
		std::vector<entt::entity> children;
	};

	struct TransformComponent
	{
		float3 position;
	};

	struct MeshComponent
	{
		MeshBuffers buffers;
		MaterialInstance material;
	};

	HighResolutionClock updateClock;

	screenData.cam.SetView(float3{ -2, 2, -2 }, 0.0f, 45.0f);

	InitPipelines();

	MeshComponent mesh;
	mesh.buffers = MakeSphereMesh(8, 8);

	std::vector<float3> positions;

	for (int y = -16; y < 16; y++)
	{
		for (int x = -16; x < 16; x++)
		{
			auto ent = registry.create();
			registry.emplace<TransformComponent>(ent, float3{ (float)x * 2.0f, 0.0f, float(y) * 2.0f });

			MeshComponent& component = registry.emplace<MeshComponent>(ent);
			component.buffers = mesh.buffers;		
		}
	}	

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

		ImGui_ImplRender_NewFrame();

		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		ImGui::Render();

		updateClock.Tick();
		const float delta = (float)updateClock.GetDeltaSeconds();

		screenData.cam.UpdateView(delta);

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
			float preExposure;
			float3 lightDir;
			float pad1;
			float3 lightRadiance;
			float pad2;
			float3 lightAmbient;
			float pad3;
		} viewBufData;

		viewBufData.viewProjMat = screenData.cam.GetView() * screenData.cam.GetProjection();
		viewBufData.camPos = screenData.cam.GetPosition();
		viewBufData.preExposure = 1.0f;

		const float pitchRad = ConvertToRadians(lightData.sunPitchYaw.x);
		const float yawRad = ConvertToRadians(lightData.sunPitchYaw.y);

		viewBufData.lightDir = NormalizeF3(float3{ sinf(yawRad), sinf(-pitchRad), cosf(yawRad) });
		viewBufData.lightRadiance = lightData.radiance;
		viewBufData.lightAmbient = lightData.ambient;

		DynamicBuffer_t viewBuf = CreateDynamicConstantBuffer(&viewBufData, sizeof(viewBufData));

		cl->BindVertexCBVs(0, 1, &viewBuf);
		cl->BindPixelCBVs(0, 1, &viewBuf);

		auto entView = registry.view<const TransformComponent, const MeshComponent>();

		bool done = false;
		for (auto [entity, pos, mesh] : entView.each())
		{
			//if (!done)
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

				meshConsts.transform = MakeMatrixTranslation(pos.position);

				meshConsts.albedoTint = mesh.material.params.baseColorFactor;
				meshConsts.metallicFactor = mesh.material.params.metallicFactor;
				meshConsts.roughnessFactor = mesh.material.params.roughnessFactor;

				meshConsts.useAlbedoTex = mesh.material.baseColorTexture != Texture_t::INVALID;
				meshConsts.useNormalTex = mesh.material.normalTexture != Texture_t::INVALID;
				meshConsts.useMetallicRoughnessTex = mesh.material.metallicRoughnessTexture != Texture_t::INVALID;
				meshConsts.alphaMask = mesh.material.params.alphaMask;
				meshConsts.blendCutoff = mesh.material.params.alphaCutoff;

				DynamicBuffer_t meshBuf = CreateDynamicConstantBuffer(&meshConsts, sizeof(meshConsts));

				cl->SetPipelineState(pipelines[mesh.material.pipeline.opaque]);

				cl->BindVertexCBVs(1, 1, &meshBuf);
				cl->BindPixelCBVs(1, 1, &meshBuf);


				{

					Texture_t textures[] =
					{
						mesh.material.baseColorTexture,
						mesh.material.normalTexture,
						mesh.material.metallicRoughnessTexture,
					};
					cl->BindTexturesAsPixelSRVs(0, textures);


					cl->SetVertexBuffers(0, 1, &mesh.buffers.positionBuf.buffer, &mesh.buffers.positionBuf.stride, &mesh.buffers.positionBuf.offset);
					cl->SetVertexBuffers(1, 1, &mesh.buffers.normalBuf.buffer, &mesh.buffers.normalBuf.stride, &mesh.buffers.normalBuf.offset);
					cl->SetVertexBuffers(2, 1, &mesh.buffers.tangentBuf.buffer, &mesh.buffers.tangentBuf.stride, &mesh.buffers.tangentBuf.offset);
					cl->SetVertexBuffers(3, 1, &mesh.buffers.texcoordBuf.buffer, &mesh.buffers.texcoordBuf.stride, &mesh.buffers.texcoordBuf.offset);

					cl->SetIndexBuffer(mesh.buffers.indexBuf.buffer, mesh.buffers.indexBuf.format, mesh.buffers.indexBuf.offset);
					done = true;
				}
			}


			cl->DrawIndexedInstanced(mesh.buffers.indexBuf.count, 1, 0, 0, 0);
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
