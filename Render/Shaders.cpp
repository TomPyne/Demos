#include "Shaders.h"

#include "IDArray.h"

#include "Impl/ShadersImpl.h"

struct ShaderData
{
	bool compiled = false;
	std::string path;
	ShaderMacros macros;
};

IDArray<VertexShader_t,		ShaderData>	g_VertexShaders;
IDArray<PixelShader_t,		ShaderData>	g_PixelShaders;
IDArray<GeometryShader_t,	ShaderData>	g_GeometryShaders;
IDArray<ComputeShader_t,	ShaderData>	g_ComputeShaders;

static void UpdateShader(ShaderData* data, const char* path, const ShaderMacros& macros)
{
	if (!data)
		return;

	data->path = path;
	data->macros = macros;

	data->compiled = true;
}

VertexShader_t CreateVertexShader(const char* path, const ShaderMacros& macros)
{
	VertexShader_t newShader = g_VertexShaders.Create();
	ShaderData* data = g_VertexShaders.Get(newShader);

	UpdateShader(data, path, macros);

	data->macros.push_back({ "_VS", "1" });

	if (!CompileVertexShader(newShader, path, data->macros))
	{
		g_VertexShaders.Release(newShader);
		return VertexShader_t::INVALID;
	}

	return newShader;
}

PixelShader_t CreatePixelShader(const char* path, const ShaderMacros& macros)
{
	PixelShader_t newShader = g_PixelShaders.Create();
	ShaderData* data = g_PixelShaders.Get(newShader);

	UpdateShader(data, path, macros);

	data->macros.push_back({ "_PS", "1" });

	if (!CompilePixelShader(newShader, path, data->macros))
	{
		g_PixelShaders.Release(newShader);
		return PixelShader_t::INVALID;
	}

	return newShader;
}

GeometryShader_t CreateGeometryShader(const char* path, const ShaderMacros& macros)
{
	GeometryShader_t newShader = g_GeometryShaders.Create();
	ShaderData* data = g_GeometryShaders.Get(newShader);

	UpdateShader(data, path, macros);

	data->macros.push_back({ "_GS", "1" });

	if (!CompileGeometryShader(newShader, path, data->macros))
	{
		g_GeometryShaders.Release(newShader);
		return GeometryShader_t::INVALID;
	}

	return newShader;
}

ComputeShader_t CreateComputeShader(const char* path, const ShaderMacros& macros)
{
	ComputeShader_t newShader = g_ComputeShaders.Create();
	ShaderData* data = g_ComputeShaders.Get(newShader);

	UpdateShader(data, path, macros);

	data->macros.push_back({ "_CS", "1" });

	if (!CompileComputeShader(newShader, path, data->macros))
	{
		g_ComputeShaders.Release(newShader);
		return ComputeShader_t::INVALID;
	}	

	return newShader;
}

void ReloadShaders()
{
	for (size_t i = 0; i < g_VertexShaders.Size(); i++)
	{
		if (ShaderData* data = g_VertexShaders.Get((VertexShader_t)i))
		{
			if (!data->compiled)
				continue;

			CompileVertexShader((VertexShader_t)i, data->path.c_str(), data->macros);				
		}
	}

	for (size_t i = 0; i < g_PixelShaders.Size(); i++)
	{
		if (ShaderData* data = g_PixelShaders.Get((PixelShader_t)i))
		{
			if (!data->compiled)
				continue;

			CompilePixelShader((PixelShader_t)i, data->path.c_str(), data->macros);
		}
	}

	for (size_t i = 0; i < g_ComputeShaders.Size(); i++)
	{
		if (ShaderData* data = g_ComputeShaders.Get((ComputeShader_t)i))
		{
			if (!data->compiled)
				continue;

			CompileComputeShader((ComputeShader_t)i, data->path.c_str(), data->macros);
		}
	}

	for (size_t i = 0; i < g_GeometryShaders.Size(); i++)
	{
		if (ShaderData* data = g_GeometryShaders.Get((GeometryShader_t)i))
		{
			if (!data->compiled)
				continue;

			CompileGeometryShader((GeometryShader_t)i, data->path.c_str(), data->macros);
		}
	}
}
