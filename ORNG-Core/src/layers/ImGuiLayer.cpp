#include "pch/pch.h"
#include "layers/ImGuiLayer.h"
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glfw/glfw3.h>
#include "implot.h"
#include "core/Window.h"
#include "core/FrameTiming.h"
#include "core/Input.h"
#include "rendering/Renderer.h"
#include "util/Timers.h"

namespace ORNG {
	void ImGuiLayer::OnInit() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForOpenGL(Window::GetGLFWwindow(), true);
		ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));
	}

	void ImGuiLayer::Update() {
		if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed('u')) {
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
				ImGui::Text(string.c_str());
				ImGui::PopID();
			}
			ProfilingTimers::UpdateTimers(FrameTiming::GetTimeStep());
		}
	}

	void ImGuiLayer::RenderDebug() {
		if (ImGui::Begin("Debug")) {
			static bool graph_paused = false;
			static std::vector<float> frametimes(1000, 0.f);
			static std::vector<float> x_data(1000, 0.f);

			if (!graph_paused) {
				static unsigned index = 0;
				frametimes[index] = FrameTiming::GetTimeStep() * 10.0;
				x_data[index] = index;

				index = (index + 1) % 1000;
			}
			ImGui::Checkbox("Paused", &graph_paused);
			ImPlot::SetNextAxesLimits(0, 1000, 0, 120);
			if (ImPlot::BeginPlot("p", ImVec2(1000, 150), ImPlotFlags_NoFrame | ImPlotFlags_NoTitle)) {
				ImPlot::SetupAxis(ImAxis_Y1);
				ImPlot::SetupAxisScale(ImAxis_Y1, 0.001);
				ImPlot::PlotLine("Frametime*10", x_data.data(), frametimes.data(), 1000);
				ImPlot::EndPlot();
			}

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text(std::format("Draw calls: {}", Renderer::GetDrawCalls()).c_str());
			RenderProfilingTimers();
		}
		ImGui::End();
	}

	void ImGuiLayer::OnImGuiRender() {
		if (m_render_debug)
			RenderDebug();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
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