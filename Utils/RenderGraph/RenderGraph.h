#pragma once

#include "Render/Render.h"
#include "Utils/Flags.h"
#include "Utils/SurfMath.h"
#include <functional>
#include <map>
#include <string>
#include <string_view>

enum class RenderPassType : u8
{
	GRAPHICS,
	COMPUTE,
};

enum class RenderPassResourceAccess : u32
{
	NONE = 0,
	READ = (1u << 0u),
	WRITE = (1u << 1u),
	READ_WRITE = READ | WRITE,
};
IMPLEMENT_FLAGS(RenderPassResourceAccess, u32)

enum class RenderPassOutputAccess : u8
{
	DONT_CARE,
	LOAD,
	CLEAR,
};

enum class RenderGraphResourceType : u8
{
	NONE,
	TEXTURE,
	BUFFER,
};

enum class RenderGraphResource_t : u32 { NONE };

struct RenderPassResource
{
	RenderGraphResource_t _resourceHandle;	
	RenderPassResourceAccess _access;
	RenderResourceFlags _accessFlags;

	RenderPassResource(RenderGraphResource_t resHandle, RenderPassResourceAccess access, RenderResourceFlags flags)
		: _resourceHandle(resHandle)
		, _access(access)
		, _accessFlags(flags)
	{}

	inline constexpr bool operator==(const RenderPassResource& other) { return other._resourceHandle == _resourceHandle; }
};

struct RenderGraph;
//typedef void (*RenderGraphCallback_Func)(RenderGraph&, CommandList* cl);
using RenderGraphCallback_Func = std::function<void(RenderGraph&, CommandList* cl)>;

struct RenderPass
{
	std::string _name;
	RenderPassType _type;

	std::vector<RenderPassResource> _resources;

	bool _root = false;
	RenderGraphCallback_Func _function = nullptr;

	void AssertResourceUnique(RenderGraphResource_t res);

	RenderPass(const std::string& name, RenderPassType type)
		: _name(name)
		, _type(type)
	{}

public:
	static RenderPass Make(const std::string& name, RenderPassType type) { return RenderPass{ name, type }; }

	RenderPass& SetExecuteCallback(RenderGraphCallback_Func&& func);
	RenderPass& AddRenderTarget(RenderGraphResource_t resource, RenderPassOutputAccess access);
	RenderPass& AddDepthTarget(RenderGraphResource_t resource, RenderPassOutputAccess access);
	RenderPass& AddComputeTarget(RenderGraphResource_t resource, RenderPassOutputAccess access);
	RenderPass& AddResource(RenderGraphResource_t resource, RenderPassOutputAccess access, RenderResourceFlags flags);
	RenderPass& ReadResource(RenderGraphResource_t tex);
	RenderPass& MakeRoot() { _root = true; return *this; }
};

struct RenderGraphTextureDesc
{
	u32 width = 0;
	u32 height = 0;
	RenderFormat format = RenderFormat::UNKNOWN;
};

struct RenderGraph
{
	RenderGraphResource_t RegisterTexture(const std::string& name, const RenderGraphTextureDesc& desc);
	RenderGraphResource_t AddExternalRTV(const std::string& name, RenderTargetView_t rtv, u32 width, u32 height);
	RenderPass& AddPass(const std::string& name, RenderPassType type);

	RenderGraphResource_t GetResource(const std::string& name);

	Texture_t GetTexture(RenderGraphResource_t resource);
	ShaderResourceView_t GetSRV(RenderGraphResource_t resource);
	RenderTargetView_t GetRTV(RenderGraphResource_t resource);
	DepthStencilView_t GetDSV(RenderGraphResource_t resource);
	UnorderedAccessView_t GetUAV(RenderGraphResource_t resource);
	uint3 GetResourceDimensions(RenderGraphResource_t resource);

	void Build();

	void Execute();

	~RenderGraph();

private:
	RenderView* _view = nullptr;

	std::vector<RenderPass> _passes;
	std::vector<RenderPass*> _consolidatedPasses;
	std::vector<uint64_t> _affinityMasks;

	struct RenderGraphRegisteredResource
	{
		bool external = false;
		RenderResourceFlags flags = RenderResourceFlags::None;
		RenderGraphResourceType type = RenderGraphResourceType::NONE;

		union
		{
			struct
			{
				u32 width = 0;
				u32 height = 0;
				RenderFormat format = RenderFormat::UNKNOWN;
			} texture;
			struct
			{
				u32 size = 0;
			} buffer;
		};
	};

	std::map<std::string, RenderGraphResource_t> _registeredResourceMap;
	std::vector<RenderGraphRegisteredResource> _registeredResources;

	struct RenderGraphResource
	{
		bool external = false;
		RenderGraphResourceType type = RenderGraphResourceType::NONE;
		ShaderResourceView_t srv = ShaderResourceView_t::INVALID;
		RenderTargetView_t rtv = RenderTargetView_t::INVALID;
		DepthStencilView_t dsv = DepthStencilView_t::INVALID;
		UnorderedAccessView_t uav = UnorderedAccessView_t::INVALID;
		union
		{
			struct
			{
				Texture_t tex = Texture_t::INVALID;
				uint3 dimensions = {};
			} texture;
		};
	};

	std::map<std::string, RenderGraphResource_t> _consolidatedResourceMap;
	std::vector<RenderGraphResource> _resources;
};