#include "../../ORNG-Core/headers/EngineAPI.h"

namespace ORNG {
	class RuntimeLayer : public Layer {
		void OnInit() override;
		void Update() override;
		void OnRender() override;
		void OnShutdown() override;
		void OnImGuiRender() override;
	private:
		void InitRenderGraph();

		RenderGraph m_render_graph;

		// To resize display texture on window resize
		Events::EventListener<Events::WindowEvent> m_window_event_listener;
		Shader* mp_quad_shader = nullptr;
		Scene m_scene;
		std::unique_ptr<Texture2D> mp_display_tex = nullptr;
	};

}