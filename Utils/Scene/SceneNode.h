#pragma once

#include "Utils/SurfMath.h"
#include <vector>

class SceneNode
{
	SceneNode* parent = nullptr;
	std::vector<SceneNode*> children;

public:



};