#pragma once

#include "RenderTypes.h"

RENDER_TYPE(VertexShader_t);
RENDER_TYPE(PixelShader_t);
RENDER_TYPE(GeometryShader_t);
RENDER_TYPE(ComputeShader_t);

struct ShaderMacro
{
	std::string _define;
	std::string _value;

	ShaderMacro() {}

	ShaderMacro(const char* define)
		: _define(define)
		, _value("1")
	{}

	ShaderMacro(const char* define, const char* value)
		: _define(define)
		, _value(value)
	{}
};
typedef std::vector<ShaderMacro> ShaderMacros;

VertexShader_t		CreateVertexShader(const char* path, const ShaderMacros& macros = {});
PixelShader_t		CreatePixelShader(const char* path, const ShaderMacros& macros = {});
GeometryShader_t	CreateGeometryShader(const char* path, const ShaderMacros& macros = {});
ComputeShader_t		CreateComputeShader(const char* path, const ShaderMacros& macros = {});

void ReloadShaders();