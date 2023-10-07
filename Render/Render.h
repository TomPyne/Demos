#pragma once

#define RENDER_MAJOR_VERSION 1
#define RENDER_MINOR_VERSION 0

#include "Binding.h"
#include "Buffers.h"
#include "CommandList.h"
#include "PipelineState.h"
#include "RenderTypes.h"
#include "Samplers.h"
#include "Shaders.h"
#include "Textures.h"
#include "View.h"

#define RENDER_DEBUG 1
bool Render_Init();
bool Render_Initialised();
void Render_NewFrame();
void Render_ShutDown();

void Render_PushDebugWarningDisable(RenderDebugWarnings warning);
void Render_PopDebugWarningDisable();