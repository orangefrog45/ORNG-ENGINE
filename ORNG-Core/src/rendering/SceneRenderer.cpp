#include "pch/pch.h"

#include "rendering/SceneRenderer.h"
#include "shaders/ShaderLibrary.h"
#include "core/Window.h"
#include "rendering/Renderer.h"
#include "core/GLStateManager.h"
#include "components/CameraComponent.h"
#include "events/EventManager.h"
#include "util/Timers.h"
#include "scene/SceneEntity.h"
#include "terrain/TerrainChunk.h"
#include "core/FrameTiming.h"
#include "rendering/Material.h"
#include "assets/AssetManager.h"
#include "glm/glm/gtc/round.hpp"
#include <glm/glm/gtc/integer.hpp>

#include "scene/MeshInstanceGroup.h"



namespace ORNG {
	enum GBufferVariants {
		TERRAIN,
		MESH,
		PARTICLE,
		SKYBOX,
		BILLBOARD,
		PARTICLE_BILLBOARD
	};

	enum TransparencyShaderVariants {
		DEFAULT,
		T_PARTICLE,
		T_PARTICLE_BILLBOARD,
	};

	enum MipMap3D_ShaderVariants {
		DEFAULT_MIP,
		ANISOTROPIC,
		ANISOTROPIC_CHAIN,
	};

	enum class DepthAwareUpsampleSV {
		DEFAULT,
		CONE_TRACE
	};

	void SceneRenderer::I_Init() {
		mp_shader_library = &Renderer::GetShaderLibrary();
		mp_framebuffer_library = &Renderer::GetFramebufferLibrary();
		m_pointlight_system.OnLoad();
		m_spotlight_system.OnLoad();

		std::vector<std::string> gbuffer_uniforms{
			"u_roughness_sampler_active",
				"u_metallic_sampler_active",
				"u_emissive_sampler_active",
				"u_normal_sampler_active",
				"u_ao_sampler_active",
				"u_displacement_sampler_active",
				"u_num_parallax_layers",
				"u_parallax_height_scale",
				"u_material.base_color",
				"u_material.metallic",
				"u_material.roughness",
				"u_material.ao",
				"u_material.tile_scale",
				"u_material.emissive",
				"u_material.emissive_strength",
				"u_bloom_threshold",
				"u_shader_id",
				"u_material.using_spritesheet",
				"u_material.sprite_data.num_rows",
				"u_material.sprite_data.num_cols",
				"u_material.sprite_data.fps"
		};

		std::vector<std::string> ptcl_uniforms = gbuffer_uniforms;
		ptcl_uniforms.push_back("u_transform_start_index");


		mp_gbuffer_shader_variants = &mp_shader_library->CreateShaderVariants("gbuffer");
		mp_gbuffer_shader_variants->SetPath(GL_VERTEX_SHADER, "res/shaders/GBufferVS.glsl");
		mp_gbuffer_shader_variants->SetPath(GL_FRAGMENT_SHADER, "res/shaders/GBufferFS.glsl");
		{
			using enum GBufferVariants;
			mp_gbuffer_shader_variants->AddVariant(TERRAIN, { "TERRAIN_MODE" }, gbuffer_uniforms);
			mp_gbuffer_shader_variants->AddVariant(MESH, {}, gbuffer_uniforms);
			mp_gbuffer_shader_variants->AddVariant(PARTICLE, { "PARTICLE" }, ptcl_uniforms);
			mp_gbuffer_shader_variants->AddVariant(SKYBOX, { "SKYBOX_MODE" }, {});
			mp_gbuffer_shader_variants->AddVariant(BILLBOARD, { "BILLBOARD" }, gbuffer_uniforms);
			mp_gbuffer_shader_variants->AddVariant(PARTICLE_BILLBOARD, { "PARTICLE", "BILLBOARD" }, ptcl_uniforms);
		}


		mp_transparency_shader_variants = &mp_shader_library->CreateShaderVariants("transparency");
		mp_transparency_shader_variants->SetPath(GL_VERTEX_SHADER, "res/shaders/GBufferVS.glsl");
		mp_transparency_shader_variants->SetPath(GL_FRAGMENT_SHADER, "res/shaders/WeightedBlendedFS.glsl");
		{
			using enum TransparencyShaderVariants;
			mp_transparency_shader_variants->AddVariant(DEFAULT, { }, gbuffer_uniforms);
			mp_transparency_shader_variants->AddVariant(T_PARTICLE, { "PARTICLE" }, ptcl_uniforms);
			mp_transparency_shader_variants->AddVariant(T_PARTICLE_BILLBOARD, { "PARTICLE", "BILLBOARD" }, ptcl_uniforms);
		}

		auto voxel_uniforms = gbuffer_uniforms;
		voxel_uniforms.push_back("u_orth_proj_view_matrix");
		voxel_uniforms.push_back("u_aligned_camera_pos");
		mp_scene_voxelization_shader = &mp_shader_library->CreateShaderVariants("scene-voxelizer");
		mp_scene_voxelization_shader->SetPath(GL_VERTEX_SHADER, "res/shaders/GBufferVS.glsl");
		mp_scene_voxelization_shader->SetPath(GL_FRAGMENT_SHADER, "res/shaders/SceneVoxelizationFS.glsl");
		mp_scene_voxelization_shader->AddVariant(0, {"VOXELIZE"}, voxel_uniforms);

		mp_voxel_debug_shader = &mp_shader_library->CreateShader("voxel_debug");
		mp_voxel_debug_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/VoxelDebugViewVS.glsl");
		mp_voxel_debug_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/ColorFS.glsl", {"VOXELIZATION"});
		mp_voxel_debug_shader->Init();
		mp_voxel_debug_shader->AddUniform("u_aligned_camera_pos");

		mp_3d_mipmap_shader = &mp_shader_library->CreateShaderVariants("3d-mipmap");
		mp_3d_mipmap_shader->SetPath(GL_COMPUTE_SHADER, "res/shaders/MipMap3D.glsl");
		mp_3d_mipmap_shader->AddVariant(MipMap3D_ShaderVariants::DEFAULT_MIP, {}, {});
		mp_3d_mipmap_shader->AddVariant(MipMap3D_ShaderVariants::ANISOTROPIC, {"ANISOTROPIC_MIPMAP"}, {});
		mp_3d_mipmap_shader->AddVariant(MipMap3D_ShaderVariants::ANISOTROPIC_CHAIN, { "ANISOTROPIC_MIPMAP_CHAIN" }, {"u_mip_level"});


		mp_transparency_composite_shader = &mp_shader_library->CreateShader("transparency_composite");
		mp_transparency_composite_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/TransparentCompositeFS.glsl");
		mp_transparency_composite_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.glsl");
		mp_transparency_composite_shader->Init();

		m_lighting_shader = &mp_shader_library->CreateShader("lighting", ShaderLibrary::LIGHTING_SHADER_ID);
		m_lighting_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/LightingCS.glsl");
		m_lighting_shader->Init();
		m_lighting_shader->AddUniforms({
			"u_aligned_camera_pos",
			});



		// Render quad
		m_post_process_shader = &mp_shader_library->CreateShader("post_process");
		m_post_process_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/PostProcessCS.glsl");
		m_post_process_shader->Init();
		m_post_process_shader->AddUniform("exposure");
		m_post_process_shader->AddUniform("u_bloom_intensity");

		m_post_process_shader->AddUniforms({
			"quad_sampler",
			"u_bloom_intensity",
			"world_position_sampler",
			"camera_pos",
			"u_show_depth_map"
			});


		// blue noise for post-processing effects in quad
		Texture2DSpec noise_spec;
		noise_spec.filepath = "res/textures/BlueNoise64Tiled.png";
		noise_spec.min_filter = GL_NEAREST;
		noise_spec.mag_filter = GL_NEAREST;
		noise_spec.wrap_params = GL_REPEAT;
		noise_spec.storage_type = GL_UNSIGNED_BYTE;
		noise_spec.wrap_params = GL_REPEAT;

		m_blue_noise_tex.SetSpec(noise_spec);
		m_blue_noise_tex.LoadFromFile();


		mp_orth_depth_shader = &mp_shader_library->CreateShader("orth_depth");
		mp_orth_depth_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/DepthVS.glsl");
		mp_orth_depth_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/DepthFS.glsl", { "ORTHOGRAPHIC" });
		mp_orth_depth_shader->Init();
		mp_orth_depth_shader->AddUniform("u_light_pv_matrix");

		mp_persp_depth_shader = &mp_shader_library->CreateShader("persp_depth");
		mp_persp_depth_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/DepthVS.glsl");
		mp_persp_depth_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/DepthFS.glsl", { "PERSPECTIVE", "SPOTLIGHT"});
		mp_persp_depth_shader->Init();
		mp_persp_depth_shader->AddUniform("u_light_pv_matrix");
		mp_persp_depth_shader->AddUniform("u_light_pos");


		mp_pointlight_depth_shader = &mp_shader_library->CreateShader("pointlight_depth");
		mp_pointlight_depth_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/DepthVS.glsl");
		mp_pointlight_depth_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/DepthFS.glsl", { "PERSPECTIVE", "POINTLIGHT" });
		mp_pointlight_depth_shader->Init();
		mp_pointlight_depth_shader->AddUniform("u_light_pv_matrix");
		mp_pointlight_depth_shader->AddUniform("u_light_pos");
		mp_pointlight_depth_shader->AddUniform("u_light_zfar");


		m_blur_shader = &mp_shader_library->CreateShader("SR Blur");
		m_blur_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/BlurFS.glsl");
		m_blur_shader->Init();
		m_blur_shader->AddUniform("u_horizontal");

		mp_depth_aware_upsample_sv = &mp_shader_library->CreateShaderVariants("SR depth aware upsample");
		mp_depth_aware_upsample_sv->SetPath(GL_COMPUTE_SHADER, "res/shaders/DepthAwareUpsampleCS.glsl");
		mp_depth_aware_upsample_sv->AddVariant((unsigned)DepthAwareUpsampleSV::DEFAULT, {}, {});
		// Cone trace upsample pass is also normal-aware as well as depth-aware
		mp_depth_aware_upsample_sv->AddVariant((unsigned)DepthAwareUpsampleSV::CONE_TRACE, {"CONE_TRACE_UPSAMPLE"}, {});


		/* GBUFFER FB */
		m_gbuffer_fb = &mp_framebuffer_library->CreateFramebuffer("gbuffer", true);

		Texture2DSpec gbuffer_spec_2;
		gbuffer_spec_2.format = GL_RED_INTEGER;
		gbuffer_spec_2.internal_format = GL_R8UI;
		gbuffer_spec_2.storage_type = GL_UNSIGNED_INT;
		gbuffer_spec_2.width = Window::GetWidth();
		gbuffer_spec_2.height = Window::GetHeight();


		Texture2DSpec low_pres_spec;
		low_pres_spec.format = GL_RGBA;
		low_pres_spec.internal_format = GL_RGBA16F;
		low_pres_spec.storage_type = GL_FLOAT;
		low_pres_spec.width = Window::GetWidth();
		low_pres_spec.height = Window::GetHeight();


		Texture2DSpec gbuffer_depth_spec;
		gbuffer_depth_spec.format = GL_DEPTH_COMPONENT;
		gbuffer_depth_spec.internal_format = GL_DEPTH_COMPONENT24;
		gbuffer_depth_spec.storage_type = GL_FLOAT;
		gbuffer_depth_spec.min_filter = GL_NEAREST;
		gbuffer_depth_spec.mag_filter = GL_NEAREST;
		gbuffer_depth_spec.width = Window::GetWidth();
		gbuffer_depth_spec.height = Window::GetHeight();

		m_gbuffer_fb->Add2DTexture("normals", GL_COLOR_ATTACHMENT0, low_pres_spec);
		m_gbuffer_fb->Add2DTexture("albedo", GL_COLOR_ATTACHMENT1, low_pres_spec);
		m_gbuffer_fb->Add2DTexture("roughness_metallic_ao", GL_COLOR_ATTACHMENT2, low_pres_spec);
		m_gbuffer_fb->Add2DTexture("shader_ids", GL_COLOR_ATTACHMENT3, gbuffer_spec_2);
		m_gbuffer_fb->Add2DTexture("shared_depth", GL_DEPTH_ATTACHMENT, gbuffer_depth_spec);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		m_gbuffer_fb->EnableDrawBuffers(4, buffers);


		/* TRANSPARENCY FB */
		mp_transparency_fb = &mp_framebuffer_library->CreateFramebuffer("transparency", true);
		auto lp4_spec = low_pres_spec;
		lp4_spec.internal_format = GL_RGBA16F;
		lp4_spec.format = GL_RGBA;
		mp_transparency_fb->Add2DTexture("accum", GL_COLOR_ATTACHMENT0, lp4_spec);
		Texture2DSpec r8_spec = low_pres_spec;
		r8_spec.format = GL_RED;
		r8_spec.internal_format = GL_R8;
		mp_transparency_fb->Add2DTexture("revealage", GL_COLOR_ATTACHMENT1, r8_spec);
		GLenum buffers2[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		mp_transparency_fb->EnableDrawBuffers(2, buffers2);

		/* TRANSPARENCY COMPOSITION FB */
		mp_composition_fb = &mp_framebuffer_library->CreateFramebuffer("transparent_composition", true);

		/* DIRECTIONAL LIGHT DEPTH FB */
		m_depth_fb = &mp_framebuffer_library->CreateFramebuffer("dir_depth", false);

		Texture2DArraySpec depth_spec;
		depth_spec.format = GL_DEPTH_COMPONENT;
		depth_spec.internal_format = GL_DEPTH_COMPONENT16;
		depth_spec.storage_type = GL_FLOAT;
		depth_spec.min_filter = GL_NEAREST;
		depth_spec.mag_filter = GL_NEAREST;
		depth_spec.wrap_params = GL_CLAMP_TO_EDGE;
		depth_spec.layer_count = 3;
		depth_spec.width = m_shadow_map_resolution;
		depth_spec.height = m_shadow_map_resolution;

		Texture2DArraySpec spotlight_depth_spec = depth_spec;
		spotlight_depth_spec.internal_format = GL_DEPTH_COMPONENT24;
		spotlight_depth_spec.width = 2048;
		spotlight_depth_spec.height = 2048;
		spotlight_depth_spec.layer_count = 8;

		m_directional_light_depth_tex.SetSpec(depth_spec);

		/* POST PROCESSING */
		Texture2DSpec rgba16_spec;
		rgba16_spec.format = GL_RGBA;
		rgba16_spec.internal_format = GL_RGBA16F;
		rgba16_spec.storage_type = GL_FLOAT;
		rgba16_spec.width = Window::GetWidth();
		rgba16_spec.height = Window::GetHeight();
		rgba16_spec.min_filter = GL_NEAREST;
		rgba16_spec.mag_filter = GL_NEAREST;
		rgba16_spec.wrap_params = GL_CLAMP_TO_EDGE;


		m_fog_blur_tex_1.SetSpec(rgba16_spec);
		m_fog_blur_tex_2.SetSpec(rgba16_spec);

		Texture2DSpec bloom_rgb_spec;
		bloom_rgb_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		bloom_rgb_spec.mag_filter = GL_LINEAR;
		bloom_rgb_spec.generate_mipmaps = true;
		bloom_rgb_spec.width = Window::GetWidth() / 2;
		bloom_rgb_spec.height = Window::GetHeight() / 2;
		bloom_rgb_spec.format = GL_RGBA;
		bloom_rgb_spec.internal_format = GL_RGBA16F;
		bloom_rgb_spec.wrap_params = GL_CLAMP_TO_EDGE;
		bloom_rgb_spec.storage_type = GL_FLOAT;

		m_bloom_tex.SetSpec(bloom_rgb_spec);

		// Fog
		m_fog_shader = &mp_shader_library->CreateShader("fog");
		m_fog_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/FogCS.glsl");
		m_fog_shader->Init();
		m_fog_shader->AddUniforms({
			"u_fog_color",
			"u_time",
			"u_scattering_anistropy",
			"u_absorption_coef",
			"u_scattering_coef",
			"u_density_coef",
			"u_step_count",
			"u_emissive",
			"u_dir_light_matrices[0]",
			"u_dir_light_matrices[1]",
			"u_dir_light_matrices[2]",
			});


		// Fog texture
		Texture2DSpec fog_overlay_spec = rgba16_spec;
		fog_overlay_spec.width = Window::GetWidth() / 2;
		fog_overlay_spec.height = Window::GetHeight() / 2;
		m_fog_output_tex.SetSpec(fog_overlay_spec);

		// Setting up event listener to resize the loose textures on window resize, these are rendered to through compute shaders and not part of a FB so will not be resized unless I do this
		static Events::EventListener<Events::WindowEvent> resize_listener;
		resize_listener.OnEvent = [this](const Events::WindowEvent& t_event) {
			if (t_event.event_type == Events::Event::WINDOW_RESIZE) {
				Texture2DSpec resized_spec = m_fog_output_tex.GetSpec();
				resized_spec.width = t_event.new_window_size.x * 0.5;
				resized_spec.height = t_event.new_window_size.y * 0.5;
				m_fog_output_tex.SetSpec(resized_spec);

				resized_spec.width = t_event.new_window_size.x;
				resized_spec.height = t_event.new_window_size.y;
				m_fog_blur_tex_1.SetSpec(resized_spec);
				m_fog_blur_tex_2.SetSpec(resized_spec);

				Texture2DSpec resized_bloom_spec = m_bloom_tex.GetSpec();
				resized_bloom_spec.width = t_event.new_window_size.x * 0.5;
				resized_bloom_spec.height = t_event.new_window_size.y * 0.5;
				m_bloom_tex.SetSpec(resized_bloom_spec);

				Texture2DSpec resized_ct_spec = m_cone_trace_accum_tex.GetSpec();
				resized_ct_spec.width = t_event.new_window_size.x * 0.5;
				resized_ct_spec.height = t_event.new_window_size.y * 0.5;
				m_cone_trace_accum_tex.SetSpec(resized_ct_spec);
			}
			};


		Events::EventManager::RegisterListener(resize_listener);

		mp_bloom_downsample_shader = &mp_shader_library->CreateShader("bloom downsample");
		mp_bloom_downsample_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/BloomDownsampleCS.glsl");
		mp_bloom_downsample_shader->Init();
		mp_bloom_downsample_shader->AddUniform("u_mip_level");

		mp_bloom_upsample_shader = &mp_shader_library->CreateShader("bloom upsample");
		mp_bloom_upsample_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/BloomUpsampleCS.glsl");
		mp_bloom_upsample_shader->Init();
		mp_bloom_upsample_shader->AddUniform("u_mip_level");

		mp_bloom_threshold_shader = &mp_shader_library->CreateShader("bloom threshold");
		mp_bloom_threshold_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/BloomThresholdCS.glsl");
		mp_bloom_threshold_shader->Init();
		mp_bloom_threshold_shader->AddUniform("u_threshold");
		mp_bloom_threshold_shader->AddUniform("u_knee");

		Texture3DSpec voxel_spec_rgbaf;
		voxel_spec_rgbaf.format = GL_RGBA;
		voxel_spec_rgbaf.internal_format = GL_RGBA16F;
		voxel_spec_rgbaf.width = 256;
		voxel_spec_rgbaf.height = 256;
		voxel_spec_rgbaf.layer_count = 256;
		voxel_spec_rgbaf.storage_type = GL_FLOAT;
		voxel_spec_rgbaf.mag_filter = GL_LINEAR;
		voxel_spec_rgbaf.min_filter = GL_LINEAR;
		

		Texture3DSpec voxel_spec_r32ui = voxel_spec_rgbaf;
		voxel_spec_r32ui.format = GL_RED_INTEGER;
		voxel_spec_r32ui.internal_format = GL_R32UI;
		voxel_spec_r32ui.storage_type = GL_UNSIGNED_INT;
		voxel_spec_r32ui.min_filter = GL_NEAREST;
		voxel_spec_r32ui.mag_filter = GL_NEAREST;



		m_scene_voxel_tex_normals.SetSpec(voxel_spec_r32ui);
		m_scene_voxel_tex.SetSpec(voxel_spec_r32ui);

		Texture3DSpec voxel_mip_spec = voxel_spec_rgbaf;
		voxel_mip_spec.width = 128;
		voxel_mip_spec.height = 128;
		voxel_mip_spec.layer_count = 128;
		voxel_mip_spec.generate_mipmaps = true;
		voxel_mip_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		

		m_voxel_mip_faces.neg_x.SetSpec(voxel_mip_spec);
		m_voxel_mip_faces.neg_y.SetSpec(voxel_mip_spec);
		m_voxel_mip_faces.neg_z.SetSpec(voxel_mip_spec);
		m_voxel_mip_faces.pos_x.SetSpec(voxel_mip_spec);
		m_voxel_mip_faces.pos_y.SetSpec(voxel_mip_spec);
		m_voxel_mip_faces.pos_z.SetSpec(voxel_mip_spec);

		mp_scene_voxelization_fb = &mp_framebuffer_library->CreateFramebuffer("scene voxelization", false);
		mp_scene_voxelization_fb->Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_blue_noise_tex.GetTarget(), m_blue_noise_tex.GetTextureHandle(), 0);

		mp_cone_trace_shader = &Renderer::GetShaderLibrary().CreateShader("SR cone trace");
		mp_cone_trace_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/ConeTraceCS.glsl");
		mp_cone_trace_shader->Init();
		mp_cone_trace_shader->AddUniform("u_aligned_camera_pos");

		Texture2DSpec cone_trace_spec = low_pres_spec;
		cone_trace_spec.width = Window::GetWidth() * 0.5;
		cone_trace_spec.height = Window::GetHeight() * 0.5;
		m_cone_trace_accum_tex.SetSpec(cone_trace_spec);

	}


	void SceneRenderer::CheckResizeScreenSizeTextures(Texture2D* p_output_tex) {
		// Resize fog texture if it needs resizing
		auto& spec = p_output_tex->GetSpec();
		Texture2DSpec fog_spec = m_fog_output_tex.GetSpec();

		if (fog_spec.width != spec.width || fog_spec.height != spec.height) {
			fog_spec.width = spec.width / 2;
			fog_spec.height = spec.height / 2;
			m_fog_output_tex.SetSpec(fog_spec);

			fog_spec.width = spec.width;
			fog_spec.height = spec.height;
			m_fog_blur_tex_1.SetSpec(fog_spec);
			m_fog_blur_tex_2.SetSpec(fog_spec);

			Texture2DSpec resized_bloom_spec = m_bloom_tex.GetSpec();
			resized_bloom_spec.width = spec.width / 2;
			resized_bloom_spec.height = spec.height / 2;
			m_bloom_tex.SetSpec(resized_bloom_spec);
		}
	}

	glm::mat4 PxTransformToGlmMat4(PxTransform t) {
		auto euler = glm::degrees(glm::eulerAngles(glm::quat(t.q.w, t.q.x, t.q.y, t.q.z)));
		return ExtraMath::Init3DTranslationTransform(t.p.x, t.p.y, t.p.z) * ExtraMath::Init3DRotateTransform(euler.x, euler.y, euler.z);
	}

	void SceneRenderer::PrepRenderPasses(CameraComponent* p_cam, Texture2D* p_output_tex) {

		UpdateLightSpaceMatrices(p_cam);
		m_pointlight_system.OnUpdate(&mp_scene->m_registry);
		m_spotlight_system.OnUpdate(&mp_scene->m_registry);
		auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = p_cam_transform->GetAbsoluteTransforms()[0];
		glm::mat4 view_mat = glm::lookAt(pos, pos + p_cam_transform->forward, p_cam_transform->up);
		glm::mat4 proj_mat = p_cam->GetProjectionMatrix();
		mp_shader_library->SetCommonUBO(p_cam_transform->GetAbsoluteTransforms()[0], p_cam_transform->forward, p_cam_transform->right, p_cam_transform->up, p_output_tex->GetSpec().width, p_output_tex->GetSpec().height, p_cam->zFar, p_cam->zNear);
		mp_shader_library->SetMatrixUBOs(proj_mat, view_mat);
		mp_shader_library->SetGlobalLighting(mp_scene->directional_light);

		CheckResizeScreenSizeTextures(p_output_tex);
	}

	void SceneRenderer::UpdateLightSpaceMatrices(CameraComponent* p_cam) {
		DirectionalLight& light = mp_scene->directional_light;

		// Calculate light space matrices
		const float aspect_ratio = p_cam->aspect_ratio;
		auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = p_cam_transform->GetAbsoluteTransforms()[0];
		glm::mat4 cam_view_matrix = glm::lookAt(pos, pos + p_cam_transform->forward, p_cam_transform->up);
		const float fov = glm::radians(p_cam->fov / 2.f);

		glm::vec3 light_dir = light.GetLightDirection();
		light.m_light_space_matrices[0] = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, 0.1f, light.cascade_ranges[0]), cam_view_matrix, light_dir, light.z_mults[0], m_shadow_map_resolution);;
		light.m_light_space_matrices[1] = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, light.cascade_ranges[0] - 2.f, light.cascade_ranges[1]), cam_view_matrix, light_dir, light.z_mults[1], m_shadow_map_resolution);;
		light.m_light_space_matrices[2] = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, light.cascade_ranges[1] - 2.f, light.cascade_ranges[2]), cam_view_matrix, light_dir, light.z_mults[2], m_shadow_map_resolution);;
	}



	SceneRenderer::SceneRenderingOutput SceneRenderer::IRenderScene(const SceneRenderingSettings& settings) {
		SceneRenderer::SceneRenderingOutput output;
		auto* p_cam = settings.p_cam_override ? settings.p_cam_override : mp_scene->m_camera_system.GetActiveCamera();
		if (!p_cam) {
			ORNG_CORE_ERROR("No camera found for scene renderer to render from");
			return output;
		}

		auto& spec = settings.p_output_tex->GetSpec();
		RenderResources res;
		res.p_gbuffer_fb = m_gbuffer_fb;
		res.p_output_tex = settings.p_output_tex;

		glViewport(0, 0, spec.width, spec.height);
		PrepRenderPasses(p_cam, settings.p_output_tex);

		if (settings.do_intercept_renderpasses) {
			DoDepthPass(p_cam, settings.p_output_tex);
			RunRenderpassIntercepts(RenderpassStage::POST_DEPTH, res);
			//DoVoxelizationPass(spec.width, spec.height);
			DoGBufferPass(p_cam, settings);
			RunRenderpassIntercepts(RenderpassStage::POST_GBUFFER, res);
			DoLightingPass(settings.p_output_tex);
			RunRenderpassIntercepts(RenderpassStage::POST_LIGHTING, res);
			DoFogPass(spec.width, spec.height);
			DoTransparencyPass(settings.p_output_tex, spec.width, spec.height);
			DoPostProcessingPass(p_cam, settings.p_output_tex);
			RunRenderpassIntercepts(RenderpassStage::POST_POST_PROCESS, res);
		}
		else {
			DoDepthPass(p_cam, settings.p_output_tex);
			//DoVoxelizationPass(spec.width, spec.height);
			DoGBufferPass(p_cam, settings);
			DoLightingPass(settings.p_output_tex);
			DoFogPass(spec.width, spec.height);
			DoTransparencyPass(settings.p_output_tex, spec.width, spec.height);
			DoPostProcessingPass(p_cam, settings.p_output_tex);
		}


		return output;
	}

	void SceneRenderer::RunRenderpassIntercepts(RenderpassStage stage, const RenderResources& res) {
		std::ranges::for_each(m_render_intercepts, [&](const auto rp) {if (rp.stage == stage) { rp.func(res); }; });
	}


	void SceneRenderer::SetGBufferMaterial(ShaderVariants* p_shader, const Material* p_material) {
		if (p_material->base_color_texture) {
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->base_color_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		}
		else { // Replace with 1x1 white pixel texture
			GL_StateManager::BindTexture(GL_TEXTURE_2D, AssetManager::GetAsset<Texture2D>(ORNG_BASE_TEX_ID)->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		}

		if (p_material->roughness_texture == nullptr) {
			p_shader->SetUniform("u_roughness_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_roughness_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->roughness_texture->GetTextureHandle(), GL_StateManager::TextureUnits::ROUGHNESS);
		}

		if (p_material->metallic_texture == nullptr) {
			p_shader->SetUniform("u_metallic_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_metallic_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->metallic_texture->GetTextureHandle(), GL_StateManager::TextureUnits::METALLIC);
		}

		if (p_material->ao_texture == nullptr) {
			p_shader->SetUniform("u_ao_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_ao_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->ao_texture->GetTextureHandle(), GL_StateManager::TextureUnits::AO);
		}

		if (p_material->normal_map_texture == nullptr) {
			p_shader->SetUniform("u_normal_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_normal_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->normal_map_texture->GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP);
		}

		if (p_material->displacement_texture == nullptr) {
			p_shader->SetUniform("u_displacement_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_parallax_height_scale", p_material->parallax_height_scale);
			p_shader->SetUniform<unsigned int>("u_num_parallax_layers", p_material->parallax_layers);
			p_shader->SetUniform("u_displacement_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->displacement_texture->GetTextureHandle(), GL_StateManager::TextureUnits::DISPLACEMENT);
		}

		if (p_material->emissive_texture == nullptr) {
			p_shader->SetUniform("u_emissive_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_emissive_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->emissive_texture->GetTextureHandle(), GL_StateManager::TextureUnits::EMISSIVE);
		}

		p_shader->SetUniform("u_material.base_color", p_material->base_color);
		p_shader->SetUniform("u_material.roughness", p_material->roughness);
		p_shader->SetUniform("u_material.ao", p_material->ao);
		p_shader->SetUniform("u_material.metallic", p_material->metallic);
		p_shader->SetUniform("u_material.tile_scale", p_material->tile_scale);
		p_shader->SetUniform("u_material.emissive", (int)p_material->emissive);
		p_shader->SetUniform("u_material.emissive_strength", p_material->emissive_strength);

		p_shader->SetUniform("u_material.using_spritesheet", p_material->is_spritesheet);
		p_shader->SetUniform("u_material.sprite_data.num_rows", p_material->spritesheet_data.num_rows);
		p_shader->SetUniform("u_material.sprite_data.num_cols", p_material->spritesheet_data.num_cols);
		p_shader->SetUniform("u_material.sprite_data.fps", p_material->spritesheet_data.fps);
	}


	void SceneRenderer::RenderVehicles(ShaderVariants* p_shader, RenderGroup render_group) {
		p_shader->SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);

		for (auto [entity, vehicle] : mp_scene->m_registry.view<VehicleComponent>().each()) {
			PxShape* shapes[5];
			vehicle.m_vehicle.mPhysXState.physxActor.rigidBody->getShapes(&shapes[0], 5);
			auto pose = vehicle.m_vehicle.mPhysXState.physxActor.rigidBody->getGlobalPose();

			glm::mat4 b_scale = ExtraMath::Init3DScaleTransform(vehicle.body_scale.x, vehicle.body_scale.y, vehicle.body_scale.z);
			for (unsigned int i = 0; i < vehicle.p_body_mesh->m_submeshes.size(); i++) {
				const Material* p_material = vehicle.m_body_materials[vehicle.p_body_mesh->m_submeshes[i].material_index];

				if (p_material->render_group != render_group)
					continue;
				p_shader->SetUniform("u_transform", PxTransformToGlmMat4(pose * shapes[0]->getLocalPose()) * b_scale);
				p_shader->SetUniform<unsigned int>("u_shader_id", p_material->emissive ? ShaderLibrary::INVALID_SHADER_ID : p_material->shader_id);

				SetGBufferMaterial(p_shader, p_material);

				Renderer::DrawSubMesh(vehicle.p_body_mesh, i);
			}

			glm::mat4 w_scale = ExtraMath::Init3DScaleTransform(vehicle.wheel_scale.x, vehicle.wheel_scale.y, vehicle.wheel_scale.z);
			for (unsigned int wheel = 0; wheel < 4; wheel++) {
				for (unsigned int i = 0; i < vehicle.p_wheel_mesh->m_submeshes.size(); i++) {
					const Material* p_material = vehicle.m_wheel_materials[vehicle.p_wheel_mesh->m_submeshes[i].material_index];

					if (p_material->render_group != render_group)
						continue;

					p_shader->SetUniform("u_transform", PxTransformToGlmMat4(pose * shapes[wheel + 1]->getLocalPose()) * w_scale);
					p_shader->SetUniform<unsigned int>("u_shader_id", p_material->emissive ? ShaderLibrary::INVALID_SHADER_ID : p_material->shader_id);

					//SetGBufferMaterial(p_material);

					Renderer::DrawSubMesh(vehicle.p_wheel_mesh, i);
				}
			}
		}
	}


	unsigned convVec4ToRGBA8(glm::vec4 val) {
		return (unsigned(val.w) & 0x000000FF) << 24U
			| (unsigned(val.z) & 0x000000FF) << 16U
			| (unsigned(val.y) & 0x000000FF) << 8U
			| (unsigned(val.x) & 0x000000FF);
	}

	glm::vec4 convRGBA8ToVec4(unsigned val) {
		return glm::vec4(float((val & 0x000000FF)),
			float((val & 0x0000FF00) >> 8U),
			float((val & 0x00FF0000) >> 16U),
			float((val & 0xFF000000) >> 24U));
	}

	void SceneRenderer::DoVoxelizationPass(unsigned output_width, unsigned output_height) {
		ORNG_PROFILE_FUNC_GPU();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		GLuint clear_val = convVec4ToRGBA8({ 0, 0, 0, 0 });
		glClearTexImage(m_scene_voxel_tex.GetTextureHandle(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_val);
		glClearTexImage(m_scene_voxel_tex_normals.GetTextureHandle(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_val);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindImageTexture(0, m_scene_voxel_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		glBindImageTexture(1, m_scene_voxel_tex_normals.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		glViewport(0, 0, 256, 256);
		mp_scene_voxelization_shader->Activate(0);

		auto proj = glm::ortho(-128.f * 0.2f, 128.f * 0.2f, -128.f * 0.2f, 128.f * 0.2f, 0.2f, 256.f * 0.2f);
		auto cam_pos = glm::roundMultiple(mp_scene->GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0], glm::vec3(6.4));

		std::array<glm::mat4, 3> matrices = { glm::lookAt(cam_pos + glm::vec3(128 * 0.2, 0, 0), cam_pos, {0, 1, 0}), glm::lookAt(cam_pos + glm::vec3(0, 128 * 0.2, 0), cam_pos, {0, 0, 1}) , glm::lookAt(cam_pos + glm::vec3(0, 0, 128 * 0.2), cam_pos, {0, 1, 0}) };
		mp_scene_voxelization_shader->SetUniform("u_aligned_camera_pos", cam_pos);
		for (int i = 0; i < 3; i++) {
			mp_scene_voxelization_shader->SetUniform("u_orth_proj_view_matrix", proj * matrices[i]);
			for (auto* p_group : mp_scene->m_mesh_component_manager.GetInstanceGroups()) {
				DrawInstanceGroupGBuffer(mp_scene_voxelization_shader, p_group, SOLID);
			}
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glViewport(0, 0, output_width, output_height);
		
		// Create anisotropic mips
		constexpr unsigned num_mip_levels = 6;
		std::array<Texture3D*, 6> voxel_mips = { &m_voxel_mip_faces.pos_x, &m_voxel_mip_faces.pos_y, &m_voxel_mip_faces.pos_z, &m_voxel_mip_faces.neg_x, &m_voxel_mip_faces.neg_y, &m_voxel_mip_faces.neg_z };
		for (int i = 0; i < 6; i++) {
			for (int y = 0; y < num_mip_levels; y++) {
				glClearTexImage(voxel_mips[i]->GetTextureHandle(), y, GL_RGBA, GL_FLOAT, nullptr);
			}

			glBindImageTexture(i + 1, voxel_mips[i]->GetTextureHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		}
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindImageTexture(2, m_scene_voxel_tex_normals.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		mp_3d_mipmap_shader->Activate(MipMap3D_ShaderVariants::ANISOTROPIC);
		GL_StateManager::DispatchCompute(128 / 4, 128 / 4, 128 / 4);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		mp_3d_mipmap_shader->Activate(MipMap3D_ShaderVariants::ANISOTROPIC_CHAIN);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, voxel_mips[0]->GetTextureHandle(), GL_TEXTURE0);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, voxel_mips[1]->GetTextureHandle(), GL_TEXTURE1);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, voxel_mips[2]->GetTextureHandle(), GL_TEXTURE2);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, voxel_mips[3]->GetTextureHandle(), GL_TEXTURE3);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, voxel_mips[4]->GetTextureHandle(), GL_TEXTURE4);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, voxel_mips[5]->GetTextureHandle(), GL_TEXTURE5);

		for (int i = 1; i <= 6; i++) {
			mp_3d_mipmap_shader->SetUniform("u_mip_level", i);
			glBindImageTexture(0, voxel_mips[0]->GetTextureHandle(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glBindImageTexture(1, voxel_mips[1]->GetTextureHandle(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glBindImageTexture(2, voxel_mips[2]->GetTextureHandle(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glBindImageTexture(3, voxel_mips[3]->GetTextureHandle(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glBindImageTexture(4, voxel_mips[4]->GetTextureHandle(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glBindImageTexture(5, voxel_mips[5]->GetTextureHandle(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

			int group_dim = glm::ceil((128.f / (i + 1)) / 4.f);
			GL_StateManager::DispatchCompute(group_dim, group_dim, group_dim);
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glBindImageTexture(1, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	}

	void SceneRenderer::DoTransparencyPass(Texture2D* p_output_tex, unsigned width, unsigned height) {
		GL_StateManager::BindSSBO(mp_scene->m_particle_system.m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		mp_transparency_fb->Bind();
		mp_transparency_fb->BindTexture2D(m_gbuffer_fb->GetTexture<Texture2D>("shared_depth").GetTextureHandle(), GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shared_depth").GetTextureHandle(), GL_StateManager::TextureUnits::VIEW_DEPTH);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunci(0, GL_ONE, GL_ONE); // accumulation blend target
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR); // revealge blend target#
		glBlendEquation(GL_FUNC_ADD);

		static auto filler_0 = glm::vec4(0); static auto filler_1 = glm::vec4(1);
		glClearBufferfv(GL_COLOR, 0, &filler_0[0]);
		glClearBufferfv(GL_COLOR, 1, &filler_1[0]);

		mp_transparency_shader_variants->Activate(DEFAULT);
		mp_transparency_shader_variants->SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);
		//Draw all meshes in scene (instanced)
		for (const auto* group : mp_scene->m_mesh_component_manager.GetInstanceGroups()) {
			DrawInstanceGroupGBuffer(mp_transparency_shader_variants, group, RenderGroup::ALPHA_TESTED);
		}

		mp_transparency_shader_variants->Activate(T_PARTICLE);
		for (auto [entity, emitter, res] : mp_scene->m_registry.view<ParticleEmitterComponent, ParticleMeshResources>().each()) {
			mp_transparency_shader_variants->SetUniform("u_transform_start_index", emitter.m_particle_start_index);
			IDrawMeshGBuffer(mp_transparency_shader_variants, res.p_mesh, ALPHA_TESTED, emitter.GetNbParticles(), &res.materials[0]);
		}

		mp_transparency_shader_variants->Activate(T_PARTICLE_BILLBOARD);
		auto* p_quad_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_QUAD_ID);
		for (auto [entity, emitter, res] : mp_scene->m_registry.view<ParticleEmitterComponent, ParticleBillboardResources>().each()) {
			mp_transparency_shader_variants->SetUniform("u_transform_start_index", emitter.m_particle_start_index);

			IDrawMeshGBuffer(mp_transparency_shader_variants, p_quad_mesh, ALPHA_TESTED, emitter.GetNbParticles(), &res.p_material);
		}


		//RenderVehicles(mp_transparency_shader_variants, RenderGroup::ALPHA_TESTED);

		glEnable(GL_CULL_FACE);


		mp_composition_fb->Bind();
		mp_composition_fb->BindTexture2D(p_output_tex->GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		mp_transparency_composite_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shared_depth").GetTextureHandle(), GL_StateManager::TextureUnits::VIEW_DEPTH);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_transparency_fb->GetTexture<Texture2D>("accum").GetTextureHandle(), GL_TEXTURE0);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, mp_transparency_fb->GetTexture<Texture2D>("revealage").GetTextureHandle(), GL_TEXTURE1);
		Renderer::DrawQuad();
		mp_composition_fb->BindTexture2D(0, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
		glDepthMask(GL_TRUE);
	}

	void SceneRenderer::DrawInstanceGroupGBuffer(ShaderVariants* p_shader, const MeshInstanceGroup* group, RenderGroup render_group) {
		GL_StateManager::BindSSBO(group->m_transform_ssbo.GetHandle(), 0);
		IDrawMeshGBuffer(p_shader, group->m_mesh_asset, render_group, group->GetInstanceCount(), group->m_materials.data());
	}

	void SceneRenderer::IDrawMeshGBuffer(ShaderVariants* p_shader, const MeshAsset* p_mesh, RenderGroup render_group, unsigned instances, const Material* const* materials) {
		for (unsigned int i = 0; i < p_mesh->m_submeshes.size(); i++) {
			const Material* p_material = materials[p_mesh->m_submeshes[i].material_index];

			if (p_material->render_group != render_group)
				continue;

			p_shader->SetUniform<unsigned int>("u_shader_id", p_material->emissive ? ShaderLibrary::INVALID_SHADER_ID : p_material->shader_id);

			SetGBufferMaterial(p_shader, p_material);

			Renderer::DrawSubMeshInstanced(p_mesh, instances, i);
		}
	}

	void SceneRenderer::DoGBufferPass(CameraComponent* p_cam, const SceneRenderingSettings& settings) {
		ORNG_PROFILE_FUNC_GPU();
		m_gbuffer_fb->Bind();

		GL_StateManager::DefaultClearBits();
		GL_StateManager::ClearBitsUnsignedInt();

		using enum GBufferVariants;
		mp_gbuffer_shader_variants->Activate(MESH);
		mp_gbuffer_shader_variants->SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);

		if (settings.render_meshes) {
			//Draw all meshes in scene (instanced)
			for (const auto* group : mp_scene->m_mesh_component_manager.GetInstanceGroups()) {
				DrawInstanceGroupGBuffer(mp_gbuffer_shader_variants, group, SOLID);
			}


			mp_gbuffer_shader_variants->Activate(BILLBOARD);
			for (const auto* group : mp_scene->m_mesh_component_manager.GetBillboardInstanceGroups()) {
				DrawInstanceGroupGBuffer(mp_gbuffer_shader_variants, group, SOLID);
			}

			//RenderVehicles(mp_gbuffer_shader_mesh_bufferless, RenderGroup::SOLID);
			GL_StateManager::BindSSBO(mp_scene->m_particle_system.m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
			mp_gbuffer_shader_variants->Activate(PARTICLE);
			for (auto [entity, emitter, res] : mp_scene->m_registry.view<ParticleEmitterComponent, ParticleMeshResources>().each()) {
				mp_gbuffer_shader_variants->SetUniform("u_transform_start_index", emitter.m_particle_start_index);
				IDrawMeshGBuffer(mp_gbuffer_shader_variants, res.p_mesh, SOLID, emitter.GetNbParticles(), &res.materials[0]);
			}

			mp_gbuffer_shader_variants->Activate(PARTICLE_BILLBOARD);
			auto* p_quad_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_QUAD_ID);
			for (auto [entity, emitter, res] : mp_scene->m_registry.view<ParticleEmitterComponent, ParticleBillboardResources>().each()) {
				mp_gbuffer_shader_variants->SetUniform("u_transform_start_index", emitter.m_particle_start_index);

				IDrawMeshGBuffer(mp_gbuffer_shader_variants, p_quad_mesh, SOLID, emitter.GetNbParticles(), &res.p_material);
			}
		}

#ifdef ORNG_EDITOR_LAYER
		if (!((int)settings.voxel_render_face & (int)VoxelRenderFace::NONE)) {
			if ((int)settings.voxel_render_face & (int)VoxelRenderFace::BASE) {
				glBindImageTexture(0, m_scene_voxel_tex.GetTextureHandle(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
			}
			else {
				std::array<Texture3D*, 6> voxel_mips = { &m_voxel_mip_faces.pos_x, &m_voxel_mip_faces.pos_y, &m_voxel_mip_faces.pos_z, &m_voxel_mip_faces.neg_x, &m_voxel_mip_faces.neg_y, &m_voxel_mip_faces.neg_z };
				glBindImageTexture(0, voxel_mips[glm::log2((int)settings.voxel_render_face) - 2]->GetTextureHandle(), settings.voxel_mip_layer, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
			}

			mp_voxel_debug_shader->ActivateProgram();
			mp_voxel_debug_shader->SetUniform("u_aligned_camera_pos", glm::roundMultiple(mp_scene->GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0], glm::vec3(6.4)));
			Renderer::DrawMeshInstanced(AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID), glm::pow(256 / (settings.voxel_mip_layer + 1), 3));
		}
#endif	


		/* uniforms */
		mp_gbuffer_shader_variants->Activate(TERRAIN);
		SetGBufferMaterial(mp_gbuffer_shader_variants, mp_scene->terrain.mp_material);
		mp_gbuffer_shader_variants->SetUniform<unsigned int>("u_shader_id", m_lighting_shader->GetID());
		DrawTerrain(p_cam);

		mp_gbuffer_shader_variants->Activate(SKYBOX);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, mp_scene->skybox.GetSkyboxTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_CUBEMAP, false);
		DrawSkybox();
	}





	void SceneRenderer::DoDepthPass(CameraComponent* p_cam, Texture2D* p_output_tex) {
		ORNG_PROFILE_FUNC_GPU();

		if (mp_scene->directional_light.shadows_enabled) {
			// Render cascades
			m_depth_fb->Bind();
			mp_orth_depth_shader->ActivateProgram();
			for (int i = 0; i < 3; i++) {
				glViewport(0, 0, m_shadow_map_resolution, m_shadow_map_resolution);
				m_depth_fb->BindTextureLayerToFBAttachment(m_directional_light_depth_tex.GetTextureHandle(), GL_DEPTH_ATTACHMENT, i);
				GL_StateManager::ClearDepthBits();

				mp_orth_depth_shader->SetUniform("u_light_pv_matrix", mp_scene->directional_light.m_light_space_matrices[i]);
				DrawAllMeshes(SOLID);
			}
		}

		// Spotlights
		glViewport(0, 0, 512, 512);
		mp_persp_depth_shader->ActivateProgram();
		auto spotlights = mp_scene->m_registry.view<SpotLightComponent, TransformComponent>();

		int index = 0;
		for (auto [entity, light, transform] : spotlights.each()) {
			if (!light.shadows_enabled)
				continue;
			
			m_depth_fb->BindTextureLayerToFBAttachment(m_spotlight_system.m_spotlight_depth_tex.GetTextureHandle(), GL_DEPTH_ATTACHMENT, index++);
			GL_StateManager::ClearDepthBits();

			mp_persp_depth_shader->SetUniform("u_light_pv_matrix", light.GetLightSpaceTransform());
			mp_persp_depth_shader->SetUniform("u_light_pos", transform.GetAbsoluteTransforms()[0]);
			DrawAllMeshes(SOLID);
		}

		// Pointlights
		index = 0;
		glViewport(0, 0, 512, 512);
		mp_pointlight_depth_shader->ActivateProgram();
		auto pointlights = mp_scene->m_registry.view<PointLightComponent, TransformComponent>();

		for (auto [entity, pointlight, transform] : pointlights.each()) {
			if (!pointlight.shadows_enabled)
				continue;
			glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, pointlight.shadow_distance);
			glm::vec3 light_pos = transform.GetAbsoluteTransforms()[0];

			std::array<glm::mat4, 6> captureViews =
			{
			   glm::lookAt(light_pos, light_pos + glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			   glm::lookAt(light_pos, light_pos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
			};

			mp_pointlight_depth_shader->SetUniform("u_light_pos", light_pos);
			mp_pointlight_depth_shader->SetUniform("u_light_zfar", pointlight.shadow_distance);

			// Draw depth cubemap
			for (int i = 0; i < 6; i++) {
				m_depth_fb->BindTextureLayerToFBAttachment(m_pointlight_system.m_pointlight_depth_tex.GetTextureHandle(), GL_DEPTH_ATTACHMENT, index * 6 + i);
				GL_StateManager::ClearDepthBits();

				mp_pointlight_depth_shader->SetUniform("u_light_pv_matrix", captureProjection * captureViews[i]);
				DrawAllMeshes(SOLID);
			}

			index++;
		}

		glViewport(0, 0, p_output_tex->GetSpec().width, p_output_tex->GetSpec().height);
	}



	void SceneRenderer::DrawSkybox() {
		glDisable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);

		Renderer::DrawCube();

		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
	}






	void SceneRenderer::DoFogPass(unsigned int width, unsigned int height) {
		ORNG_PROFILE_FUNC_GPU();
		//draw fog texture
		m_fog_shader->ActivateProgram();

		m_fog_shader->SetUniform("u_scattering_coef", mp_scene->post_processing.global_fog.scattering_coef);
		m_fog_shader->SetUniform("u_absorption_coef", mp_scene->post_processing.global_fog.absorption_coef);
		m_fog_shader->SetUniform("u_density_coef", mp_scene->post_processing.global_fog.density_coef);
		m_fog_shader->SetUniform("u_scattering_anistropy", mp_scene->post_processing.global_fog.scattering_anistropy);
		m_fog_shader->SetUniform("u_fog_color", mp_scene->post_processing.global_fog.color);
		m_fog_shader->SetUniform("u_step_count", mp_scene->post_processing.global_fog.step_count);
		m_fog_shader->SetUniform("u_time", static_cast<float>(FrameTiming::GetTotalElapsedTime()));
		m_fog_shader->SetUniform("u_emissive", mp_scene->post_processing.global_fog.emissive_factor);

		GL_StateManager::BindTexture(GL_TEXTURE_3D, mp_scene->post_processing.global_fog.fog_noise->GetTextureHandle(), GL_StateManager::TextureUnits::DATA_3D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shared_depth").GetTextureHandle(), GL_StateManager::TextureUnits::DEPTH, false);


		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_output_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)width / 16.f), (GLuint)glm::ceil((float)height / 16.f), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//Upsample fog texture
		mp_depth_aware_upsample_sv->Activate((unsigned)DepthAwareUpsampleSV::DEFAULT);
		GL_StateManager::BindTexture(
			GL_TEXTURE_2D, m_fog_output_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3
		);

		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_blur_tex_1.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)width / 8.f), (GLuint)glm::ceil((float)height / 8.f), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//blur fog texture
		m_blur_shader->ActivateProgram();

		for (int i = 0; i < 2; i++) {
			m_blur_shader->SetUniform("u_horizontal", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, m_fog_blur_tex_1.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);
			glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_blur_tex_2.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glDispatchCompute((GLuint)glm::ceil((float)width / 8.f), (GLuint)glm::ceil((float)height / 8.f), 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			m_blur_shader->SetUniform("u_horizontal", 0);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, m_fog_blur_tex_2.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3);
			glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_blur_tex_1.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glDispatchCompute((GLuint)glm::ceil((float)width / 8.f), (GLuint)glm::ceil((float)height / 8.f), 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}





	void SceneRenderer::DoLightingPass(Texture2D* p_output_tex) {
		ORNG_PROFILE_FUNC_GPU();

		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_directional_light_depth_tex.GetTextureHandle(), GL_StateManager::TextureUnits::DIR_SHADOW_MAP, true);
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_spotlight_system.m_spotlight_depth_tex.GetTextureHandle(), GL_StateManager::TextureUnits::SPOT_SHADOW_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("albedo").GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("normals").GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shared_depth").GetTextureHandle(), GL_StateManager::TextureUnits::DEPTH, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shader_ids").GetTextureHandle(), GL_StateManager::TextureUnits::SHADER_IDS, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("roughness_metallic_ao").GetTextureHandle(), GL_StateManager::TextureUnits::ROUGHNESS_METALLIC_AO, false);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, mp_scene->skybox.GetIrradianceTexture().GetTextureHandle(), GL_StateManager::TextureUnits::DIFFUSE_PREFILTER, false);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, mp_scene->skybox.GetSpecularPrefilter().GetTextureHandle(), GL_StateManager::TextureUnits::SPECULAR_PREFILTER, false);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_pointlight_system.m_pointlight_depth_tex.GetTextureHandle(), GL_StateManager::TextureUnits::POINTLIGHT_DEPTH, false);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_scene_voxel_tex.GetTextureHandle(), GL_StateManager::TextureUnits::SCENE_VOXELIZATION, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_blue_noise_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLUE_NOISE, false);


		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_voxel_mip_faces.pos_x.GetTextureHandle(), GL_TEXTURE5, false);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_voxel_mip_faces.pos_y.GetTextureHandle(), GL_TEXTURE6, false);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_voxel_mip_faces.pos_z.GetTextureHandle(), GL_TEXTURE8, false);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_voxel_mip_faces.neg_x.GetTextureHandle(), GL_TEXTURE9, false);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_voxel_mip_faces.neg_y.GetTextureHandle(), GL_TEXTURE10, false);
		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_voxel_mip_faces.neg_z.GetTextureHandle(), GL_TEXTURE13, false);


		m_lighting_shader->ActivateProgram();
		m_lighting_shader->SetUniform("u_aligned_camera_pos", glm::roundMultiple(mp_scene->GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0], glm::vec3(6.4)));

		auto& spec = p_output_tex->GetSpec();
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output_tex->GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		GL_StateManager::DispatchCompute((GLuint)glm::ceil((float)spec.width / 8.f), (GLuint)glm::ceil((float)spec.height / 8.f), 1);

#ifdef VOXEL_GI
		// Cone trace at half res
		glClearTexImage(m_cone_trace_accum_tex.GetTextureHandle(), 0, GL_RGBA, GL_FLOAT, nullptr);
		mp_cone_trace_shader->ActivateProgram();
		mp_cone_trace_shader->SetUniform("u_aligned_camera_pos", glm::roundMultiple(mp_scene->GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0], glm::vec3(6.4)));
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_cone_trace_accum_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		GL_StateManager::DispatchCompute((GLuint)glm::ceil((float)spec.width / 16.f), (GLuint)glm::ceil((float)spec.height / 16.f), 1);

		glBindImageTexture(7, m_cone_trace_accum_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		mp_depth_aware_upsample_sv->Activate((unsigned)DepthAwareUpsampleSV::CONE_TRACE);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output_tex->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_cone_trace_accum_tex.GetTextureHandle(), GL_TEXTURE23, false);
		GL_StateManager::DispatchCompute((GLuint)glm::ceil((float)spec.width / 8.f), (GLuint)glm::ceil((float)spec.height / 8.f), 1);
#endif


	}





	void SceneRenderer::DrawTerrain(CameraComponent* p_cam) {
		std::vector<TerrainQuadtree*> node_array;

		mp_scene->terrain.m_quadtree->QueryChunks(node_array, p_cam->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0], mp_scene->terrain.m_width);
		for (auto& node : node_array) {
			const TerrainChunk* chunk = node->GetChunk();
			if (chunk->m_bounding_box.IsOnFrustum(p_cam->view_frustum)) {
				Renderer::DrawVAO_Elements(GL_TRIANGLES, chunk->m_vao);
			}
		}
	}




	void SceneRenderer::DoBloomPass(unsigned int width, unsigned int height) {
		ORNG_PROFILE_FUNC_GPU();
		mp_bloom_threshold_shader->ActivateProgram();
		mp_bloom_threshold_shader->SetUniform("u_threshold", mp_scene->post_processing.bloom.threshold);
		mp_bloom_threshold_shader->SetUniform("u_knee", mp_scene->post_processing.bloom.knee);
		// Isolate bright spots
		glBindImageTexture(0, m_bloom_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)width / 16.f), (GLuint)glm::ceil((float)height / 16.f), 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// Downsample passes
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_bloom_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLOOM, true);
		const int max_mip_layer = 6;
		mp_bloom_downsample_shader->ActivateProgram();
		for (int i = 1; i < max_mip_layer + 1; i++) {
			mp_bloom_downsample_shader->SetUniform("u_mip_level", i);
			glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
			glDispatchCompute((GLuint)glm::ceil(((float)width / 32.f) / (float)i), (GLuint)glm::ceil(((float)height / 32.f) / (float)i), 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		// Upsample passes
		mp_bloom_upsample_shader->ActivateProgram();
		for (int i = max_mip_layer - 1; i >= 0; i--) {
			mp_bloom_upsample_shader->SetUniform("u_mip_level", i);
			glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
			glDispatchCompute((GLuint)glm::ceil(((float)width / 16.f) / (float)(i + 1)), (GLuint)glm::ceil(((float)height / 16.f) / (float)(i + 1)), 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}
	}



	void SceneRenderer::DoPostProcessingPass(CameraComponent* p_cam, Texture2D* p_output_tex) {
		GL_StateManager::BindTexture(GL_TEXTURE_2D, p_output_tex->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		auto& spec = p_output_tex->GetSpec();
		DoBloomPass(spec.width, spec.height);

		ORNG_PROFILE_FUNC_GPU();
		m_post_process_shader->ActivateProgram();

		m_post_process_shader->SetUniform("exposure", p_cam->exposure);
		m_post_process_shader->SetUniform("u_bloom_intensity", mp_scene->post_processing.bloom.intensity);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_fog_blur_tex_1.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_2, false);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, p_output_tex->GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)spec.width / 8.f), (GLuint)glm::ceil((float)spec.height / 8.f), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}



	void SceneRenderer::DrawAllMeshes(RenderGroup render_group) const {
		for (const auto* group : mp_scene->m_mesh_component_manager.GetInstanceGroups()) {
			const MeshAsset* mesh_data = group->GetMeshAsset();
			GL_StateManager::BindSSBO(group->m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::TRANSFORMS);

			for (unsigned int i = 0; i < group->m_mesh_asset->m_submeshes.size(); i++) {
				const Material* p_material = group->m_materials[group->m_mesh_asset->m_submeshes[i].material_index];
				if (p_material->render_group != render_group)
					continue;

				Renderer::DrawSubMeshInstanced(group->m_mesh_asset, group->GetInstanceCount(), i);
			}
		}
	}
}