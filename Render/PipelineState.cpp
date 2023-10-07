#include "PipelineState.h"
#include "Impl/PipelineStateImpl.h"
#include "IDArray.h"

struct GraphicsPipelineStateData
{
    GraphicsPipelineStateDesc desc;
    std::vector<InputElementDesc> inputs;
};

struct ComputePipelineStateData
{
    ComputePipelineStateDesc desc;
};

IDArray<GraphicsPipelineState_t, GraphicsPipelineStateData> g_GraphicsPipelineStates;
IDArray<ComputePipelineState_t, ComputePipelineStateData> g_ComputePipelineStates;

GraphicsPipelineState_t CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs, size_t inputCount)
{
    GraphicsPipelineState_t pso = g_GraphicsPipelineStates.Create();

    if (!CompileGraphicsPipelineState(pso, desc, inputs, inputCount))
    {
        g_GraphicsPipelineStates.Release(pso);
        return GraphicsPipelineState_t::INVALID;
    }

    GraphicsPipelineStateData* data = g_GraphicsPipelineStates.Get(pso);

    data->desc = desc;
    data->inputs.resize(inputCount);
    memcpy(data->inputs.data(), inputs, inputCount * sizeof(InputElementDesc));
    
    return pso;
}

ComputePipelineState_t CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
    ComputePipelineState_t pso = g_ComputePipelineStates.Create();

    if (!CompileComputePipelineState(pso, desc))
    {
        g_ComputePipelineStates.Release(pso);
        return ComputePipelineState_t::INVALID;
    }

    ComputePipelineStateData* data = g_ComputePipelineStates.Get(pso);

    data->desc = desc;

    return pso;
}

void Render_Release(GraphicsPipelineState_t pso)
{
    if (g_GraphicsPipelineStates.Release(pso))
        DestroyGraphicsPipelineState(pso);
}

void Render_Release(ComputePipelineState_t pso)
{
    if (g_ComputePipelineStates.Release(pso))
        DestroyComputePipelineState(pso);
}
