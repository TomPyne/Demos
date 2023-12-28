// Render Example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Utils/Camera/FlyCamera.h"
#include "Utils/HighResolutionClock.h"
#include "Utils/SurfMath.h"

#include "../Render/Render.h"

#include "ThirdParty/imgui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_render.h"
#include "Utils/Logging.h"
#include "Utils/Rendergraph/RenderGraph.h"

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

union PipelineID
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
		PipelineID curId;
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

struct Material
{
	PipelineID pipeline;

	float4 baseColorFactor = float4{ 1.0f };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;

	Texture_t baseColorTexture = Texture_t::INVALID;
	Texture_t normalTexture = Texture_t::INVALID;
	Texture_t metallicRoughnessTexture = Texture_t::INVALID;

	bool alphaMask = false;
	float alphaCutoff = 0.5f;
};

struct Mesh
{
	BindVertexBuffer positionBuf;
	BindVertexBuffer normalBuf;
	BindVertexBuffer tangentBuf;
	BindVertexBuffer uvBuf;
	BindIndexBuffer indexBuf;

	Material material;
};

Mesh CreateCubeMesh()
{
	Mesh mesh;

	constexpr float3 ftl = float3(-0.5f, 0.5f, 0.5f);
	constexpr float3 ftr = float3(0.5f, 0.5f, 0.5f);
	constexpr float3 fbr = float3(0.5f, -0.5f, 0.5f);
	constexpr float3 fbl = float3(-0.5f, -0.5f, 0.5f);

	constexpr float3 btl = float3(-0.5f, 0.5f, -0.5f);
	constexpr float3 btr = float3(0.5f, 0.5f, -0.5f);
	constexpr float3 bbr = float3(0.5f, -0.5f, -0.5f);
	constexpr float3 bbl = float3(-0.5f, -0.5f, -0.5f);

	float3 PosVerts[6 * 4] =
	{
		ftl, ftr, fbr, fbl,
		btr, btl, bbl, bbr,
		ftr, btr, bbr, fbr,
		btl, ftl, fbl, bbl,
		fbl, fbr, bbr, bbl,
		ftl, btl, btr, ftr,
	};

	mesh.positionBuf.buf = CreateVertexBuffer(PosVerts, sizeof(PosVerts));
	mesh.positionBuf.offset = 0;
	mesh.positionBuf.stride = sizeof(float3);

	constexpr float3 ln = float3(1, 0, 0);
	constexpr float3 un = float3(0, 1, 0);
	constexpr float3 fn = float3(0, 0, 1);

	float3 NormVerts[6 * 4] =
	{
		 fn,  fn,  fn,  fn,
		-fn, -fn, -fn, -fn,
		 ln,  ln,  ln,  ln,
		-ln, -ln, -ln, -ln,
		-un, -un, -un, -un,
		 un,  un,  un,  un,
	};

	mesh.normalBuf.buf = CreateVertexBuffer(NormVerts, sizeof(NormVerts));
	mesh.normalBuf.offset = 0;
	mesh.normalBuf.stride = sizeof(float3);

	constexpr float4 lt = float4(0, 1, 0, 0);
	constexpr float4 ut = float4(0, 0, 1, 0);
	constexpr float4 ft = float4(1, 0, 0, 0);

	float4 TangentVerts[6 * 4] =
	{
		 ut,  ut,  ut,  ut,
		-ft, -ft, -ft, -ft,
		 lt,  lt,  lt,  lt,
		-lt, -lt, -lt, -lt,
		-ut, -ut, -ut, -ut,
		 ut,  ut,  ut,  ut,
	};

	mesh.tangentBuf.buf = CreateVertexBuffer(TangentVerts, sizeof(TangentVerts));
	mesh.tangentBuf.offset = 0;
	mesh.tangentBuf.stride = sizeof(float4);

	constexpr float2 tl = float2(0.0f, 0.0f);
	constexpr float2 tr = float2(1.0f, 0.0f);
	constexpr float2 bl = float2(0.0f, 1.0f);
	constexpr float2 br = float2(1.0f, 1.0f);

	float2 TexcoordVerts[6 * 4] =
	{
		tl, tr, br, bl,
		tl, tr, br, bl,
		tl, tr, br, bl,
		tl, tr, br, bl,
		tl, tr, br, bl,
		tl, tr, br, bl,
	};

	mesh.uvBuf.buf = CreateVertexBuffer(TexcoordVerts, sizeof(TexcoordVerts));
	mesh.uvBuf.offset = 0;
	mesh.uvBuf.stride = sizeof(float2);

	u16 Indices[6 * 6] =
	{
		2, 1, 0, 0, 3, 2,
		6, 5, 4, 4, 7, 6,
		10, 9, 8, 8, 11, 10,
		14, 13, 12, 12, 15, 14,
		18, 17, 16, 16, 19, 18,
		22, 21, 20, 20, 23, 22,
	};

	mesh.indexBuf.buf = CreateIndexBuffer(Indices, sizeof(Indices));
	mesh.indexBuf.count = ARRAYSIZE(Indices);
	mesh.indexBuf.offset = 0;
	mesh.indexBuf.format = RenderFormat::R16_UINT;

	return mesh;
}

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

Mesh g_mesh;

void DrawMesh(CommandList* cl, Mesh& mesh)
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

	meshConsts.transform = MakeMatrixIdentity();

	meshConsts.albedoTint = mesh.material.baseColorFactor;
	meshConsts.metallicFactor = mesh.material.metallicFactor;
	meshConsts.roughnessFactor = mesh.material.roughnessFactor;

	meshConsts.useAlbedoTex = mesh.material.baseColorTexture != Texture_t::INVALID;
	meshConsts.useNormalTex = mesh.material.normalTexture != Texture_t::INVALID;
	meshConsts.useMetallicRoughnessTex = mesh.material.metallicRoughnessTexture != Texture_t::INVALID;
	meshConsts.alphaMask = mesh.material.alphaMask;
	meshConsts.blendCutoff = mesh.material.alphaCutoff;

	DynamicBuffer_t meshBuf = CreateDynamicConstantBuffer(&meshConsts, sizeof(meshConsts));

	cl->SetPipelineState(pipelines[mesh.material.pipeline.opaque]);

	cl->BindVertexCBVs(1, 1, &meshBuf);
	cl->BindPixelCBVs(1, 1, &meshBuf);

	Texture_t textures[] =
	{
		mesh.material.baseColorTexture,
		mesh.material.normalTexture,
		mesh.material.metallicRoughnessTexture,
	};
	cl->BindTexturesAsPixelSRVs(0, textures);

	cl->SetVertexBuffers(0, 1, &mesh.positionBuf.buf, &mesh.positionBuf.stride, &mesh.positionBuf.offset);
	cl->SetVertexBuffers(1, 1, &mesh.normalBuf.buf, &mesh.normalBuf.stride, &mesh.normalBuf.offset);
	cl->SetVertexBuffers(2, 1, &mesh.tangentBuf.buf, &mesh.tangentBuf.stride, &mesh.tangentBuf.offset);
	cl->SetVertexBuffers(3, 1, &mesh.uvBuf.buf, &mesh.uvBuf.stride, &mesh.uvBuf.offset);

	cl->SetIndexBuffer(mesh.indexBuf.buf, mesh.indexBuf.format, mesh.indexBuf.offset);
	cl->DrawIndexedInstanced(mesh.indexBuf.count, 1, 0, 0, 0);
}

void AddScenePass(RenderGraph& rg, RenderGraphResource_t sceneColor, RenderGraphResource_t sceneDepth)
{
	rg.AddPass("Scene", RenderPassType::GRAPHICS)
		.AddRenderTarget(sceneColor, RenderPassOutputAccess::CLEAR)
		.AddDepthTarget(sceneDepth, RenderPassOutputAccess::CLEAR)
		.SetExecuteCallback([sceneColor, sceneDepth](RenderGraph& rdg, CommandList* cl)
		{
			const RenderTargetView_t sceneRtv = rdg.GetRTV(sceneColor);
			const DepthStencilView_t sceneDsv = rdg.GetDSV(sceneDepth);

			const float clearCol[] = { 1.0f, 0.0f, 0.0f, 1.0f };
			cl->ClearRenderTarget(sceneRtv, clearCol);
			cl->ClearDepth(sceneDsv, 1.0f);

			// Could make this a graph function to avoid binding render targets
			// multiple times
			cl->SetRenderTargets(&sceneRtv, 1, sceneDsv);

			{
				Viewport vp{ screenData.w, screenData.h };
				cl->SetViewports(&vp, 1);
				cl->SetDefaultScissor();
			}

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

			DrawMesh(cl, g_mesh);

			// Having to unbind targets just so compute stages can use them is awkward
			// again if we bound render or compute targets through the graph
			// we could handle this automatically.
			RenderTargetView_t empty = RenderTargetView_t::INVALID;
			cl->SetRenderTargets(&empty, 1, DepthStencilView_t::INVALID);
		}
	);
}

void AddTonemapPass(RenderGraph& rg, RenderGraphResource_t input)
{
	static ComputePipelineState_t tonemapCs = ComputePipelineState_t::INVALID;

	if (tonemapCs == ComputePipelineState_t::INVALID)
	{
		ComputePipelineStateDesc desc = {};
		desc.cs = CreateComputeShader("RenderGraph/Shaders/TonemapCS.hlsl");

		tonemapCs = CreateComputePipelineState(desc);
	}

	rg.AddPass("Tonemap_Rheinhardt", RenderPassType::COMPUTE)
		.AddComputeTarget(input, RenderPassOutputAccess::LOAD)
		.SetExecuteCallback([input](RenderGraph& rdg, CommandList* cl)
		{
			const UnorderedAccessView_t uav = rdg.GetUAV(input);
			cl->SetPipelineState(tonemapCs);

			uint3 dimensions = rdg.GetResourceDimensions(input);
			
			cl->BindComputeUAVs(0, 1, &uav);

			struct
			{
				uint2 dimensions;
				float pad[2];
			} shaderData;

			shaderData.dimensions = uint2{ dimensions.x, dimensions.y };

			// Uploading constant data here will probably cause headaches with Dx12
			// Need to find a way of enqueing these during the build stage
			DynamicBuffer_t shaderCbuf = CreateDynamicConstantBuffer(&shaderData, sizeof(shaderData)); 

			cl->BindComputeCBVs(0, 1, &shaderCbuf);

			cl->Dispatch(DivideRoundUp(dimensions.x, 8u), DivideRoundUp(dimensions.y, 8u), 1u);

			UnorderedAccessView_t empty = UnorderedAccessView_t::INVALID;
			cl->BindComputeUAVs(0, 1, &empty);
		}
	);
}

void AddResolvePass(RenderGraph& rg, RenderGraphResource_t dst, RenderGraphResource_t src)
{
	static GraphicsPipelineState_t resolvePSO = GraphicsPipelineState_t::INVALID;

	if (resolvePSO == GraphicsPipelineState_t::INVALID)
	{
		GraphicsPipelineStateDesc desc = {};
		desc.RasterizerDesc(PrimitiveTopologyType::Triangle, FillMode::Solid, CullMode::Front)
			.DepthDesc(false, ComparisionFunc::Always);

		desc.numRenderTargets = 1;
		desc.blendMode[0].None();

		desc.vs = CreateVertexShader("RenderGraph/Shaders/Resolver.hlsl");
		desc.ps = CreatePixelShader("RenderGraph/Shaders/Resolver.hlsl");

		resolvePSO = CreateGraphicsPipelineState(desc);
	}

	rg.AddPass("Resolve", RenderPassType::GRAPHICS)
		.AddRenderTarget(dst, RenderPassOutputAccess::LOAD)
		.ReadResource(src)
		.SetExecuteCallback([dst, src](RenderGraph& rdg, CommandList* cl)
		{
			const RenderTargetView_t rtv = rdg.GetRTV(dst);
			const ShaderResourceView_t srv = rdg.GetSRV(src);

			cl->SetRenderTargets(&rtv, 1, DepthStencilView_t::INVALID);

			cl->SetPipelineState(resolvePSO);

			uint3 dstDim = rdg.GetResourceDimensions(dst);
			{
				Viewport vp{ dstDim.x, dstDim.y };
				cl->SetViewports(&vp, 1);
				cl->SetDefaultScissor();
			}

			struct
			{
				float2 offset;
				float2 scale;
				float2 uvOffset;
				float2 uvScale;
			} shaderData;

			shaderData.offset = { 0.0f, 0.0f };
			shaderData.uvOffset = { 0.0f, 0.0f };
			shaderData.scale = { 1.0f, 1.0f };
			shaderData.uvScale = { 1.0f, -1.0f };

			DynamicBuffer_t shaderBuf = CreateDynamicConstantBuffer(&shaderData, sizeof(shaderData));

			cl->BindVertexCBVs(0, 1, &shaderBuf);

			cl->BindPixelSRVs(0, 1, &srv);

			cl->DrawInstanced(6, 1, 0, 0);
		}
	);
}

void AddUIPass(RenderGraph& rg, RenderGraphResource_t target)
{
	rg.AddPass("UI", RenderPassType::GRAPHICS)
		.AddRenderTarget(target, RenderPassOutputAccess::LOAD)
		.MakeRoot()
		.SetExecuteCallback([target](RenderGraph& rdg, CommandList* cl)
		{
			const RenderTargetView_t rtv = rdg.GetRTV(target);

			cl->SetRenderTargets(&rtv, 1, DepthStencilView_t::INVALID);

			ImGui_ImplRender_RenderDrawData(ImGui::GetDrawData(), cl);
		}
	);
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main()
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Render Graph", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, L"Render Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	if (!Render_Init())
	{
		Render_ShutDown();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

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

	g_mesh = CreateCubeMesh();

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

		RenderGraph rdg;

		RenderGraphResource_t backbuffer = rdg.AddExternalRTV("Backbuffer", view->GetCurrentBackBufferRTV(), view->width, view->height);

		RenderGraphTextureDesc screenTexDesc;
		screenTexDesc.width = screenData.w;
		screenTexDesc.height = screenData.h;

		screenTexDesc.format = RenderFormat::D32_FLOAT;
		RenderGraphResource_t sceneDepth = rdg.RegisterTexture("SceneDepth", screenTexDesc);

		screenTexDesc.format = RenderFormat::R16G16B16A16_FLOAT;
		RenderGraphResource_t sceneColor = rdg.RegisterTexture("SceneColor", screenTexDesc);

		bool postProcess = false;
		//RenderGraphResource_t sceneTarget = postProcess ? sceneColor : backBuffer;

#if 0

		// Bloom
		{
			float2 mipSize = float2((float)screenData.w, (float)screenData.h);
			uint2 mipIntSize = mipSize;

			static const char* BloomDownSampleNames[6] = { "Bloom_Downsample0", "Bloom_Downsample1", "Bloom_Downsample2", "Bloom_Downsample3", "Bloom_Downsample4", "Bloom_Downsample5" };
			static const char* BloomUpSampleNames[6] = { "Bloom_Upsample0", "Bloom_Upsample1", "Bloom_Upsample2", "Bloom_Upsample3", "Bloom_Upsample4", "Bloom_Upsample5" };
			static const char* BloomTextureNames[6] = { "Bloom_HalfRes", "Bloom_QuarterRes", "Bloom_EigthRes", "Bloom_SixteenthRes", "Bloom_ThirtySecondRes", "Bloom_SixtyFourthRes" };

			uint2 mipSizes[6];
			u32 mipCount = 0;
			for (u32 i = 0; i < 6; i++)
			{
				mipSize *= 0.5f;
				mipIntSize /= 2;

				mipIntSize.x = Max(mipIntSize.x, 1u);
				mipIntSize.y = Max(mipIntSize.y, 1u);

				mipSizes[i] = mipIntSize;

				mipCount++;

				if ((mipIntSize.x <= 1 && mipIntSize.y <= 1))
					break;
			}

			RenderGraphResource_t bloomTextures[6];
			for (u32 i = 0; i < mipCount; i++)
			{
				bloomTextures[i] = rdg.AddResource(BloomTextureNames[i]);
			}

			for (u32 i = 0; i < mipCount; i++)
			{
				rdg.AddPass(BloomDownSampleNames[i], RenderPassType::COMPUTE_RW)
					.AddComputeTarget(bloomTextures[i], RenderPassResourceAccess::WRITE)
					.SampleTexture(i == 0 ? sceneColor : bloomTextures[i-1]);
			}

			for (u32 i = mipCount - 1; i > 0; --i)
			{
				rdg.AddPass(BloomUpSampleNames[i], RenderPassType::COMPUTE_RW)
					.AddComputeTarget(bloomTextures[i-1], RenderPassResourceAccess::WRITE)
					.SampleTexture(bloomTextures[i]);
			}

			rdg.AddPass("Bloom_Apply", RenderPassType::COMPUTE_RW)
				.AddComputeTarget(sceneWithBloom, RenderPassResourceAccess::WRITE)
				.SampleTexture(bloomTextures[0]);
		}

#endif
		AddScenePass(rdg, sceneColor, sceneDepth);

		AddTonemapPass(rdg, sceneColor);

		AddResolvePass(rdg, backbuffer, sceneColor);

		AddUIPass(rdg, backbuffer);

		rdg.Build();

		updateClock.Tick();
		const float delta = (float)updateClock.GetDeltaSeconds();

		screenData.cam.UpdateView(delta);

		ImGui_ImplRender_NewFrame();

		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DrawUI();

		ImGui::Render();		

		Render_NewFrame();

		rdg.Execute();

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