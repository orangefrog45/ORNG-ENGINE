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
		void DrawFractal();

		std::unique_ptr<Texture2D> p_fractal_depth_chain = nullptr;

		glm::vec3 fractal_csize = {0.9, 0.8, 0.7};
		float k_factor_1 = 0.9;
		float k_factor_2 = 0.5;
		int num_iters = 12;

		SSBO<float> m_fractal_animator_ssbo{ true, 0 };

		InterpolatorV3 m_csize_interpolator{ glm::vec2{FLT_MIN, FLT_MAX}, glm::vec2{FLT_MIN, FLT_MAX}, {1, 1, 1}, {1, 1, 1} };
		InterpolatorV1 m_k1_interpolator{ glm::vec2{FLT_MIN, FLT_MAX}, glm::vec2{FLT_MIN, FLT_MAX}, 1, 1 };
		InterpolatorV1 m_k2_interpolator{ glm::vec2{FLT_MIN, FLT_MAX}, glm::vec2{FLT_MIN, FLT_MAX}, 1, 1 };

		// Set up once with ifdefs for different update/render functionality
		ShaderVariants* p_particle_update_shader_variants = nullptr;
		ShaderVariants* p_particle_render_shader_variants = nullptr;


		EditorLayer* p_editor_layer = nullptr;

		Shader* p_map_shader = nullptr;
		ShaderVariants* p_kleinian_shader_mips = nullptr;
		Shader* p_kleinian_shader_full_res = nullptr;

		Framebuffer* p_map_fb = nullptr;
		std::unique_ptr<Texture2D> p_map_tex = nullptr;
	};

}