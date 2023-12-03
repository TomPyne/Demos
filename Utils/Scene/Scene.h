#pragma once

#include "SceneNode.h"

#include <memory>
#include <vector>

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