#pragma once

#include "../PipelineState.h"

bool CompileGraphicsPipelineState(GraphicsPipelineState_t handle, const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs, size_t inputCount);
bool CompileComputePipelineState(ComputePipelineState_t handle, const ComputePipelineStateDesc& desc);

void DestroyGraphicsPipelineState(GraphicsPipelineState_t pso);
void DestroyComputePipelineState(ComputePipelineState_t pso);