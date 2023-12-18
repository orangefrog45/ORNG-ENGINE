#include "../../ORNG-Core/headers/EngineAPI.h"

namespace ORNG {
	class Shader;
	class Framebuffer;
	class Texture2D;
	class EditorLayer;

	class GameLayer : public Layer {
	public:
		GameLayer()=default;

		void SetEditor(EditorLayer* p_layer) { p_editor_layer = p_layer; };

		 void OnInit() override;
		 void Update() override;
		 void OnRender() override;
		 void OnShutdown() override;
		 void OnImGuiRender() override;

		 std::unique_ptr<Scene> p_scene = nullptr;

	private:
		void DrawMap();

		void UpdateParticles();
		void RenderParticles();


		// Set up once with ifdefs for different update/render functionality
		ShaderVariants* p_particle_update_shader_variants = nullptr;
		ShaderVariants* p_particle_render_shader_variants = nullptr;


		EditorLayer* p_editor_layer = nullptr;

		Shader* p_map_shader = nullptr;
		Shader* p_kleinian_shader = nullptr;
		Framebuffer* p_map_fb = nullptr;
		std::unique_ptr<Texture2D> p_map_tex = nullptr;
	};

}