#include "Camera.h"

void Camera::Resize(u32 w, u32 h)
{
	if (w == _w && h == _h)
		return;

	_w = w;
	_h = h;

	_aspectRatio = (float)w / (float)h;

	_projection = MakeMatrixPerspectiveFovLH( ConvertToRadians( _fov ), _aspectRatio, _nearZ, _farZ);
}

void Camera::SetNearFar(float near, float far)
{
	if (near == _nearZ && far == _farZ)
		return;

	_nearZ = near;
	_farZ = far;

	_projection = MakeMatrixPerspectiveFovLH(ConvertToRadians(_fov), _aspectRatio, _nearZ, _farZ);
}

void Camera::SetFov(float fov)
{
	if (fov == _fov)
		return;

	_fov = fov;

	_projection = MakeMatrixPerspectiveFovLH(ConvertToRadians(_fov), _aspectRatio, _nearZ, _farZ);
}
