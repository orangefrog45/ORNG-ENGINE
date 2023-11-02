#pragma once
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"
#include "rendering/Textures.h"
#include "scene/Scene.h"


namespace ORNG {
	class Scene;
	class ShaderLibrary;
	class FramebufferLibrary;
	struct CameraComponent;

	class SceneRenderer {
		friend class EditorLayer;
	public:
		SceneRenderer() = default;
		~SceneRenderer() {
			m_spotlight_system.OnUnload();
			m_pointlight_system.OnUnload();
		}

		static void Init() {
			Get().I_Init();
		}


		struct SceneRenderingSettings {
			bool display_depth_map = false;
			CameraComponent* p_cam_override = nullptr;

			// Output tex has to have format RGBA16F
			Texture2D* p_output_tex = nullptr;
			Texture2D* p_input_tex = nullptr;
		};

		struct SceneRenderingOutput {
			unsigned int entity_on_mouse_pos = 0;
			unsigned int final_color_texture_handle = 0; // the texture produced as the final product of rendering the scene
		};

		static SceneRenderingOutput RenderScene(const SceneRenderingSettings& settings) {
			return Get().IRenderScene(settings);
		};


		static void SetActiveScene(Scene* p_scene) {
			Get().mp_scene = p_scene;
		}


		static Scene* GetScene() {
			return Get().mp_scene;
		}



		static SceneRenderer& Get() {
			static SceneRenderer s_instance;
			return s_instance;
		}

		void PrepRenderPasses(CameraComponent* p_cam, Texture2D* p_output_tex);
		void DoGBufferPass(CameraComponent* p_cam);
		void DoDepthPass(CameraComponent* p_cam, Texture2D* p_output_tex);
		void DoFogPass(unsigned int width, unsigned int height);
		void DoLightingPass(Texture2D* output_tex);
		void DoPostProcessingPass(CameraComponent* p_cam, Texture2D* output_tex);

	private:


		void I_Init();
		SceneRenderingOutput IRenderScene(const SceneRenderingSettings& settings);
		void DrawTerrain(CameraComponent* p_cam);
		void DrawSkybox();
		void DoBloomPass(unsigned int width, unsigned int height);
		void DrawAllMeshes() const;

		void SetGBufferMaterial(const Material* p_mat, Shader* p_gbuffer_shader);


		std::vector<glm::mat4> m_light_space_matrices = { glm::mat4(1), glm::mat4(1), glm::mat4(1) };


		Shader* mp_gbuffer_shader_terrain = nullptr;
		Shader* mp_gbuffer_shader_skybox = nullptr;
		Shader* mp_gbuffer_shader_mesh = nullptr;
		Shader* m_post_process_shader = nullptr;
		Shader* mp_orth_depth_shader = nullptr; //dir light
		Shader* mp_persp_depth_shader = nullptr; //spotlights
		Shader* mp_pointlight_depth_shader = nullptr;
		Shader* m_blur_shader = nullptr;
		Shader* m_fog_shader = nullptr;
		Shader* m_lighting_shader = nullptr;
		Shader* mp_bloom_downsample_shader = nullptr;
		Shader* mp_bloom_upsample_shader = nullptr;
		Shader* mp_bloom_threshold_shader = nullptr;
		Shader* mp_portal_shader = nullptr;

		Framebuffer* m_gbuffer_fb = nullptr;
		Framebuffer* m_depth_fb = nullptr;
		//none of the objects the pointers below reference managed by scene renderer, just inputs
		Scene* mp_scene = nullptr;
		ShaderLibrary* mp_shader_library = nullptr;
		FramebufferLibrary* mp_framebuffer_library = nullptr;

		Texture2D m_blue_noise_tex{ "SR blue noise" };
		Texture2D m_fog_output_tex{ "SR fog output" };
		Texture2D m_fog_blur_tex_1{ "SR fog blur 1" };
		Texture2D m_fog_blur_tex_2{ "SR fog blur 2 tex" };
		Texture2D m_bloom_tex{ "SR fog blur 1" };
		Texture2DArray m_directional_light_depth_tex{ "SR Directional depth array" };

		PointlightSystem m_pointlight_system;
		SpotlightSystem m_spotlight_system;

		glm::vec3 m_sampled_world_pos = { 0, 0, 0 };
		unsigned int m_num_shadow_cascades = 3;
		unsigned int m_shadow_map_resolution = 4096;
	};
}