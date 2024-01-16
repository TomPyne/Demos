// Render Example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Render/Render.h"
#include "Utils/HighResolutionClock.h"
#include "Utils/KeyCodes.h"
#include "Utils/Logging.h"
#include "Utils/SurfMath.h"
#include "Utils/TextureLoader.h"

#include "ThirdParty/imgui/imgui.h"
#include "ImGui/imgui_impl_render.h"
#include "ImGui/imgui_impl_win32.h"

#include <algorithm>

struct Mesh
{
	VertexBuffer_t vbuf;
	IndexBuffer_t ibuf;
	u32 numIndices;
};

Mesh CreateSphere(u32 slices, u32 stacks)
{
	u32 indexCount = slices * 6u + slices * (stacks - 2u) * 6u;
	u32 vertexCount = slices * (stacks - 1) + 2u;

	assert(indexCount < USHORT_MAX);

	std::vector<float3> verts;
	std::vector<u16> indices;

	verts.resize(vertexCount);
	indices.resize(indexCount);

	float3* vertIter = &verts.front();
	u16* idxIter = &indices.front();

	*vertIter++ = float3(0, 0.5f, 0);

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

			*vertIter++ = pos * 0.5f;
		}
	}

	*vertIter++ = float3(0, -0.5f, 0);

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

	Mesh mesh;
	mesh.vbuf = CreateVertexBuffer(verts.data(), verts.size() * sizeof(float3));
	mesh.ibuf = CreateIndexBuffer(indices.data(), indices.size() * sizeof(u16));
	mesh.numIndices = indices.size();

	return mesh;
}

Mesh CreateVolume()
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

	mesh.vbuf = CreateVertexBuffer(PosVerts, sizeof(PosVerts));

	u16 Indices[6 * 6] =
	{
		2, 1, 0, 0, 3, 2,
		6, 5, 4, 4, 7, 6,
		10, 9, 8, 8, 11, 10,
		14, 13, 12, 12, 15, 14,
		18, 17, 16, 16, 19, 18,
		22, 21, 20, 20, 23, 22,
	};

	mesh.ibuf = CreateIndexBuffer(Indices, sizeof(Indices));

	mesh.numIndices = ARRAYSIZE(Indices);

	return mesh;
}

GraphicsPipelineState_t CreatePipelineState()
{
	const char* shaderPath = "Volumetrics/Volumetrics.hlsl";

	InputElementDesc inputDesc[] =
	{
		{"POSITION", 0, RenderFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PerVertex, 0 },
	};

	GraphicsPipelineStateDesc desc = {};
	desc.RasterizerDesc(PrimitiveTopologyType::Triangle, FillMode::Solid, CullMode::Front);
	desc.DepthDesc(true, ComparisionFunc::LessEqual);
	desc.numRenderTargets = 1;
	desc.blendMode[0].Default();

	desc.vs = CreateVertexShader(shaderPath);
	desc.ps = CreatePixelShader(shaderPath);

	return CreateGraphicsPipelineState(desc, inputDesc, ARRAYSIZE(inputDesc));
}

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

struct
{
	float2 sunPitchYaw = float2{70.0f, 0.0f};
	float3 radiance = float3{5.0f, 5.0f, 5.0f};
	float3 ambient = float3{0.02f, 0.02f, 0.04f};
} lightData;

struct
{
	float sigmaAbsorption = 0.5f;
	float sigmaScatter = 0.5f;
	float density = 5.0f;
	float asymmetry = 0.8f;
	float3 panSpeed = float3( 0, -1, 0 );
	float noiseScale = 5.0f;
	float stepSize = 0.02f;
} scatterData;

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

		constexpr float speed = 1.0f;

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

///////////////////////////////////////////////////////////////////////////////
// UI
///////////////////////////////////////////////////////////////////////////////

void DrawUI()
{
	if (!ImGui::Begin("Volumetrics"))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	if (ImGui::Button("Recompile"))
	{
		ReloadShaders();
	}

	ImGui::SliderFloat("Sun Pitch", &lightData.sunPitchYaw.x, -90.0f, 90.0f);
	ImGui::SliderFloat("Sun Yaw", &lightData.sunPitchYaw.y, -180.0f, 180.0f);
	ImGui::DragFloat3("Radiance", lightData.radiance.v);
	ImGui::DragFloat3("Ambient", lightData.ambient.v);

	ImGui::Separator();
	ImGui::Text("Scattering");
	ImGui::DragFloat("Absorption", &scatterData.sigmaAbsorption, 0.02f);
	ImGui::DragFloat("Scatter", &scatterData.sigmaScatter, 0.02f);
	ImGui::DragFloat("Density", &scatterData.density, 0.02f);
	ImGui::DragFloat("Asymmetry", &scatterData.asymmetry, 0.02f);
	ImGui::InputFloat( "Noise Scale", &scatterData.noiseScale, 0.05f );
	ImGui::InputFloat3( "Pan Speed", scatterData.panSpeed.v );
	ImGui::InputFloat( "Step Size", &scatterData.stepSize );

	scatterData.stepSize = Max( scatterData.stepSize, 0.01f );

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int argc, char* argv[])
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

	UpdateView(float3{ -1, 0, 0 }, 0.0f, 0.0f);

	GraphicsPipelineState_t pso = CreatePipelineState();

	Mesh mesh = CreateVolume();

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

		{
			ImGui_ImplRender_NewFrame();
			ImGui_ImplWin32_NewFrame();

			ImGui::NewFrame();

			DrawUI();

			ImGui::Render();
		}

		scatterData.sigmaAbsorption = Max(scatterData.sigmaAbsorption, 0.0f);
		scatterData.sigmaScatter = Max(scatterData.sigmaScatter, 0.0f);
		scatterData.asymmetry = Clamp(scatterData.asymmetry, -1.0f, 1.0f);

		Render_NewFrame();

		CommandListPtr cl = CommandList::Create();

		constexpr float ClearCol[4] = {0.572f, 0.772f, 0.921f, 0.0f};
		view->ClearCurrentBackBufferTarget(cl.get(), ClearCol );

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
			float totalTime;
			float3 lightDir;
			float pad1;
			float3 lightRadiance;
			float pad2;
			float3 lightAmbient;
			float pad3;
		} viewBufData;

		viewBufData.viewProjMat = viewData.view * screenData.projection;
		viewBufData.camPos = viewData.position;

		const float pitchRad = -ConvertToRadians(lightData.sunPitchYaw.x + 90.0f);
		const float yawRad = ConvertToRadians(lightData.sunPitchYaw.y);

		viewBufData.lightDir = NormalizeF3(float3{ sinf( pitchRad ) * cosf(yawRad), cosf(pitchRad), sinf(pitchRad) *  sinf(yawRad)});
		viewBufData.totalTime = updateClock.GetTotalSeconds();
		viewBufData.lightRadiance = lightData.radiance;
		viewBufData.lightAmbient = lightData.ambient;

		DynamicBuffer_t viewBuf = CreateDynamicConstantBuffer(&viewBufData, sizeof(viewBufData));

		cl->BindVertexCBVs(0, 1, &viewBuf);
		cl->BindPixelCBVs(0, 1, &viewBuf);

		{
			cl->SetPipelineState(pso);

			struct
			{
				matrix transform;

				float sigma_s;
				float sigma_a;
				float asymmetry;
				float noiseScale;

				float3 movementDirection;
				float densityScale;

				float stepSize;
				float3 pad;
			} meshConsts;

			meshConsts.transform = MakeMatrixIdentity();

			meshConsts.sigma_s = scatterData.sigmaScatter;
			meshConsts.sigma_a = scatterData.sigmaAbsorption;
			meshConsts.asymmetry = scatterData.asymmetry;
			meshConsts.noiseScale = scatterData.noiseScale;

			meshConsts.movementDirection = scatterData.panSpeed;			
			meshConsts.densityScale = scatterData.density;
			meshConsts.stepSize = scatterData.stepSize;

			DynamicBuffer_t cbuf = CreateDynamicConstantBuffer(&meshConsts, sizeof(meshConsts));
			cl->BindVertexCBVs(1, 1, &cbuf);
			cl->BindPixelCBVs(1, 1, &cbuf);

			u32 stride = (u32)sizeof(float3);
			u32 offset = 0;
			cl->SetVertexBuffers(0, 1, &mesh.vbuf, &stride, &offset);
			cl->SetIndexBuffer(mesh.ibuf, RenderFormat::R16_UINT, 0);

			cl->DrawIndexedInstanced(mesh.numIndices, 1, 0, 0, 0);
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


