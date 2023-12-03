#include "FlyCamera.h"
#include "ThirdParty/imgui/imgui.h"
#include "Utils/KeyCodes.h"

void FlyCamera::SetView(const float3& position, float pitch, float yaw)
{
	_position = position;

	if (yaw > 360.0f)
		yaw -= 360.0f;

	if (yaw < -360.0f)
		yaw += 360.0f;

	_camPitch = pitch;
	_camYaw = yaw;

	pitch = Clamp(pitch, -89.9f, 89.9f);

	yaw = ConvertToRadians(yaw);
	pitch = ConvertToRadians(pitch);

	float cosPitch = cosf(pitch);

	_lookDir = float3{ cosf(yaw) * cosPitch, sinf(pitch), sinf(yaw) * cosPitch };

	_view = MakeMatrixLookToLH(position, _lookDir, float3{ 0, 1, 0 });
}

void FlyCamera::UpdateView(float delta)
{
	ImGuiIO& io = ImGui::GetIO();

	float camPitch = _camPitch;
	float camYaw = _camYaw;

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
		float3 fwd = _lookDir;
		float3 rgt = CrossF3(float3{ 0, 1, 0 }, _lookDir);

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

	SetView(_position + translation, camPitch, camYaw);
}
