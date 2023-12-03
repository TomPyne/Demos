#pragma once

#include "Utils/SurfMath.h"

struct Camera
{
	void Resize(u32 w, u32 h);
	void SetNearFar(float near, float far);
	void SetFov(float fov);

	const matrix& GetProjection() const noexcept { return _projection; }

protected:
	u32 _w = 0;
	u32 _h = 0;
	float _nearZ = 0.1f;
	float _farZ = 10'000.0f;
	float _fov = 45.0f;
	float _aspectRatio = 0.0f;
	matrix _projection;
};