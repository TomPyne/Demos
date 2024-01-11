#pragma once

#include "SunTemple/Model/ModelBuffers.h"
#include "SunTemple/Model/ModelMaterials.h"

#include <memory>
#include <vector>

struct SceneNode;

using SceneNodePtr = std::shared_ptr<SceneNode>;

struct RenderBatch
{
	ConstantBuffer_t vertexConstants;
	ConstantBuffer_t pixelConstants;
	Texture_t pixelTextures[8];
	uint32_t numPixelTextures;

	BindVertexBuffer vertexBuffers[8];
	uint32_t numVertexBuffers;

	BindIndexBuffer indexBuffer;
};

enum class RenderQueueType : uint32_t
{
	OPAQUE_QUEUE,
	TRANSPARENT_QUEUE,
	COUNT
};

struct RenderQueue
{
	std::vector<RenderBatch> batches;
};

struct RenderScene
{
	RenderQueue queues[(uint32_t)RenderQueueType::COUNT];
};

class SceneNode
{
public:
	SceneNodePtr parent = nullptr;
	std::vector<SceneNodePtr> children;

	matrix transform;

	virtual void Render(RenderScene& scene) {}
};

struct StaticMesh
{	
	matrix worldTransform;
	matrix transformFromParent;
	ModelBuffers buffers;
	MaterialInstance material;

	AABB aabb;
};

class StaticModelNode : public SceneNode
{
public:
	static SceneNodePtr CreateFromGltf(const char* path);

	std::vector<StaticMesh> meshes;

	virtual void Render(RenderScene& scene) override;
};

SceneNodePtr& SceneGraph_RootNode();

void SceneGraph_Render(RenderScene& renderScene);