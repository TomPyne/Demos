#pragma once

#include "../Shaders.h"

bool CompileVertexShader(VertexShader_t handle, const char* path, const ShaderMacros& macros);
bool CompilePixelShader(PixelShader_t handle, const char* path, const ShaderMacros& macros);
bool CompileGeometryShader(GeometryShader_t handle, const char* path, const ShaderMacros& macros);
bool CompileComputeShader(ComputeShader_t handle, const char* path, const ShaderMacros& macros);