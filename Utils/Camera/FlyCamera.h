#pragma once

#include "Camera.h"

struct FlyCamera : public Camera
{
	void SetView(const float3& position, float pitch, float yaw);
	void UpdateView(float delta);

	const matrix& GetView() const noexcept { return _view; }
	const float3& GetPosition() const noexcept { return _position; }

protected:
	float3 _position;
	float3 _lookDir;
	float _camPitch = 0.0f;
	float _camYaw = 0.0f;
	matrix _view;
};