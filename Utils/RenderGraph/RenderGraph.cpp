#include "RenderGraph.h"

#include "Utils/SurfMath.h"
#include "Utils/Logging.h"

#include <algorithm>
#include <array>
#include <bitset>

#define RG_VALIDATION 1

constexpr size_t RG_MAX_RESOURCES = 1024u;
using RGResourceBits = std::bitset<RG_MAX_RESOURCES>;

struct RGCachedTexture
{
	RenderFormat format;
	u32 width;
	u32 height;
	Texture_t textureHandle;
};

std::vector<RGCachedTexture> g_cachedTextures;

static Texture_t FindOrCreateTexture(RenderFormat format, u32 width, u32 height, RenderResourceFlags flags)
{
	u32 idx = 0;
	for( ; idx < g_cachedTextures.size(); idx++ )
	{
		if (g_cachedTextures[idx].format == format && g_cachedTextures[idx].width == width && g_cachedTextures[idx].height == height)
		{
			break;
		}
	}

	if (idx < g_cachedTextures.size())
	{
		Texture_t found = g_cachedTextures[idx].textureHandle;

		g_cachedTextures.erase(g_cachedTextures.begin() + idx);

		// Ensures we have all the views we need
		if (Textures_CreateViewsForResourceFlags(found, flags))
			return found;

#if RG_VALIDATION
		ASSERTMSG(0, "FindOrCreateTexture failed to create views for found texture");
#endif

		return Texture_t::INVALID;
	}
	else
	{
		TextureCreateDesc texDesc = {};
		texDesc.width = width;
		texDesc.height = height;
		texDesc.format = format;
		texDesc.flags = flags;

		Texture_t created = CreateTexture(texDesc);

#if RG_VALIDATION
		ASSERTMSG(created != Texture_t::INVALID, "FindOrCreateTexture failed to create texture");
#endif

		return created;
	}
}

RenderPass& RenderPass::SetExecuteCallback(RenderGraphCallback_Func&& func)
{
	_function = func;

	return *this;
}

void RenderPass::AssertResourceUnique(RenderGraphResource_t res)
{
	ASSERTMSG(std::find_if(_resources.begin(), _resources.end(), [res](const RenderPassResource& a) {return a._resourceHandle == res; }) == _resources.end(),
		"RenderPass::AssertResourceUnique failed, adding the same resource twice");
}

RenderPass& RenderPass::AddResource(RenderGraphResource_t resource, RenderPassOutputAccess access, RenderResourceFlags flags)
{
	AssertResourceUnique(resource);

	RenderPassResourceAccess ra = RenderPassResourceAccess::WRITE;

	if (access == RenderPassOutputAccess::LOAD)
		ra |= RenderPassResourceAccess::READ;

	_resources.emplace_back(resource, ra, flags);

	return *this;
}

RenderPass& RenderPass::AddRenderTarget(RenderGraphResource_t resource, RenderPassOutputAccess access)
{
	return AddResource(resource, access, RenderResourceFlags::RTV);
}

RenderPass& RenderPass::AddDepthTarget(RenderGraphResource_t resource, RenderPassOutputAccess access)
{
	return AddResource(resource, access, RenderResourceFlags::DSV);
}

RenderPass& RenderPass::AddComputeTarget(RenderGraphResource_t resource, RenderPassOutputAccess access)
{
	return AddResource(resource, access, RenderResourceFlags::UAV);
}

RenderPass& RenderPass::ReadResource(RenderGraphResource_t resource)
{
	AssertResourceUnique(resource);

	_resources.emplace_back(resource, RenderPassResourceAccess::READ, RenderResourceFlags::SRV);

	return *this;
}

RenderGraphResource_t RenderGraph::RegisterTexture(const std::string& name, const RenderGraphTextureDesc& desc)
{
	if (GetResource(name) != RenderGraphResource_t::NONE)
	{
		LOGERROR("RenderGraph::RegisterTexture: %s is already registered", name.c_str());
		return RenderGraphResource_t::NONE;
	}

	const RenderGraphResource_t handle = (RenderGraphResource_t)_registeredResources.size();
	_registeredResourceMap[name] = handle;

	_registeredResources.push_back({});

	RenderGraphRegisteredResource& res = _registeredResources.back();

	res.flags = RenderResourceFlags::None;
	res.type = RenderGraphResourceType::TEXTURE;

	res.texture.format = desc.format;
	res.texture.width = desc.width;
	res.texture.height = desc.height;

	return handle;
}

RenderGraphResource_t RenderGraph::AddExternalRTV(const std::string& name, RenderTargetView_t rtv, u32 width, u32 height)
{
	if (GetResource(name) != RenderGraphResource_t::NONE)
	{
		LOGERROR("RenderGraph::AddExternalRTV: %s is already registered", name.c_str());
		return RenderGraphResource_t::NONE;
	}

	const RenderGraphResource_t handle = (RenderGraphResource_t)_registeredResources.size();
	_registeredResourceMap[name] = handle;

	_registeredResources.push_back({});

	_registeredResources.back().external = true;

	// Insert the rtv into the finalised resource array
	if ((size_t)handle >= _resources.size())
	{
		_resources.resize((size_t)handle + 1, {});
		_resources.back().rtv = rtv;
		_resources.back().texture.dimensions.x = width;
		_resources.back().texture.dimensions.y = height;
	}

	return handle;
}

RenderPass& RenderGraph::AddPass(const std::string& name, RenderPassType type)
{
	_passes.emplace_back( RenderPass::Make(name, type) );
	return _passes.back();
}

RenderGraphResource_t RenderGraph::GetResource(const std::string& name)
{
	auto it = _registeredResourceMap.find(name);
	return it != _registeredResourceMap.end() ? it->second : RenderGraphResource_t::NONE;
}

struct RGNode
{
	RGResourceBits reads = 0;
	RGResourceBits writes = 0;
	uint64_t rootAffinity = 0;
	bool root = false;
	bool contributes = false;
	char const* name = nullptr;
};

void RenderGraph::Build()
{
	std::vector<RGNode> nodes;
	std::array<RenderGraphResource_t, RG_MAX_RESOURCES> uniqueResources;
	size_t numResources = 0;

	for (RenderPass& pass : _passes)
	{
		nodes.push_back({});
		RGNode& node = nodes.back();

		for (const RenderPassResource& res : pass._resources)
		{
			size_t resourceIdx = 0;
			for (; resourceIdx < numResources; resourceIdx++)
			{
				if (uniqueResources[resourceIdx] == res._resourceHandle)
					break;
			}

			if (resourceIdx == numResources)
			{
				uniqueResources[resourceIdx] = res._resourceHandle;
				numResources++;

				assert(numResources < RG_MAX_RESOURCES);
			}

			node.reads.set(resourceIdx, (res._access & RenderPassResourceAccess::READ) != RenderPassResourceAccess::NONE);
			node.writes.set(resourceIdx, (res._access & RenderPassResourceAccess::WRITE) != RenderPassResourceAccess::NONE);

			node.root = pass._root;

			node.name = pass._name.c_str();
		}
	}

	// Loop back from root nodes and track where any contributing writes are sourced from, skipping
	// passes that dont contribute.
	size_t rootCount = 0;
	{
		RGResourceBits reads;
		
		for (auto nodeIt = nodes.rbegin(); nodeIt != nodes.rend(); nodeIt++)
		{
			bool writesAreRead = (nodeIt->writes & reads).any();

			if (!nodeIt->root && !writesAreRead)
				continue;

			// Any traced writes are removed, any new reads from this pass are added
			reads = (reads & (~nodeIt->writes)) | nodeIt->reads;

			nodeIt->contributes = true;

			// If this root node is read by another root then it loses its status.
			if (nodeIt->root && writesAreRead)
				nodeIt->root = false;

			if (nodeIt->root)
				rootCount++;
		}
	}

	{
		std::vector<RGResourceBits> rootReads(rootCount);
		std::vector<RGResourceBits> rootWrites(rootCount);

		{
			// Test for isolated trees

			uint64_t rootIndex = 0;

			for (auto nodeIt = nodes.rbegin(); nodeIt != nodes.rend(); nodeIt++)
			{
				while (nodeIt != nodes.rend() && !nodeIt->root)
					nodeIt++;

				if (nodeIt == nodes.rend())
					break;

				RGResourceBits& read = rootReads[rootIndex];
				RGResourceBits& write = rootWrites[rootIndex];

				read = nodeIt->reads;
				write = nodeIt->writes;

				nodeIt->rootAffinity |= (1ull << rootIndex);

				for (auto n = nodeIt + 1; n != nodes.rend(); n++)
				{
					if (n->root)
						continue;

					// If we write to subsequent reads, add to current tree.
					if ((n->writes & read).any())
					{
						read |= n->reads;
						write |= n->writes;

						n->rootAffinity |= (1ull << rootIndex);
					}
				}

				rootIndex++;
			}
		}

		// Test isolated graphs for overlapping resource accesses and combine if detected.
		{
			std::vector<uint64_t> subgraphId(rootCount);
			std::vector<int> subgraphIdIdx(rootCount);

			for (size_t i = 0; i < rootCount; i++)
			{
				subgraphId[i] |= (1ull << i);
				subgraphIdIdx[i] = (int)i;
			}

			for (size_t i = 0; i < rootCount; i++)
			{
				for (size_t j = i + 1; j < rootCount; j++)
				{
					// if i reads from j or j reads from i
					if (!(rootReads[i] & rootWrites[j]).any() && !(rootReads[j] & rootWrites[i]).any())
						continue;

					uint64_t combinedId = subgraphId[subgraphIdIdx[j]] | subgraphId[subgraphIdIdx[i]];

					if (subgraphIdIdx[i] <= subgraphIdIdx[j])
						subgraphIdIdx[j] = subgraphIdIdx[i];
					else
						subgraphIdIdx[i] = subgraphIdIdx[j];

					subgraphId[subgraphIdIdx[i]] = combinedId;
				}
			}

			subgraphIdIdx.erase(std::unique(subgraphIdIdx.begin(), subgraphIdIdx.end()), subgraphIdIdx.end());

			// Consolidate and test queues dont overlap.

			uint64_t checkSubgraph = 0;
			for (size_t i = 0; i < subgraphIdIdx.size(); i++)
			{
				_affinityMasks.push_back(subgraphId[subgraphIdIdx[i]]);

				uint64_t q = subgraphId[subgraphIdIdx[i]];
				assert(!(q& checkSubgraph)); // Subgraph isn't independent.
				checkSubgraph |= q;
			}
		}
	}

	// Remove non-contributing passes
	{
		size_t numPasses = _passes.size();

		_consolidatedPasses.reserve(numPasses);

		for (size_t i = 0; i < numPasses; i++)
		{
			if (nodes[i].contributes)
				_consolidatedPasses.push_back(&_passes[i]);
		}
	}

	// Build textures
	{
		//  Assign access flags
		{
			for (const RenderPass* rp : _consolidatedPasses)
			{
				for (const RenderPassResource& res : rp->_resources)
				{
					_registeredResources[(size_t)res._resourceHandle].flags |= res._accessFlags;
				}
			}
		}

		std::vector<RenderGraphResource_t> usedResources;
		for (const RenderPass* rp : _consolidatedPasses)
		{
			for (const RenderPassResource& res : rp->_resources)
			{
				usedResources.push_back(res._resourceHandle);
			}
		}

		usedResources.erase(std::unique(usedResources.begin(), usedResources.end()), usedResources.end());

		for (RenderGraphResource_t handle : usedResources)
		{
			if ((size_t)handle >= _resources.size())
			{
				_resources.resize((size_t)handle + 1, {});
			}

			RenderGraphRegisteredResource registeredRes = _registeredResources[(size_t)handle];
			RenderGraphResource& res = _resources[(size_t)handle];

			if (res.external)
				continue;

			res.type = registeredRes.type;

			if (registeredRes.type == RenderGraphResourceType::TEXTURE)
			{
				res.texture.tex = FindOrCreateTexture(registeredRes.texture.format, registeredRes.texture.width, registeredRes.texture.height, registeredRes.flags);
				res.texture.dimensions = uint3(registeredRes.texture.width, registeredRes.texture.height, 1);
				res.srv = GetTextureSRV(res.texture.tex);
				res.rtv = GetTextureRTV(res.texture.tex);
				res.dsv = GetTextureDSV(res.texture.tex);
				res.uav = GetTextureUAV(res.texture.tex);
			}
		}

		// Clear cached textures
		for (const auto& tex : g_cachedTextures)
		{
			Render_Release(tex.textureHandle);
		}

		g_cachedTextures.clear();
	}
}

void RenderGraph::Execute()
{
	CommandListPtr cl = CommandList::Create();

	for (const RenderPass* rp : _consolidatedPasses)
	{
		// If we require end barriers, we execute them here.

		// Execute callback
		rp->_function(*this, cl.get());

		// Insert begin barriers
	}

	CommandList::Execute(cl);
}

ShaderResourceView_t RenderGraph::GetSRV(RenderGraphResource_t resource)
{
	return _resources[(size_t)resource].srv;
}

RenderTargetView_t RenderGraph::GetRTV(RenderGraphResource_t resource)
{
	return _resources[(size_t)resource].rtv;
}

DepthStencilView_t RenderGraph::GetDSV(RenderGraphResource_t resource)
{
	return _resources[(size_t)resource].dsv;
}

UnorderedAccessView_t RenderGraph::GetUAV(RenderGraphResource_t resource)
{
	return _resources[(size_t)resource].uav;
}

uint3 RenderGraph::GetResourceDimensions(RenderGraphResource_t resource)
{
	return _resources[(size_t)resource].texture.dimensions;
}

RenderGraph::~RenderGraph()
{
	for (const RenderGraphResource& resource : _resources)
	{
		if (resource.type == RenderGraphResourceType::TEXTURE)
			Render_Release(resource.texture.tex);
	}
}

