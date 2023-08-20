#include "CommonIncludes.h"

#include "DebugUI.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "render/texture/Texture.h"
#include "render/buffer/VulkanBuffer.h";
#include "render/buffer/DynamicBuffer.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanDescriptorCache.h"
#include "render/device/Types.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderCache.h"
#include "render/shader/ShaderDefines.h"
#include "widgets/EngineStats.h"
#include "render/renderer/EnvironmentSettings.h"
#include "render/debug/DebugSettings.h"

namespace render { 
	
	namespace DebugUI
	{
		using namespace Device;

		namespace 
		{
			std::unique_ptr<Texture> font_texture;
			std::unique_ptr<DynamicBuffer<char>> vertex_buffers[2];
			std::unique_ptr<DynamicBuffer<char>> index_buffers[2];
			uint32_t current_buffer = 0;
			ShaderProgram* shader = nullptr;
			DescriptorSet descriptor_set;
			VertexLayout vertex_layout;
			bool engine_stats_visible = true;
			bool main_widget_visible = true;
			bool environment_widget_visible = true;
			uint32_t engine_stats_index = 0;

			EnvironmentSettings* environment_settings = nullptr;
			DebugSettings* debug_settings = nullptr;

			typedef void (*engine_stats_callback)(void);
			std::array<engine_stats_callback, 2> engine_stats_functions;
		}

		void DrawMainWidget(DebugSettings* debug_settings);
		void DrawEnvironmentEditorWidget(EnvironmentSettings& settings);

		void SetEngineStatsVisible(bool visible)
		{
			engine_stats_visible = visible;
		}

		void SetMainWidgetVisible(bool visible)
		{
			main_widget_visible = visible;
		}

		void SetEnvironmentWidgetVisible(bool visible)
		{
			environment_widget_visible = visible;
		}

		bool WantCaptureMouse()
		{
			return ImGui::GetIO().WantCaptureMouse;
		}

		bool WantCaptureKeyboard()
		{
			return ImGui::GetIO().WantCaptureKeyboard;
		}

		//

		void Initialize(GLFWwindow* window, ShaderCache* shader_cache, EnvironmentSettings& _environment_settings, DebugSettings& _debug_settings)
		{
			environment_settings = &_environment_settings;
			debug_settings = &_debug_settings;
			auto shader_info = ShaderProgramInfo()
				.AddShader(ShaderProgram::Stage::Vertex, L"shaders/imgui.hlsl", "vs_main")
				.AddShader(ShaderProgram::Stage::Fragment, L"shaders/imgui.hlsl", "ps_main");
			shader = shader_cache->GetShaderProgram(shader_info);

			ImGui::CreateContext();
			ImGui_ImplGlfw_InitForVulkan(window, true);

			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontDefault();

			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

			TextureInitializer initializer(width, height, 4, pixels, false);
			font_texture = std::make_unique<Texture>(initializer);

			vertex_buffers[0] = std::make_unique<DynamicBuffer<char>>("IMGUI", 256, BufferType::Vertex, false);
			index_buffers[0] = std::make_unique<DynamicBuffer<char>>("IMGUI", 256, BufferType::Index, false);

			ResourceBindings resource_bindings;
			resource_bindings.AddTextureBinding("font_texture", font_texture.get());
			auto descriptor_cache = Engine::GetVulkanContext()->GetDescriptorCache();
			descriptor_set = descriptor_cache->GetDescriptorSet(DescriptorSetBindings(resource_bindings, *shader->GetDescriptorSetLayout(0)));

			vertex_layout = VertexLayout();
			vertex_layout.AddAttrib(VertexAttrib::Position, Format::R32G32_float, sizeof(vec2));
			vertex_layout.AddAttrib(VertexAttrib::TexCoord0, Format::R32G32_float, sizeof(vec2));
			vertex_layout.AddAttrib(VertexAttrib::VertexColor, Format::R8G8B8A8_unorm, sizeof(uint32_t));

			uint32_t current_index = 0;
			engine_stats_functions[current_index++] = EngineStatsMemory;
			engine_stats_functions[current_index++] = EngineStatsProfiler;
		}

		void NewFrame()
		{
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		template <typename T>
		void ResizeBuffer(std::unique_ptr<DynamicBuffer<T>> buffer[2], size_t size)
		{
			if (buffer[0]->GetSize() < size)
			{
				auto type = buffer[0]->GetType();
				buffer[1] = std::move(buffer[0]);
				buffer[0] = std::make_unique<DynamicBuffer<T>>("IMGUI", size, type, false);
			}
		}

		void UpdateBuffers()
		{
			auto* draw_data = ImGui::GetDrawData();
			size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
			size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
			ResizeBuffer(vertex_buffers, vertex_size);
			ResizeBuffer(index_buffers, index_size);

			// Upload vertex/index data into a single contiguous GPU buffer
			if (vertex_size && index_size)
			{
				auto* vtx_dst = (ImDrawVert*)vertex_buffers[0]->Map();
				auto* idx_dst = (ImDrawIdx*)index_buffers[0]->Map();
				
				for (int n = 0; n < draw_data->CmdListsCount; n++)
				{
					const ImDrawList* cmd_list = draw_data->CmdLists[n];
					memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
					memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
					vtx_dst += cmd_list->VtxBuffer.Size;
					idx_dst += cmd_list->IdxBuffer.Size;
				}

				vertex_buffers[0]->SetUploadSize(vertex_size);
				index_buffers[0]->SetUploadSize(index_size);
				vertex_buffers[0]->Unmap();
				index_buffers[0]->Unmap();
			}
		}

		void Update(float dt)
		{
			if (engine_stats_visible && engine_stats_index != -1)
			{
				(*engine_stats_functions[engine_stats_index])();
			}

			if (main_widget_visible)
				DrawMainWidget(debug_settings);

			if (environment_widget_visible && environment_settings)
				DrawEnvironmentEditorWidget(*environment_settings);

			ImGui::Render();
			UpdateBuffers();
		}

		void SwitchEngineStats()
		{
			engine_stats_index = (engine_stats_index + 1) % engine_stats_functions.size();
		}

		void SetupRenderState(ImDrawData* draw_data, VulkanRenderState& state)
		{
			//state.SetShader(*shader);

			//RenderMode mode;
			//mode.SetDepthTestEnabled(false);
			//mode.SetDepthWriteEnabled(false);
			//mode.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
			//mode.SetPolygonMode(PolygonMode::Fill);
			//mode.SetCullMode(CullMode::None);
			//
			//mode.SetAlphaBlendEnabled(true);
			//mode.SetSrcBlend(BlendFactor::SrcAlpha);
			//mode.SetDestBlend(BlendFactor::OneMinusSrcAlpha);
			//mode.SetBlend(BlendOp::Add);

			//mode.SetSrcBlendAlpha(BlendFactor::OneMinusSrcAlpha);
			//mode.SetDestBlendAlpha(BlendFactor::Zero);
			//mode.SetBlendAlpha(BlendOp::Add);

			//state.SetRenderMode(mode);
			//state.SetVertexLayout(vertex_layout);
			//state.RemoveGlobalBindings();

			//state.UpdateState();

			//auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();
			//float scale[2];
			//scale[0] = 2.0f / draw_data->DisplaySize.x;
			//scale[1] = 2.0f / draw_data->DisplaySize.y;
			//float translate[2];
			//translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
			//translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];

			//state.PushConstants(ShaderProgram::Stage::Vertex, sizeof(float) * 0, sizeof(float) * 2, scale);
			//state.PushConstants(ShaderProgram::Stage::Vertex, sizeof(float) * 2, sizeof(float) * 2, translate);
			//state.SetDescriptorSet(*descriptor_set, 0, 0, nullptr);
		}

		void Render(VulkanRenderState& state)
		{
			//auto* draw_data = ImGui::GetDrawData();
			//float fb_width = state.GetViewport().z;
			//float fb_height = state.GetViewport().w;

			//SetupRenderState(draw_data, state);
			//
			//// Will project scissor/clipping rectangles into framebuffer space
			//ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
			//ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

			//												 // Render command lists
			//												 // (Because we merged all buffers into a single one, we maintain our own offset into them)
			//int global_vtx_offset = 0;
			//int global_idx_offset = 0;
			//for (int n = 0; n < draw_data->CmdListsCount; n++)
			//{
			//	const ImDrawList* cmd_list = draw_data->CmdLists[n];
			//	for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			//	{
			//		const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			//		if (pcmd->UserCallback != NULL)
			//		{
			//			// User callback, registered via ImDrawList::AddCallback()
			//			// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
			//			if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
			//				SetupRenderState(draw_data, state);
			//			else
			//				pcmd->UserCallback(cmd_list, pcmd);
			//		}
			//		else
			//		{
			//			// Project scissor/clipping rectangles into framebuffer space
			//			ImVec4 clip_rect;
			//			clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
			//			clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
			//			clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
			//			clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

			//			if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
			//			{
			//				// Negative offsets are illegal for vkCmdSetScissor
			//				if (clip_rect.x < 0.0f)
			//					clip_rect.x = 0.0f;
			//				if (clip_rect.y < 0.0f)
			//					clip_rect.y = 0.0f;

			//				// Apply scissor/clipping rectangle
			//				vec4 scissor;
			//				scissor.x = (int32_t)(clip_rect.x);
			//				scissor.y = (int32_t)(clip_rect.y);
			//				scissor.z = (uint32_t)(clip_rect.z - clip_rect.x);
			//				scissor.w = (uint32_t)(clip_rect.w - clip_rect.y);
			//				if (scissor.z == 0 || scissor.w == 0)
			//					continue;

			//				state.SetScissor(scissor);

			//				// Draw
			//				auto index_type = sizeof(ImDrawIdx) == 2 ? IndexType::UINT16 : IndexType::UINT32;
			//				state.DrawIndexed(*vertex_buffers[0]->GetBuffer(), *index_buffers[0]->GetBuffer(), pcmd->VtxOffset + global_vtx_offset, pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, index_type);
			//			}
			//		}
			//	}
			//	global_idx_offset += cmd_list->IdxBuffer.Size;
			//	global_vtx_offset += cmd_list->VtxBuffer.Size;
			//}
		}

		void Deinitialize()
		{
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			font_texture = nullptr;
			vertex_buffers[0] = nullptr;
			vertex_buffers[1] = nullptr;
			index_buffers[0] = nullptr;
			index_buffers[1] = nullptr;
		}

	}

}