#pragma once
#include "Layer.h"

namespace ORNG {
	class ImGuiLayer : public Layer {
	public:
		void OnInit() override;
		void OnShutdown() override;
		void Update() override;
		void OnRender() override;
		void OnImGuiRender() override;
		void BeginFrame();
	private:
		void RenderProfilingTimers();
		void RenderDebug();
		bool m_render_debug = false;
	};
}