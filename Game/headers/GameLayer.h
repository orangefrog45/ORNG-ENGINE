#include "../../ORNG-Core/headers/EngineAPI.h"

namespace ORNG {
	class Shader;
	class Framebuffer;
	class Texture2D;

	class GameLayer : public Layer {
		 void OnInit() override;
		 void Update() override;
		 void OnRender() override;
		 void OnShutdown() override;
		 void OnImGuiRender() override;
	private:
		Shader* p_map_shader = nullptr;
		Shader* p_kleinian_shader = nullptr;
		Framebuffer* p_map_fb = nullptr;
		std::unique_ptr<Texture2D> p_map_tex = nullptr;
	};

}