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
	public:

		static void Init() {
			Get().I_Init();
		}


		struct SceneRenderingSettings {
			bool display_depth_map = false;
			CameraComponent* p_cam_override = nullptr;
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




	private:

		static SceneRenderer& Get() {
			static SceneRenderer s_instance;
			return s_instance;
		}


		void I_Init();
		SceneRenderingOutput IRenderScene(const SceneRenderingSettings& settings);
		void IPrepRenderPasses(CameraComponent* p_cam);
		void IDoGBufferPass(CameraComponent* p_cam);
		void IDoDepthPass(CameraComponent* p_cam);
		void IDoFogPass();
		void IDoLightingPass();
		void IDoPostProcessingPass(CameraComponent* p_cam);
		void IDrawTerrain(CameraComponent* p_cam);
		void DrawSkybox();
		void DoBloomPass();
		void DrawAllMeshes() const;

		void SetGBufferMaterial(const Material* p_mat);


		std::vector<glm::mat4> m_light_space_matrices = { glm::mat4(1), glm::mat4(1), glm::mat4(1) };


		Shader* m_gbuffer_shader = nullptr;
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

		Framebuffer* m_gbuffer_fb = nullptr;
		Framebuffer* m_depth_fb = nullptr;
		Framebuffer* m_lighting_fb = nullptr;
		Framebuffer* m_post_processing_fb = nullptr;

		//none of the objects the pointers below reference managed by scene renderer, just inputs
		Scene* mp_scene = nullptr;
		ShaderLibrary* mp_shader_library = nullptr;
		FramebufferLibrary* mp_framebuffer_library = nullptr;

		Texture2D m_blue_noise_tex{ "SR blue noise" };
		Texture2D m_fog_output_tex{ "SR fog output" };
		Texture2D m_fog_blur_tex_1{ "SR fog blur 1" };
		Texture2D m_fog_blur_tex_2{ "SR fog blur 2 tex" };
		Texture2D m_bloom_tex{ "SR fog blur 1" };


		glm::vec3 m_sampled_world_pos = { 0, 0, 0 };
		unsigned int m_num_shadow_cascades = 3;
		unsigned int m_shadow_map_resolution = 4096;

	};
}