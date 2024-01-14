#pragma once
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"
#include "rendering/Textures.h"
#include "scene/Scene.h"
#include "rendering/Material.h"

#ifdef ORNG_EDITOR_LAYER
#include "Settings.h"
#endif

namespace ORNG {
	class Scene;
	class ShaderLibrary;
	class FramebufferLibrary;
	struct CameraComponent;

	enum RenderpassStage {
		POST_GBUFFER,
		POST_DEPTH,
		POST_LIGHTING,
		POST_POST_PROCESS
	};

	struct RenderResources {
		Framebuffer* p_gbuffer_fb;
	};

	typedef void(__cdecl* RenderpassFunc)(RenderResources);

	struct Renderpass {
		RenderpassStage stage;
		std::function<void(RenderResources)> func;
		std::string name;
	};


	struct DirectMeshRenderData {
		glm::vec3 camera_pos;
		float cam_fov = 60.f;
		float aspect_ratio = 1.f;
		Texture2D* p_output_tex = nullptr;

		MeshAsset* p_mesh = nullptr;
		std::vector<Material*> materials;
	};

	struct VoxelMipFaces {
		Texture3D pos_x{ "Voxel face X+" };
		Texture3D pos_y{ "Voxel face Y+" };
		Texture3D pos_z{ "Voxel face Z+" };

		Texture3D neg_x{ "Voxel face X-" };
		Texture3D neg_y{ "Voxel face Y-" };
		Texture3D neg_z{ "Voxel face Z-" };
	};

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
			// Will custom user renderpasses be executed
			bool do_intercept_renderpasses = true;
			bool render_meshes = true;
			bool display_depth_map = false;
			CameraComponent* p_cam_override = nullptr;

#ifdef ORNG_EDITOR_LAYER
			VoxelRenderFace voxel_render_face = VoxelRenderFace::NONE;
			unsigned voxel_mip_layer = 0;
#endif

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

		// Insert a custom renderpass during RenderScene
		static void AttachRenderpassIntercept(Renderpass renderpass) {
			Get().IAttachRenderpassIntercept(renderpass);
		}

		static void DetachRenderpassIntercept(const std::string& name) {
			Get().IDetachRenderpassIntercept(name);
		}

		static void DrawMeshGBuffer(ShaderVariants* p_shader, const MeshAsset* p_mesh, RenderGroup render_group, unsigned instances, const Material* const* materials) {
			Get().IDrawMeshGBuffer(p_shader, p_mesh, render_group, instances, materials);
		}

		static SceneRenderer& Get() {
			static SceneRenderer s_instance;
			return s_instance;
		}

		void PrepRenderPasses(CameraComponent* p_cam, Texture2D* p_output_tex);
		void DoGBufferPass(CameraComponent* p_cam, const SceneRenderingSettings& settings);
		void DoDepthPass(CameraComponent* p_cam, Texture2D* p_output_tex);
		void DoLightingPass(Texture2D* output_tex);
		void DoFogPass(unsigned int width, unsigned int height);
		void DoPostProcessingPass(CameraComponent* p_cam, Texture2D* output_tex);
		void DoTransparencyPass(Texture2D* p_output_tex, unsigned width, unsigned height);
		void DoVoxelizationPass(unsigned output_width, unsigned output_height);

	private:

		void IAttachRenderpassIntercept(Renderpass renderpass) {
			ASSERT(std::ranges::find_if(m_render_intercepts, [&](const auto& pass) {return pass.name == renderpass.name; }) == m_render_intercepts.end());
			m_render_intercepts.push_back(renderpass);
		}

		void IDetachRenderpassIntercept(const std::string& name) {
			auto it = std::ranges::find_if(m_render_intercepts, [&](const auto& pass) {return pass.name == name; });
			ASSERT(it != m_render_intercepts.end());
			m_render_intercepts.erase(it);
		}

		void UpdateLightSpaceMatrices(CameraComponent* p_cam);

		void RunRenderpassIntercepts(RenderpassStage stage, const RenderResources& res);
		void I_Init();
		SceneRenderingOutput IRenderScene(const SceneRenderingSettings& settings);
		void DrawTerrain(CameraComponent* p_cam);
		void DrawSkybox();
		void DoBloomPass(unsigned int width, unsigned int height);
		void DrawAllMeshes(RenderGroup render_group) const;
		void CheckResizeScreenSizeTextures(Texture2D* p_output_tex);
		void SetGBufferMaterial(ShaderVariants* p_shader, const Material* p_mat);

		void DrawInstanceGroupGBuffer(ShaderVariants* p_shader, const MeshInstanceGroup* p_group, RenderGroup render_group);
		void IDrawMeshGBuffer(ShaderVariants* p_shader, const MeshAsset* p_mesh, RenderGroup render_group, unsigned instances, const Material* const* materials);
		void RenderVehicles(ShaderVariants* p_shader, RenderGroup render_group);


		// User-attached renderpasses go here, e.g to insert a custom renderpass just after the gbuffer stage
		std::vector<Renderpass> m_render_intercepts;

		ShaderVariants* mp_gbuffer_shader_variants = nullptr;

		Shader* m_post_process_shader = nullptr;

		Shader* mp_orth_depth_shader = nullptr; //dir light
		Shader* mp_persp_depth_shader = nullptr; //spotlights
		Shader* mp_pointlight_depth_shader = nullptr;
		ShaderVariants* mp_3d_mipmap_shader = nullptr;

		ShaderVariants* mp_scene_voxelization_shader = nullptr;
		Shader* mp_voxel_debug_shader = nullptr;
		Framebuffer* mp_scene_voxelization_fb = nullptr;

		ShaderVariants* mp_depth_aware_upsample_sv = nullptr;
		Shader* m_blur_shader = nullptr;
		Shader* m_fog_shader = nullptr;
		Shader* m_lighting_shader = nullptr;
		Shader* mp_bloom_downsample_shader = nullptr;
		Shader* mp_bloom_upsample_shader = nullptr;
		Shader* mp_bloom_threshold_shader = nullptr;
		Shader* mp_cone_trace_shader = nullptr;
		ShaderVariants* mp_transparency_shader_variants = nullptr;

		Shader* mp_transparency_composite_shader = nullptr;

		Framebuffer* m_gbuffer_fb = nullptr;
		Framebuffer* m_depth_fb = nullptr;
		Framebuffer* mp_transparency_fb = nullptr;
		Framebuffer* mp_composition_fb = nullptr;
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

		Texture2D m_cone_trace_accum_tex{ "SR cone trace accum" };

		Texture3D m_scene_voxel_tex{ "SR scene voxel tex" };
		Texture3D m_scene_voxel_tex_normals{ "SR scene voxel tex normals" };
		VoxelMipFaces m_voxel_mip_faces;

		PointlightSystem m_pointlight_system;
		SpotlightSystem m_spotlight_system;

		unsigned int m_num_shadow_cascades = 3;
		unsigned int m_shadow_map_resolution = 4096;
	};
}