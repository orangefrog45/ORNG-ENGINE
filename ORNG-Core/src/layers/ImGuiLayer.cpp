#include "pch/pch.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template" // ImPlot causes this but it's harmless here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <implot.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "core/Window.h"
#include "core/FrameTiming.h"
#include "core/Input.h"
#include "layers/ImGuiLayer.h"
#include "rendering/Renderer.h"
#include "util/Timers.h"
#include "util/ExtraUI.h"

namespace ORNG {
	void ImGuiLayer::OnInit() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		ImGui_ImplGlfw_InitForOpenGL(Window::GetGLFWwindow(), true);
		ImGui_ImplOpenGL3_Init(reinterpret_cast<const char*>(glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS)));
	}

	void ImGuiLayer::Update() {
		ExtraUI::OnUpdate();
		if (Window::Get().input.IsKeyDown(Key::LeftControl) && Window::Get().input.IsKeyPressed('u')) {
			m_render_debug = !m_render_debug;
		}
	}

	void ImGuiLayer::BeginFrame() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::RenderProfilingTimers() {
		static bool display_profiling_timers = ProfilingTimers::AreTimersEnabled();

		if (ImGui::Checkbox("Timers", &display_profiling_timers)) {
			ProfilingTimers::SetTimersEnabled(display_profiling_timers);
		}
		if (ProfilingTimers::AreTimersEnabled()) {
			for (auto& string : ProfilingTimers::GetTimerData()) {
				ImGui::PushID(&string);
				ImGui::Text("%s", string.c_str());
				ImGui::PopID();
			}
			ProfilingTimers::UpdateTimers(FrameTiming::GetTimeStepHighPrecision());
		}
	}

	void ImGuiLayer::RenderDebug() {
		if (ImGui::Begin("Debug")) {
			static bool graph_paused = false;
			static std::vector<float> frametimes(1000, 0.f);
			static std::vector<float> x_data(1000, 0.f);

			if (!graph_paused) {
				static unsigned index = 0;
				frametimes[index] = FrameTiming::GetTimeStep() * 10.f;
				x_data[index] = static_cast<float>(index);

				index = (index + 1) % 1000;
			}
			ImGui::Checkbox("Paused", &graph_paused);
			ImPlot::SetNextAxesLimits(0, 1000, 0, 120);
			if (ImPlot::BeginPlot("p", ImVec2(1000, 150), ImPlotFlags_NoFrame | ImPlotFlags_NoTitle)) {
				ImPlot::SetupAxis(ImAxis_Y1);
				ImPlot::SetupAxisScale(ImAxis_Y1, 1);
				ImPlot::PlotLine("Frametime*10", x_data.data(), frametimes.data(), 1000);
				ImPlot::EndPlot();
			}

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", static_cast<double>(1000.0f / ImGui::GetIO().Framerate),
				static_cast<double>(ImGui::GetIO().Framerate));
			ImGui::Text("%s", std::format("Draw calls: {}", Renderer::GetDrawCalls()).c_str());
			RenderProfilingTimers();
		}
		ImGui::End();
	}

	void ImGuiLayer::OnImGuiRender() {
		if (m_render_debug)
			RenderDebug();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}


	void ImGuiLayer::OnRender() {
	}

	void ImGuiLayer::OnShutdown() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImPlot::DestroyContext();
		ImGui::DestroyContext();
	}
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
