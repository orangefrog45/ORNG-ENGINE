#include "pch/pch.h"

#include "rendering/SceneRenderer.h"
#include "shaders/ShaderLibrary.h"
#include "core/Window.h"
#include "rendering/Renderer.h"
#include "framebuffers/FramebufferLibrary.h"
#include "core/GLStateManager.h"
#include "scene/Scene.h"
#include "components/CameraComponent.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "events/EventManager.h"
#include "util/Timers.h"
#include "scene/SceneEntity.h"
#include "core/CodedAssets.h"


namespace ORNG {

	void SceneRenderer::I_Init() {

		mp_shader_library = &Renderer::GetShaderLibrary();
		mp_framebuffer_library = &Renderer::GetFramebufferLibrary();

		m_gbuffer_shader = &mp_shader_library->CreateShader("gbuffer");
		m_gbuffer_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::GBufferVS);
		m_gbuffer_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::GBufferFS);
		m_gbuffer_shader->Init();
		m_gbuffer_shader->AddUniforms(std::vector<std::string> {
			"u_terrain_mode",
				"u_skybox_mode",
				"u_roughness_sampler_active",
				"u_metallic_sampler_active",
				"u_emissive_sampler_active",
				"u_normal_sampler_active",
				"u_ao_sampler_active",
				"u_displacement_sampler_active",
				"u_num_parallax_layers",
				"u_parallax_height_scale",
				"u_material.base_color_and_metallic",
				"u_material.roughness",
				"u_material.ao",
				"u_material.tile_scale",
				"u_material.emissive",
				"u_material.emissive_strength",
				"u_bloom_threshold",
				"u_material_id",
				"u_shader_id",
		});


		m_lighting_shader = &mp_shader_library->CreateShader("lighting", ShaderLibrary::LIGHTING_SHADER_ID);
		m_lighting_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::LightingVS);
		m_lighting_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::LightingFS);
		m_lighting_shader->Init();
		m_lighting_shader->AddUniforms({
			"u_terrain_mode",

			"u_num_point_lights",
			"u_num_spot_lights",

			"u_dir_light_matrices[0]",
			"u_dir_light_matrices[1]",
			"u_dir_light_matrices[2]",
			});



		// Render quad
		m_post_process_shader = &mp_shader_library->CreateShader("post_process");
		m_post_process_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::QuadVS);
		m_post_process_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::PostProcessFS);
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

		m_post_process_shader->SetUniform("quad_sampler", GL_StateManager::TextureUnitIndexes::COLOUR);
		m_post_process_shader->SetUniform("world_position_sampler", GL_StateManager::TextureUnitIndexes::WORLD_POSITIONS);

		// blue noise for post-processing effects in quad
		Texture2DSpec noise_spec;
		noise_spec.filepath = "./res/textures/BlueNoise64Tiled.png";
		noise_spec.min_filter = GL_NEAREST;
		noise_spec.mag_filter = GL_NEAREST;
		noise_spec.wrap_params = GL_REPEAT;
		noise_spec.storage_type = GL_UNSIGNED_BYTE;

		m_blue_noise_tex.SetSpec(noise_spec);
		m_blue_noise_tex.LoadFromFile();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_blue_noise_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLUE_NOISE, false);


		mp_orth_depth_shader = &mp_shader_library->CreateShader("orth_depth");
		mp_orth_depth_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::DepthVS);
		mp_orth_depth_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::DepthFS, { "ORTHOGRAPHIC" });
		mp_orth_depth_shader->Init();
		mp_orth_depth_shader->AddUniform("u_light_pv_matrix");

		mp_persp_depth_shader = &mp_shader_library->CreateShader("persp_depth");
		mp_persp_depth_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::DepthVS);
		mp_persp_depth_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::DepthFS, { "PERSPECTIVE" });
		mp_persp_depth_shader->Init();
		mp_persp_depth_shader->AddUniform("u_light_pv_matrix");

		mp_pointlight_depth_shader = &mp_shader_library->CreateShader("pointlight_depth");
		mp_pointlight_depth_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::DepthVS);
		mp_pointlight_depth_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::DepthFS, { "PERSPECTIVE", "POINTLIGHT" });
		mp_pointlight_depth_shader->Init();
		mp_pointlight_depth_shader->AddUniform("u_light_pv_matrix");
		mp_pointlight_depth_shader->AddUniform("u_light_pos");
		mp_pointlight_depth_shader->AddUniform("u_light_zfar");


		m_blur_shader = &mp_shader_library->CreateShader("blur");
		m_blur_shader->AddStageFromString(GL_COMPUTE_SHADER, CodedAssets::BlurFS);
		m_blur_shader->Init();
		m_blur_shader->AddUniform("u_horizontal");
		m_blur_shader->AddUniform("u_first_iter");


		/* LIGHTING FB */
		Texture2DSpec color_render_texture_spec;
		color_render_texture_spec.format = GL_RGB;
		color_render_texture_spec.internal_format = GL_RGB16F;
		color_render_texture_spec.storage_type = GL_UNSIGNED_BYTE;
		color_render_texture_spec.mag_filter = GL_LINEAR;
		color_render_texture_spec.min_filter = GL_LINEAR;
		color_render_texture_spec.width = Window::GetWidth();
		color_render_texture_spec.height = Window::GetHeight();
		color_render_texture_spec.wrap_params = GL_CLAMP_TO_EDGE;

		m_lighting_fb = &mp_framebuffer_library->CreateFramebuffer("lighting", true);
		m_lighting_fb->AddRenderbuffer(Window::GetWidth(), Window::GetHeight());
		m_lighting_fb->Add2DTexture("render_texture", GL_COLOR_ATTACHMENT0, color_render_texture_spec);

		/* GBUFFER FB */
		m_gbuffer_fb = &mp_framebuffer_library->CreateFramebuffer("gbuffer", true);

		Texture2DSpec gbuffer_spec_1;
		gbuffer_spec_1.format = GL_RGB;
		gbuffer_spec_1.internal_format = GL_RGB32F;
		gbuffer_spec_1.storage_type = GL_FLOAT;
		gbuffer_spec_1.width = Window::GetWidth();
		gbuffer_spec_1.height = Window::GetHeight();

		Texture2DSpec gbuffer_spec_2;
		gbuffer_spec_2.format = GL_RED_INTEGER;
		gbuffer_spec_2.internal_format = GL_R16UI;
		gbuffer_spec_2.storage_type = GL_UNSIGNED_INT;
		gbuffer_spec_2.width = Window::GetWidth();
		gbuffer_spec_2.height = Window::GetHeight();


		Texture2DSpec low_pres_spec;
		low_pres_spec.format = GL_RGB;
		low_pres_spec.internal_format = GL_RGB16F;
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

		m_gbuffer_fb->Add2DTexture("world_positions", GL_COLOR_ATTACHMENT0, gbuffer_spec_1);
		m_gbuffer_fb->Add2DTexture("normals", GL_COLOR_ATTACHMENT1, low_pres_spec);
		m_gbuffer_fb->Add2DTexture("albedo", GL_COLOR_ATTACHMENT2, low_pres_spec);
		m_gbuffer_fb->Add2DTexture("roughness_metallic_ao", GL_COLOR_ATTACHMENT3, low_pres_spec);
		m_gbuffer_fb->Add2DTexture("shader_ids", GL_COLOR_ATTACHMENT4, gbuffer_spec_2);
		m_gbuffer_fb->Add2DTexture("shared_depth", GL_DEPTH_ATTACHMENT, gbuffer_depth_spec);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
		m_gbuffer_fb->EnableDrawBuffers(5, buffers);


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
		m_depth_fb->Add2DTextureArray("dir_depth", depth_spec);
		m_depth_fb->Add2DTextureArray("spotlight_depth", spotlight_depth_spec);



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
		m_fog_shader->AddStageFromString(GL_COMPUTE_SHADER, CodedAssets::FogCS);
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
				resized_spec.width = t_event.new_window_size.x / 2;
				resized_spec.height = t_event.new_window_size.y / 2;
				m_fog_output_tex.SetSpec(resized_spec);

				resized_spec.width = t_event.new_window_size.x;
				resized_spec.height = t_event.new_window_size.y;
				m_fog_blur_tex_1.SetSpec(resized_spec);
				m_fog_blur_tex_2.SetSpec(resized_spec);

				Texture2DSpec resized_bloom_spec = m_bloom_tex.GetSpec();
				resized_bloom_spec.width = t_event.new_window_size.x / 2;
				resized_bloom_spec.height = t_event.new_window_size.y / 2;
				m_bloom_tex.SetSpec(resized_bloom_spec);
			}
		};
		Events::EventManager::RegisterListener(resize_listener);


		// POST PROCESSING FB
		m_post_processing_fb = &mp_framebuffer_library->CreateFramebuffer("post_processing", true);
		m_post_processing_fb->AddRenderbuffer(Window::GetWidth(), Window::GetHeight());
		m_post_processing_fb->Add2DTexture("shared_render_texture", GL_COLOR_ATTACHMENT0, color_render_texture_spec);

		mp_bloom_downsample_shader = &mp_shader_library->CreateShader("bloom downsample");
		mp_bloom_downsample_shader->AddStageFromString(GL_COMPUTE_SHADER, CodedAssets::BloomDownsampleCS);
		mp_bloom_downsample_shader->Init();
		mp_bloom_downsample_shader->AddUniform("u_mip_level");

		mp_bloom_upsample_shader = &mp_shader_library->CreateShader("bloom upsample");
		mp_bloom_upsample_shader->AddStageFromString(GL_COMPUTE_SHADER, CodedAssets::BloomUpsampleCS);
		mp_bloom_upsample_shader->Init();
		mp_bloom_upsample_shader->AddUniform("u_mip_level");

		mp_bloom_threshold_shader = &mp_shader_library->CreateShader("bloom threshold");
		mp_bloom_threshold_shader->AddStageFromString(GL_COMPUTE_SHADER, CodedAssets::BloomThresholdCS);
		mp_bloom_threshold_shader->Init();
		mp_bloom_threshold_shader->AddUniform("u_threshold");
		mp_bloom_threshold_shader->AddUniform("u_knee");
	}




	void SceneRenderer::IPrepRenderPasses(CameraComponent* p_cam) {
		glm::mat4 view_mat = p_cam->GetViewMatrix();
		glm::mat4 proj_mat = p_cam->GetProjectionMatrix();
		mp_shader_library->SetCommonUBO(p_cam->GetEntity()->GetComponent<TransformComponent>()->GetPosition(), p_cam->target);
		mp_shader_library->SetMatrixUBOs(proj_mat, view_mat);
		mp_shader_library->SetGlobalLighting(mp_scene->m_directional_light);
	}





	SceneRenderer::SceneRenderingOutput SceneRenderer::IRenderScene(const SceneRenderingSettings& settings) {

		SceneRenderer::SceneRenderingOutput output;
		auto* p_cam = settings.p_cam_override ? settings.p_cam_override : mp_scene->m_camera_system.GetActiveCamera();
		if (!p_cam) {
			output.final_color_texture_handle = m_post_processing_fb->GetTexture<Texture2D>("shared_render_texture").GetTextureHandle();
			return output;
		}

		IPrepRenderPasses(p_cam);

		IDoGBufferPass(p_cam);
		IDoDepthPass(p_cam);
		IDoLightingPass();
		IDoFogPass();
		IDoPostProcessingPass(p_cam);
		output.final_color_texture_handle = m_post_processing_fb->GetTexture<Texture2D>("shared_render_texture").GetTextureHandle();



		return output;
	}



	void SceneRenderer::SetGBufferMaterial(const Material* p_material) {
		if (p_material->base_color_texture) {
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->base_color_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		}
		else { // Replace with 1x1 white pixel texture
			GL_StateManager::BindTexture(GL_TEXTURE_2D, CodedAssets::GetBaseTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		}

		if (p_material->roughness_texture == nullptr) {
			m_gbuffer_shader->SetUniform("u_roughness_sampler_active", 0);
		}
		else {
			m_gbuffer_shader->SetUniform("u_roughness_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->roughness_texture->GetTextureHandle(), GL_StateManager::TextureUnits::ROUGHNESS);
		}

		if (p_material->metallic_texture == nullptr) {
			m_gbuffer_shader->SetUniform("u_metallic_sampler_active", 0);
		}
		else {
			m_gbuffer_shader->SetUniform("u_metallic_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->metallic_texture->GetTextureHandle(), GL_StateManager::TextureUnits::METALLIC);
		}

		if (p_material->ao_texture == nullptr) {
			m_gbuffer_shader->SetUniform("u_ao_sampler_active", 0);
		}
		else {
			m_gbuffer_shader->SetUniform("u_ao_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->ao_texture->GetTextureHandle(), GL_StateManager::TextureUnits::AO);
		}

		if (p_material->normal_map_texture == nullptr) {
			m_gbuffer_shader->SetUniform("u_normal_sampler_active", 0);
		}
		else {
			m_gbuffer_shader->SetUniform("u_normal_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->normal_map_texture->GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP);
		}

		if (p_material->displacement_texture == nullptr) {
			m_gbuffer_shader->SetUniform("u_displacement_sampler_active", 0);
		}
		else {
			m_gbuffer_shader->SetUniform("u_parallax_height_scale", p_material->parallax_height_scale);
			m_gbuffer_shader->SetUniform<unsigned int>("u_num_parallax_layers", p_material->parallax_layers);
			m_gbuffer_shader->SetUniform("u_displacement_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->displacement_texture->GetTextureHandle(), GL_StateManager::TextureUnits::DISPLACEMENT);
		}

		if (p_material->emissive_texture == nullptr) {
			m_gbuffer_shader->SetUniform("u_emissive_sampler_active", 0);
		}
		else {
			m_gbuffer_shader->SetUniform("u_emissive_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->emissive_texture->GetTextureHandle(), GL_StateManager::TextureUnits::EMISSIVE);
		}

		m_gbuffer_shader->SetUniform("u_material.base_color_and_metallic", glm::vec4(p_material->base_color, p_material->metallic));
		m_gbuffer_shader->SetUniform("u_material.roughness", p_material->roughness);
		m_gbuffer_shader->SetUniform("u_material.ao", p_material->ao);
		m_gbuffer_shader->SetUniform("u_material.tile_scale", p_material->tile_scale);
		m_gbuffer_shader->SetUniform("u_material.emissive", (int)p_material->emissive);
		m_gbuffer_shader->SetUniform("u_material.emissive_strength", p_material->emissive_strength);

	}




	void SceneRenderer::IDoGBufferPass(CameraComponent* p_cam) {
		ORNG_PROFILE_FUNC_GPU();
		m_gbuffer_fb->Bind();
		GL_StateManager::DefaultClearBits();
		GL_StateManager::ClearBitsUnsignedInt();

		m_gbuffer_shader->ActivateProgram();
		m_gbuffer_shader->SetUniform("u_bloom_threshold", mp_scene->post_processing.bloom.threshold);

		//Draw all meshes in scene (instanced)
		for (const auto* group : mp_scene->m_mesh_component_manager.GetInstanceGroups()) {

			GL_StateManager::BindSSBO(group->m_transform_ssbo_handle, 0);

			for (unsigned int i = 0; i < group->m_mesh_asset->m_submeshes.size(); i++) {

				const Material* p_material = group->m_materials[group->m_mesh_asset->m_submeshes[i].material_index];
				m_gbuffer_shader->SetUniform<unsigned int>("u_shader_id", p_material->emissive ? ShaderLibrary::INVALID_SHADER_ID : p_material->shader_id);

				SetGBufferMaterial(p_material);

				Renderer::DrawSubMeshInstanced(group->m_mesh_asset, group->GetInstanceCount(), i);
			}
		}



		/* uniforms */
		SetGBufferMaterial(mp_scene->terrain.mp_material);
		m_gbuffer_shader->SetUniform("u_terrain_mode", 1);
		m_gbuffer_shader->SetUniform<unsigned int>("u_shader_id", m_lighting_shader->GetID());
		IDrawTerrain(p_cam);
		m_gbuffer_shader->SetUniform("u_terrain_mode", 0);

		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, mp_scene->skybox.GetSkyboxTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_CUBEMAP, false);
		m_gbuffer_shader->SetUniform("u_skybox_mode", 1);
		m_gbuffer_shader->SetUniform<unsigned int>("u_shader_id", ShaderLibrary::INVALID_SHADER_ID);
		DrawSkybox();
		m_gbuffer_shader->SetUniform("u_skybox_mode", 0);


		/*if (sample_world_pos) {sc
			glm::vec2 mouse_coords = glm::min(glm::max(Window::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

			GLfloat* pixels = new GLfloat[4];
			glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RGB, GL_FLOAT, pixels);
			m_sampled_world_pos = glm::vec3(pixels[0], pixels[1], pixels[2]);
			delete[] pixels;
		}*/
	}





	void SceneRenderer::IDoDepthPass(CameraComponent* p_cam) {
		ORNG_PROFILE_FUNC_GPU();

		const DirectionalLight& light = mp_scene->m_directional_light;

		// Calculate light space matrices
		const float aspect_ratio = static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetHeight());
		const glm::mat4 cam_view_matrix = p_cam->GetViewMatrix();
		const float fov = glm::radians(p_cam->fov / 2.f);

		const glm::mat4 dir_light_space_matrix = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, 0.1f, light.cascade_ranges[0]), cam_view_matrix, light, light.z_mults[0], m_shadow_map_resolution);
		const glm::mat4 dir_light_space_matrix_2 = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, light.cascade_ranges[0] - 2.f, light.cascade_ranges[1]), cam_view_matrix, light, light.z_mults[1], m_shadow_map_resolution);
		const glm::mat4 dir_light_space_matrix_3 = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, light.cascade_ranges[1] - 2.f, light.cascade_ranges[2]), cam_view_matrix, light, light.z_mults[2], m_shadow_map_resolution);
		m_light_space_matrices[0] = dir_light_space_matrix;
		m_light_space_matrices[1] = dir_light_space_matrix_2;
		m_light_space_matrices[2] = dir_light_space_matrix_3;

		// Render cascades
		m_depth_fb->Bind();
		mp_orth_depth_shader->ActivateProgram();
		for (int i = 0; i < 3; i++) {
			glViewport(0, 0, m_shadow_map_resolution, m_shadow_map_resolution);
			m_depth_fb->BindTextureLayerToFBAttachment(m_depth_fb->GetTexture<Texture2DArray>("dir_depth").GetTextureHandle(), GL_DEPTH_ATTACHMENT, i);
			GL_StateManager::ClearDepthBits();

			mp_orth_depth_shader->SetUniform("u_light_pv_matrix", m_light_space_matrices[i]);
			DrawAllMeshes();

		}


		// Spotlights
		glViewport(0, 0, 2048, 2048);
		mp_persp_depth_shader->ActivateProgram();
		auto spotlights = mp_scene->m_registry.view<SpotLightComponent>();

		int index = 0;
		for (auto [entity, light] : spotlights.each()) {

			m_depth_fb->BindTextureLayerToFBAttachment(m_depth_fb->GetTexture<Texture2DArray>("spotlight_depth").GetTextureHandle(), GL_DEPTH_ATTACHMENT, index++);
			GL_StateManager::ClearDepthBits();

			mp_persp_depth_shader->SetUniform("u_light_pv_matrix", light.GetLightSpaceTransform());
			DrawAllMeshes();

		}

		// Pointlights
		index = 0;
		glViewport(0, 0, 512, 512);
		mp_pointlight_depth_shader->ActivateProgram();
		auto pointlights = mp_scene->m_registry.view<PointLightComponent>();

		for (auto [entity, pointlight] : pointlights.each()) {
			glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, pointlight.shadow_distance);
			glm::vec3 light_pos = pointlight.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];

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

				m_depth_fb->BindTextureLayerToFBAttachment(mp_scene->m_pointlight_component_manager.m_pointlight_depth_tex.GetTextureHandle(), GL_DEPTH_ATTACHMENT, index * 6 + i);
				GL_StateManager::ClearDepthBits();

				mp_pointlight_depth_shader->SetUniform("u_light_pv_matrix", captureProjection * captureViews[i]);
				DrawAllMeshes();

			}

			index++;
		}

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());


	}



	void SceneRenderer::DrawSkybox() {
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		Renderer::DrawCube();

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
	}



	void SceneRenderer::IDoFogPass() {
		ORNG_PROFILE_FUNC_GPU();
		//draw fog texture
		m_fog_shader->ActivateProgram();

		m_fog_shader->SetUniform("u_scattering_coef", mp_scene->post_processing.global_fog.scattering_coef);
		m_fog_shader->SetUniform("u_absorption_coef", mp_scene->post_processing.global_fog.absorption_coef);
		m_fog_shader->SetUniform("u_density_coef", mp_scene->post_processing.global_fog.density_coef);
		m_fog_shader->SetUniform("u_scattering_anistropy", mp_scene->post_processing.global_fog.scattering_anistropy);
		m_fog_shader->SetUniform("u_fog_color", mp_scene->post_processing.global_fog.color);
		m_fog_shader->SetUniform("u_step_count", mp_scene->post_processing.global_fog.step_count);
		m_fog_shader->SetUniform("u_time", static_cast<float>(glfwGetTime()));
		m_fog_shader->SetUniform("u_dir_light_matrices[0]", m_light_space_matrices[0]);
		m_fog_shader->SetUniform("u_dir_light_matrices[1]", m_light_space_matrices[1]);
		m_fog_shader->SetUniform("u_dir_light_matrices[2]", m_light_space_matrices[2]);
		m_fog_shader->SetUniform("u_emissive", mp_scene->post_processing.global_fog.emissive_factor);

		GL_StateManager::BindTexture(GL_TEXTURE_3D, mp_scene->post_processing.global_fog.fog_noise.GetTextureHandle(), GL_StateManager::TextureUnits::DATA_3D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shared_depth").GetTextureHandle(), GL_StateManager::TextureUnits::DEPTH);

		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_output_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute(Window::GetWidth() / 16, Window::GetHeight() / 16, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);


		//blur fog texture
		m_blur_shader->ActivateProgram();
		GL_StateManager::BindTexture(
			GL_TEXTURE_2D, m_fog_output_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_3
		);

		m_blur_shader->SetUniform("u_first_iter", 1);
		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_fog_blur_tex_1.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)Window::GetWidth() / 8.f), (GLuint)glm::ceil((float)Window::GetHeight() / 8.f), 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		m_blur_shader->SetUniform("u_first_iter", 0);






	}



	void SceneRenderer::IDoLightingPass() {
		ORNG_PROFILE_FUNC_GPU();

		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_depth_fb->GetTexture<Texture2DArray>("dir_depth").GetTextureHandle(), GL_StateManager::TextureUnits::DIR_SHADOW_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_depth_fb->GetTexture<Texture2DArray>("spotlight_depth").GetTextureHandle(), GL_StateManager::TextureUnits::SPOT_SHADOW_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("albedo").GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("normals").GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shader_ids").GetTextureHandle(), GL_StateManager::TextureUnits::SHADER_IDS, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("roughness_metallic_ao").GetTextureHandle(), GL_StateManager::TextureUnits::ROUGHNESS_METALLIC_AO, false);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, mp_scene->skybox.GetIrradianceTexture().GetTextureHandle(), GL_StateManager::TextureUnits::DIFFUSE_PREFILTER, false);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, mp_scene->m_pointlight_component_manager.m_pointlight_depth_tex.GetTextureHandle(), GL_StateManager::TextureUnits::POINTLIGHT_DEPTH, false);

		m_lighting_fb->Bind();
		GL_StateManager::DefaultClearBits();

		m_lighting_shader->ActivateProgram();

		m_lighting_shader->SetUniform("u_dir_light_matrices[0]", m_light_space_matrices[0]);
		m_lighting_shader->SetUniform("u_dir_light_matrices[1]", m_light_space_matrices[1]);
		m_lighting_shader->SetUniform("u_dir_light_matrices[2]", m_light_space_matrices[2]);

		Renderer::DrawQuad();

		//mp_shader_library->GetShader("skybox").ActivateProgram();
		//GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, Renderer::GetScene()->skybox.GetCubeMapTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		//Renderer::DrawSkybox();

		/* DRAW AABBS */
		/*mp_shader_library->GetShader("highlight").ActivateProgram();
		for (auto& mesh : scene->m_mesh_components) {
			if (!mesh) continue;
			mp_shader_library->GetShader("highlight").SetUniform("transform", mesh->GetTransformComponent()->GetMatrix());
			Renderer::DrawBoundingBox(*mesh->GetMeshData());
		}

		mp_shader_library->GetShader("reflection").ActivateProgram();
		mp_shader_library->GetShader("reflection").SetUniform("camera_pos", Renderer::GetActiveCameraComponent()->GetPos());
		Renderer::DrawGroupsWithShader("reflection");
		*/
	}



	void SceneRenderer::IDrawTerrain(CameraComponent* p_cam) {

		std::vector<TerrainQuadtree*> node_array;

		mp_scene->terrain.m_quadtree->QueryChunks(node_array, p_cam->GetEntity()->GetComponent<TransformComponent>()->GetPosition(), mp_scene->terrain.m_width);
		for (auto& node : node_array) {
			const TerrainChunk* chunk = node->GetChunk();
			if (chunk->m_bounding_box.IsOnFrustum(p_cam->view_frustum)) {
				Renderer::DrawVAO_Elements(GL_QUADS, chunk->m_vao);
			}
		}
	}


	void SceneRenderer::DoBloomPass() {
		ORNG_PROFILE_FUNC_GPU();
		mp_bloom_threshold_shader->ActivateProgram();
		mp_bloom_threshold_shader->SetUniform("u_threshold", mp_scene->post_processing.bloom.threshold);
		mp_bloom_threshold_shader->SetUniform("u_knee", mp_scene->post_processing.bloom.knee);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_lighting_fb->GetTexture<Texture2D>("render_texture").GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR, true);

		// Isolate bright spots
		glBindImageTexture(0, m_bloom_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute((GLuint)glm::ceil((float)Window::GetWidth() / 16.f), (GLuint)glm::ceil((float)Window::GetHeight() / 16.f), 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// Downsample passes
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_bloom_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLOOM, true);
		const int max_mip_layer = 6;
		mp_bloom_downsample_shader->ActivateProgram();
		for (int i = 1; i < max_mip_layer + 1; i++) {
			mp_bloom_downsample_shader->SetUniform("u_mip_level", i);
			glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
			glDispatchCompute((GLuint)glm::ceil(((float)Window::GetWidth() / 32.f) / (float)i), (GLuint)glm::ceil(((float)Window::GetHeight() / 32.f) / (float)i), 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}

		// Upsample passes
		mp_bloom_upsample_shader->ActivateProgram();
		for (int i = max_mip_layer - 1; i >= 0; i--) {
			mp_bloom_upsample_shader->SetUniform("u_mip_level", i);
			glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOUR, m_bloom_tex.GetTextureHandle(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
			glDispatchCompute((GLuint)glm::ceil(((float)Window::GetWidth() / 16.f) / (float)(i + 1)), (GLuint)glm::ceil(((float)Window::GetHeight() / 16.f) / (float)(i + 1)), 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}

	}

	void SceneRenderer::IDoPostProcessingPass(CameraComponent* p_cam) {

		DoBloomPass();

		ORNG_PROFILE_FUNC_GPU();
		m_post_process_shader->ActivateProgram();
		m_post_processing_fb->Bind();
		GL_StateManager::DefaultClearBits();

		m_post_process_shader->SetUniform("exposure", p_cam->exposure);
		m_post_process_shader->SetUniform("u_bloom_intensity", mp_scene->post_processing.bloom.intensity);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("world_positions").GetTextureHandle(), GL_StateManager::TextureUnits::WORLD_POSITIONS, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_fog_blur_tex_2.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR_2, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_lighting_fb->GetTexture<Texture2D>("render_texture").GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR, false);


		Renderer::DrawQuad();


	}

	void SceneRenderer::DrawAllMeshes() const {
		for (const auto* group : mp_scene->m_mesh_component_manager.GetInstanceGroups()) {
			const MeshAsset* mesh_data = group->GetMeshData();
			GL_StateManager::BindSSBO(group->m_transform_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);
			Renderer::DrawMeshInstanced(mesh_data, group->m_instances.size());
		}
	}
}