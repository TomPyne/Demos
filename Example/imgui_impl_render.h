// dear imgui: Renderer for Icarus
// This needs to be used along with a Platform Binding (e.g. Win32)

#pragma once

struct CommandList;

IMGUI_IMPL_API bool		ImGui_ImplRender_Init();
IMGUI_IMPL_API void     ImGui_ImplRender_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplRender_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplRender_RenderDrawData(ImDrawData* draw_data, CommandList* cl);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_IMPL_API void     ImGui_ImplRender_InvalidateDeviceObjects();
IMGUI_IMPL_API bool     ImGui_ImplRender_CreateDeviceObjects();