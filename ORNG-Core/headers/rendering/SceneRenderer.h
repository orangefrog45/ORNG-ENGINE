#pragma once
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"
#include "rendering/Textures.h"
#include "scene/Scene.h"
#include "rendering/Material.h"
#include "rendering/renderpasses/BloomPass.h"
#include "rendering/renderpasses/SSAOPass.h"

#ifdef ORNG_EDITOR_LAYER
#include "Settings.h"
#endif

//#define VOXEL_GI

namespace ORNG {
	class Scene;
	class ShaderLibrary;
	class FramebufferLibrary;
	struct CameraComponent;

	template<typename T>
	concept ShaderType = requires(T t) {
		std::is_same_v<T, Shader> || std::is_same_v<T, ShaderVariants>;
	};

	enum RenderpassStage {
		POST_GBUFFER,
		POST_DEPTH,
		POST_LIGHTING,
		POST_POST_PROCESS
	};

	struct RenderResources {
		Framebuffer* p_gbuffer_fb;
		Framebuffer* p_depth_fb;
		Texture2D* p_output_tex;

		Texture2DArray* p_spotlight_depth_tex;
		TextureCubemapArray* p_pointlight_depth_tex;
	};

	typedef void(__cdecl* RenderpassFunc)(RenderResources&);

	struct Renderpass {
		RenderpassStage stage;
		std::function<void(RenderResources&)> func;
		std::string name;
	};

	enum class VoxelizationSV {
		MAIN
	};

	enum class VoxelCS_SV {
		DECREMENT_LUMINANCE,
		ON_CAM_POS_UPDATE,
		BLIT,
	};

	class SpotlightSystem {
		friend class SceneRenderer;
	public:
		SpotlightSystem() = default;
		virtual ~SpotlightSystem() = default;

		void OnLoad();
		void OnUpdate(entt::registry* p_registry);
		void OnUnload();
	private:

		void WriteLightToVector(std::vector<float>& output_vec, SpotLightComponent& light, int& index);
		Texture2DArray m_spotlight_depth_tex{
		"Spotlight depth"
		}; // Used for shadow maps
		SSBO<float> m_spotlight_ssbo{ true, 0 };
	};



	class PointlightSystem {
		friend class SceneRenderer;
	public:
		PointlightSystem() = default;
		~PointlightSystem() = default;
		void OnLoad();
		void OnUpdate(entt::registry* p_registry);
		void OnUnload();
		void WriteLightToVector(std::vector<float>& output_vec, PointLightComponent& light, int& index);

		// Checks if the depth map array needs to grow/shrink
		void OnDepthMapUpdate();

		static constexpr unsigned POINTLIGHT_SHADOW_MAP_RES = 2048;

	private:
		TextureCubemapArray m_pointlight_depth_tex{ "Pointlight depth" }; // Used for shadow maps
		SSBO<float> m_pointlight_ssbo{ true, 0 };
	};


	class SceneRenderer {
		friend class EditorLayer;
	public:
		SceneRenderer() = default;
		~SceneRenderer();

		void Init();
		void Unload();

		struct SceneRenderingSettings {
			Scene* p_scene = nullptr;

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

		void RenderScene(const SceneRenderingSettings& settings);

		// Insert a custom renderpass during RenderScene
		void AttachRenderpassIntercept(const Renderpass& renderpass);
		void DetachRenderpassIntercept(const std::string& name);

		void DrawMeshGBuffer(ShaderVariants* p_shader, const MeshAsset* p_mesh, RenderGroup render_group, unsigned instances, 
			const Material* const* materials, MaterialFlags mat_flags, MaterialFlags mat_flags_excluded, bool allow_state_changes,
			GLenum primitive_type = GL_TRIANGLES);

		void PrepRenderPasses(CameraComponent* p_cam, Texture2D* p_output_tex, Scene* p_scene);
		void DoGBufferPass(CameraComponent* p_cam, const SceneRenderingSettings& settings);
		void DoDepthPass(CameraComponent* p_cam, Texture2D* p_output_tex, Scene* p_scene);
		void DoLightingPass(Texture2D* output_tex, Scene* p_scene);
		void DoFogPass(unsigned int width, unsigned int height, Scene* p_scene);
		void DoPostProcessingPass(CameraComponent* p_cam, Texture2D* output_tex, Scene* p_scene);
		void DoTransparencyPass(Texture2D* p_output_tex, unsigned width, unsigned height, Scene* p_scene);
		void DoVoxelizationPass(unsigned output_width, unsigned output_height, Texture3D& main_cascade_tex, Texture3D& normals_main_cascade_tex, 
			Texture3D& cascade_mips, unsigned cascade_width, float voxel_size, VoxelizationSV shader_variant, glm::vec3 cam_pos, Scene* p_scene);

	private:
		// Shift voxel luminance data in the 3D texture when the cameras voxel-aligned position changes
		void AdjustVoxelGridForCameraMovement(Texture3D& voxel_luminance_tex, Texture3D& intermediate_copy_tex, glm::ivec3 delta_tex_coords, unsigned tex_size);

		// Returns <aligned_cam_pos_has_changed, new_camera_position, delta_tex_coords>
		std::tuple<bool, glm::vec3, glm::vec3> UpdateVoxelAlignedCameraPos(float alignment, glm::vec3 unaligned_cam_pos, glm::vec3 voxel_aligned_cam_pos);

		// Returns true if any state was changed
		bool SetGL_StateFromMatFlags(MaterialFlags flags);

		// Sets state back to default values
		void UndoGL_StateModificationsFromMatFlags(MaterialFlags flags);

		void DrawAllMeshesDepth(RenderGroup render_group, Scene* p_scene);

		void IAttachRenderpassIntercept(const Renderpass& renderpass) {
			ASSERT(std::ranges::find_if(m_render_intercepts, [&](const auto& pass) {return pass.name == renderpass.name; }) == m_render_intercepts.end());
			m_render_intercepts.push_back(renderpass);
		}

		void IDetachRenderpassIntercept(const std::string& name) {
			auto it = std::ranges::find_if(m_render_intercepts, [&](const auto& pass) {return pass.name == name; });
			ASSERT(it != m_render_intercepts.end());
			m_render_intercepts.erase(it);
		}

		void UpdateLightSpaceMatrices(CameraComponent* p_cam, Scene* p_scene);

		void RunRenderpassIntercepts(RenderpassStage stage, RenderResources& res);
		void DrawTerrain(CameraComponent* p_cam, Scene* p_scene);
		void DrawSkybox();
		void DoBloomPass(Texture2D* p_input_and_output, unsigned int width, unsigned int height, Scene* p_scene);
		void CheckResizeScreenSizeTextures(Texture2D* p_output_tex);
		void SetGBufferMaterial(ShaderVariants* p_shader, const Material* p_mat);

		void DrawInstanceGroupGBuffer(ShaderVariants* p_shader, const MeshInstanceGroup* p_group, RenderGroup render_group, MaterialFlags mat_flags, 
			MaterialFlags mat_flags_exclusion, bool allow_state_changes, GLenum primitive_type = GL_TRIANGLES);
		void RenderVehicles(ShaderVariants* p_shader, RenderGroup render_group, Scene* p_scene);

		// User-attached renderpasses go here, e.g to insert a custom renderpass just after the gbuffer stage
		std::vector<Renderpass> m_render_intercepts;

		BloomPass m_bloom_pass;
		SSAOPass m_ssao_pass;

#define NUM_VOXEL_CASCADES 2
		// Camera position snapped to the voxel grid per cascade, index 0 = cascade 0 etc (prevents flickering when moving)
		std::array<glm::vec3, NUM_VOXEL_CASCADES> m_voxel_aligned_cam_positions{ glm::vec3{ 0 } };
		unsigned m_active_voxel_cascade_idx = 0;

		ShaderVariants* mp_gbuffer_shader_variants = nullptr;
		ShaderVariants* mp_gbuffer_displacement_sv = nullptr;

		Shader* m_post_process_shader = nullptr;

		ShaderVariants* mp_depth_sv = nullptr;
		ShaderVariants* mp_3d_mipmap_shader = nullptr;

		ShaderVariants* mp_scene_voxelization_shader = nullptr;
		ShaderVariants* mp_voxel_compute_sv = nullptr;
		Shader* mp_voxel_debug_shader = nullptr;
		Framebuffer* mp_scene_voxelization_fb = nullptr;

		ShaderVariants* mp_depth_aware_upsample_sv = nullptr;
		Shader* m_blur_shader = nullptr;
		Shader* m_fog_shader = nullptr;
		Shader* m_lighting_shader = nullptr;
		Shader* mp_cone_trace_shader = nullptr;
		ShaderVariants* mp_transparency_shader_variants = nullptr;
		Shader* mp_transparency_composite_shader = nullptr;

		Framebuffer* m_gbuffer_fb = nullptr;
		Framebuffer* m_depth_fb = nullptr;
		Framebuffer* mp_transparency_fb = nullptr;
		Framebuffer* mp_composition_fb = nullptr;

		ShaderLibrary* mp_shader_library = nullptr;
		FramebufferLibrary* mp_framebuffer_library = nullptr;
		
		FullscreenTexture2D m_gbf_normals;
		FullscreenTexture2D m_gbf_albedo;
		FullscreenTexture2D m_gbf_rma;
		FullscreenTexture2D m_gbf_depth;
		FullscreenTexture2D m_gbf_shader_ids;

		FullscreenTexture2D m_transparency_accum;
		FullscreenTexture2D m_transparency_revealage;

		Texture2D m_blue_noise_tex{ "SR blue noise" };
		Texture2D m_fog_output_tex{ "SR fog output" };
		Texture2D m_fog_blur_tex_1{ "SR fog blur 1" };
		Texture2D m_fog_blur_tex_2{ "SR fog blur 2 tex" };
		Texture2DArray m_directional_light_depth_tex{ "SR Directional depth array" };

		Texture2D m_cone_trace_accum_tex{ "SR cone trace accum" };

		Texture3D m_scene_voxel_tex_c0{ "SR scene voxel tex cascade 0" };
		Texture3D m_scene_voxel_tex_c0_normals{ "SR scene voxel tex normals cascade 0" };

		Texture3D m_scene_voxel_tex_c1{ "SR scene voxel tex cascade 1" };
		Texture3D m_scene_voxel_tex_c1_normals{ "SR scene voxel tex normals cascade 1" };

		Texture3D m_voxel_mip_faces_c0{ "SR voxel mips cascade 0" };
		Texture3D m_voxel_mip_faces_c1{ "SR voxel mips cascade 1" };
		
		PointlightSystem m_pointlight_system;
		SpotlightSystem m_spotlight_system;

		unsigned int m_num_shadow_cascades = 3;
		unsigned int m_shadow_map_resolution = 4096;
	};
}