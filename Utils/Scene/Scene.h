#pragma once

#include "SceneNode.h"

#include <memory>
#include <vector>

struct RenderNode
{
	// buffers
	// material
	// transform
	// aabb
};

struct SceneRenderable
{

};

struct SceneDrawList
{

};

struct Scene
{
	std::vector<std::unique_ptr<SceneNode>> _sceneNodes;
};