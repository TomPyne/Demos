#include "SceneGraph.h"

#include "Render/Render.h"
#include "Utils/GltfLoader.h"
#include "Utils/Logging.h"
#include "Utils/TextureLoader.h"

struct SceneGraph
{
	SceneNodePtr rootNode;
	std::vector<SceneNodePtr> nodes;
} g_sceneGraph;

template<typename T>
T* InsertNode(const SceneNodePtr& parent = nullptr)
{
	g_sceneGraph.nodes.push_back(std::make_shared<T>{});

	g_sceneGraph.nodes.back()->parent = parent;

	if (parent)
	{
		parent->children.push_back(g_sceneGraph.nodes.back());
	}
}

SceneNodePtr& SceneGraph_RootNode()
{
	return g_sceneGraph.rootNode;
}

void SceneGraph_Render(RenderScene& renderScene)
{
	for()
}

struct GltfProcessContext
{
	std::vector<StaticMesh>* meshes;
};

struct GltfProcessor
{
	const Gltf& _gltf;
	GltfProcessContext& _context;

	std::vector<Texture_t> textures;

	explicit GltfProcessor(const Gltf& gltf, GltfProcessContext& ctx) : _gltf(gltf), _context(ctx){}

	uint32_t ProcessMesh(const matrix& worldTransform, const matrix& fromParent, const GltfMeshPrimitive& prim);
	void ProcessNode(int32_t nodeIdx, const matrix& transform);
	Texture_t ProcessTexture(const GltfTextureInfo& tex);
	Texture_t ProcessNormalTexture(const GltfNormalTextureInfo& tex);
	void ProcessScenes();	
};

Texture_t GltfProcessor::ProcessTexture(const GltfTextureInfo& texInfo)
{
	if (texInfo.index < textures.size() && textures[texInfo.index] != Texture_t::INVALID)
		return textures[texInfo.index];

	textures.resize(texInfo.index + 1, Texture_t::INVALID);

	const GltfTexture& tex = _gltf.textures[texInfo.index];
	const GltfImage& img = _gltf.images[tex.source];
	const GltfBufferView& bufView = _gltf.bufferViews[img.bufferView];

	if (img.mimeType == "image/png")
	{
		uint32_t w, h;
		textures[texInfo.index] = TextureLoader_LoadPngTextureFromMemory((_gltf.data.get() + bufView.byteOffset), bufView.byteLength, &w, &h);
	}

	return textures[texInfo.index];
}

Texture_t GltfProcessor::ProcessNormalTexture(const GltfNormalTextureInfo& texInfo)
{
	if (texInfo.index < textures.size() && textures[texInfo.index] != Texture_t::INVALID)
		return textures[texInfo.index];

	textures.resize(texInfo.index + 1, Texture_t::INVALID);

	const GltfTexture& tex = _gltf.textures[texInfo.index];
	const GltfImage& img = _gltf.images[tex.source];
	const GltfBufferView& bufView = _gltf.bufferViews[img.bufferView];

	if (img.mimeType == "image/png")
	{
		uint32_t w, h;
		textures[texInfo.index] = TextureLoader_LoadPngTextureFromMemory((_gltf.data.get() + bufView.byteOffset), bufView.byteLength, &w, &h);
	}

	return textures[texInfo.index];
}

uint32_t GltfProcessor::ProcessMesh(const matrix& worldTransform, const matrix& fromParent, const GltfMeshPrimitive& prim)
{
	uint32_t loadedMeshIdx = (uint32_t)_context.meshes->size();
	_context.meshes->push_back({});
	StaticMesh& m = _context.meshes->back();

	m.worldTransform = worldTransform;
	m.transformFromParent = fromParent;

	{
		PrimitiveTopologyType topo = PrimitiveTopologyType::Undefined;

		switch (prim.mode)
		{
		case GltfMeshMode::POINTS:
			topo = PrimitiveTopologyType::Point;
			break;
		case GltfMeshMode::LINES:
			topo = PrimitiveTopologyType::Line;
			break;
		case GltfMeshMode::TRIANGLES:
			topo = PrimitiveTopologyType::Triangle;
			break;
		default:
			LOGERROR("Unsupported mesh mode %d", (int)prim.mode);
			topo = PrimitiveTopologyType::Undefined;
		};

		const GltfMaterial& mat = _gltf.materials[prim.material];

		m.material.pipeline.blendMode = mat.alphaMode == GltfAlphaMode::BLEND ? 1 : 0;
		m.material.pipeline.doubleSided = mat.doubleSided;

		m.material.baseColorFactor = float4{ (float)mat.pbr.baseColorFactor.x, (float)mat.pbr.baseColorFactor.y,(float)mat.pbr.baseColorFactor.z,(float)mat.pbr.baseColorFactor.w };

		m.material.metallicFactor = mat.pbr.metallicFactor;
		m.material.roughnessFactor = mat.pbr.roughnessFactor;

		m.material.baseColorTexture = mat.pbr.hasBaseColorTexture ? ProcessTexture(mat.pbr.baseColorTexture) : Texture_t::INVALID;
		m.material.normalTexture = mat.hasNormalTexture ? ProcessNormalTexture(mat.normalTexture) : Texture_t::INVALID;
		m.material.metallicRoughnessTexture = mat.pbr.hasMetallicRoughnessTexture ? ProcessTexture(mat.pbr.metallicRoughnessTexture) : Texture_t::INVALID;

		m.material.alphaCutoff = mat.alphaCutoff;
		m.material.alphaMask = mat.alphaMode == GltfAlphaMode::MASK;
	}

	{
		const GltfAccessor& accessor = _gltf.accessors[prim.indices];
		const GltfBufferView& bufView = _gltf.bufferViews[accessor.bufferView];

		const size_t offset = accessor.byteOffset + bufView.byteOffset;
		m.buffers.indexBuf.buf = CreateIndexBuffer(_gltf.data.get() + offset, accessor.count * GltfLoader_SizeOfComponent(accessor.componentType) * GltfLoader_ComponentCount(accessor.type));
		m.buffers.indexBuf.count = accessor.count;
		m.buffers.indexBuf.offset = 0;
		m.buffers.indexBuf.format = GltfLoader_SizeOfComponent(accessor.componentType) == 2 ? RenderFormat::R16_UINT : RenderFormat::R32_UINT;
	}

	for (const GltfMeshAttribute& attr : prim.attributes)
	{
		BindVertexBuffer* targetBuf = nullptr;
		if (attr.semantic == "POSITION") targetBuf = &m.buffers.positionBuf;
		else if (attr.semantic == "NORMAL") targetBuf = &m.buffers.normalBuf;
		else if (attr.semantic == "TANGENT") targetBuf = &m.buffers.tangentBuf;
		else if (attr.semantic == "TEXCOORD_0") targetBuf = &m.buffers.texcoordBufs[0];
		else if (attr.semantic == "TEXCOORD_1") targetBuf = &m.buffers.texcoordBufs[1];
		else
		{
			LOGWARNING("Unsupported buffer in ProcessMesh %s", attr.semantic.c_str());
			continue;
		}

		const GltfAccessor& accessor = _gltf.accessors[attr.index];
		const GltfBufferView& bufView = _gltf.bufferViews[accessor.bufferView];
		const GltfBuffer& buf = _gltf.buffers[bufView.buffer];

		targetBuf->offset = 0;
		targetBuf->stride = (uint32_t)(GltfLoader_SizeOfComponent(accessor.componentType) * GltfLoader_ComponentCount(accessor.type));
		targetBuf->buf = CreateVertexBuffer(_gltf.data.get() + accessor.byteOffset + bufView.byteOffset, accessor.count * targetBuf->stride);
	}

	return loadedMeshIdx;
}

void GltfProcessor::ProcessNode(int32_t nodeIdx, const matrix& transform)
{
	const GltfNode& node = _gltf.nodes[nodeIdx];

	const matrix transformFromParent = matrix((float)node.matrix.m[0], (float)node.matrix.m[4], (float)node.matrix.m[8], (float)node.matrix.m[12],
		(float)node.matrix.m[1], (float)node.matrix.m[5], (float)node.matrix.m[9], (float)node.matrix.m[13],
		(float)node.matrix.m[2], (float)node.matrix.m[6], (float)node.matrix.m[10], (float)node.matrix.m[14],
		(float)node.matrix.m[3], (float)node.matrix.m[7], (float)node.matrix.m[11], (float)node.matrix.m[15]);

	const matrix worldTransform = transform * transformFromParent;

	if (node.mesh >= 0)
	{
		const GltfMesh& mesh = _gltf.meshes[node.mesh];
		for (const GltfMeshPrimitive& prim : mesh.primitives)
			ProcessMesh(worldTransform, transformFromParent, prim);
	}

	for (const uint32_t child : node.children)
		ProcessNode(child, worldTransform);
}

void GltfProcessor::ProcessScenes()
{
	for (const GltfScene& scene : _gltf.scenes)
	{
		for (const uint32_t nodeIdx : scene.nodes)
		{
			ProcessNode(nodeIdx, MakeMatrixIdentity());
		}
	}
}

SceneNodePtr StaticModelNode::CreateFromGltf(const char* path)
{
	Gltf gltfModel;
	if (!GltfLoader_Load(path, &gltfModel))
		return nullptr;

	StaticModelNode* node = InsertNode<StaticModelNode>();

	GltfProcessContext context;
	context.meshes = &node->meshes;

	GltfProcessor processor{ gltfModel, context };
	processor.ProcessScenes();
}
