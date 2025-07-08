#include "EngineAPI.h"
#include "VRlib/core/headers/VR.h"
#include "layers/RuntimeSettings.h"

namespace ORNG {
	class RuntimeLayer : public Layer {
		void OnInit() override;
		void Update() override;
		void OnRender() override;
		void OnShutdown() override;
		void OnImGuiRender() override;
	private:
		void InitRenderGraph();

		void LoadRuntimeSettings();

		void InitVR();

		void RenderToVrTargets();

		void RenderToPcTarget();

		RenderGraph m_render_graph;

		RuntimeSettings m_settings;
		std::unique_ptr<vrlib::VR> mp_vr = nullptr;
		std::unique_ptr<Framebuffer> mp_vr_framebuffer = nullptr;
		XrFrameState m_xr_frame_state{};

		// To resize display texture on window resize
		Events::EventListener<Events::WindowEvent> m_window_event_listener;
		Shader* mp_quad_shader = nullptr;
		Scene m_scene;
		std::unique_ptr<Texture2D> mp_display_tex = nullptr;
	};

}