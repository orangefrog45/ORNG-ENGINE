#include "pch/pch.h"

#include "rendering/SceneRenderer.h"
#include "shaders/ShaderLibrary.h"
#include "core/Window.h"
#include "rendering/Renderer.h"
#include "framebuffers/FramebufferLibrary.h"
#include "core/GLStateManager.h"
#include "scene/Scene.h"
#include "components/Camera.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"

namespace ORNG {

	void SceneRenderer::I_Init() {

		mp_shader_library = &Renderer::GetShaderLibrary();
		mp_framebuffer_library = &Renderer::GetFramebufferLibrary();

		m_gbuffer_shader = &mp_shader_library->CreateShader("gbuffer");
		m_gbuffer_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/GBufferVS.shader");
		m_gbuffer_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/GBufferFS.shader");
		m_gbuffer_shader->Init();
		m_gbuffer_shader->AddUniforms(std::vector<std::string> {
			"u_terrain_mode",
				"u_skybox_mode",
				"diffuse_sampler",
				"specular_sampler",
				"specular_sampler_active",
				"normal_map_sampler",
				"u_normal_sampler_active",
				"diffuse_array_sampler",
				"displacement_sampler",
				"normal_array_sampler",
				"u_material_id",
				"u_shader_id",
		});



		m_gbuffer_shader->SetUniform("diffuse_sampler", GL_StateManager::TextureUnitIndexes::COLOR);
		m_gbuffer_shader->SetUniform("specular_sampler", GL_StateManager::TextureUnitIndexes::SPECULAR);
		m_gbuffer_shader->SetUniform("normal_map_sampler", GL_StateManager::TextureUnitIndexes::NORMAL_MAP);
		m_gbuffer_shader->SetUniform("diffuse_array_sampler", GL_StateManager::TextureUnitIndexes::DIFFUSE_ARRAY);
		m_gbuffer_shader->SetUniform("normal_array_sampler", GL_StateManager::TextureUnitIndexes::NORMAL_ARRAY);
		m_gbuffer_shader->SetUniform("displacement_sampler", GL_StateManager::TextureUnitIndexes::DISPLACEMENT);

		m_lighting_shader = &mp_shader_library->CreateShader("lighting");
		m_lighting_shader->m_shader_id = ShaderLibrary::LIGHTING_SHADER_ID;
		m_lighting_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/LightingVS.shader");
		m_lighting_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/LightingFS.shader");
		m_lighting_shader->Init();
		m_lighting_shader->AddUniforms({
			"u_terrain_mode",

			"u_material.ambient_color",
			"u_material.diffuse_color",
			"u_material.specular_color",

			"spot_depth_sampler",
			"dir_depth_sampler",
			"point_depth_sampler",
			"albedo_sampler",
			"world_position_sampler",
			"normal_sampler",
			"shader_material_id_sampler",

			"u_num_point_lights",
			"u_num_spot_lights",

			"u_dir_light_matrices[0]",
			"u_dir_light_matrices[1]",
			"u_dir_light_matrices[2]",
			});



		// Render quad
		m_post_process_shader = &mp_shader_library->CreateShader("post_process");
		m_post_process_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.shader");
		m_post_process_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/PostProcessFS.shader");
		m_post_process_shader->Init();
		m_post_process_shader->AddUniform("exposure");

		m_post_process_shader->AddUniforms({
			"quad_sampler",
			"world_position_sampler",
			"camera_pos",
			"time",
			"u_show_depth_map"
			});

		m_post_process_shader->SetUniform("quad_sampler", GL_StateManager::TextureUnitIndexes::COLOR);
		m_post_process_shader->SetUniform("world_position_sampler", GL_StateManager::TextureUnitIndexes::WORLD_POSITIONS);

		// blue noise for post-processing effects in quad
		Texture2DSpec noise_spec;
		noise_spec.filepath = "./res/textures/BlueNoise64Tiled.png";
		noise_spec.min_filter = GL_NEAREST;
		noise_spec.mag_filter = GL_NEAREST;
		noise_spec.wrap_params = GL_REPEAT;
		noise_spec.storage_type = GL_UNSIGNED_BYTE;
		noise_spec.format = GL_RGBA;
		noise_spec.internal_format = GL_RGBA8;

		m_blue_noise_tex.SetSpec(noise_spec, false);
		m_blue_noise_tex.LoadFromFile();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_blue_noise_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BLUE_NOISE, false);




		/*Shader& reflection_shader = &mp_shader_library->CreateShader("reflection");
		reflection_shader.AddStage(GL_VERTEX_SHADER, "./res/shaders/ReflectionVS.shader");
		reflection_shader.AddStage(GL_FRAGMENT_SHADER, "./res/shaders/ReflectionFS.shader");
		reflection_shader.Init();
		reflection_shader.AddUniform("camera_pos");*/


		/*Shader& skybox_shader = &mp_shader_library->CreateShader("skybox");
		skybox_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/SkyboxVS.shader");
		skybox_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/SkyboxFS.shader");
		skybox_shader.Init();
		skybox_shader.AddUniform("sky_texture");
		skybox_shader.SetUniform("sky_texture", GL_StateManager::TextureUnitIndexes::COLOR);*/

		m_depth_shader = &mp_shader_library->CreateShader("depth");
		m_depth_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/DepthVS.shader");
		m_depth_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/DepthFS.shader");
		m_depth_shader->Init();
		m_depth_shader->AddUniform("u_terrain_mode");
		m_depth_shader->AddUniform("u_light_pv_matrix");

		m_blur_shader = &mp_shader_library->CreateShader("gaussian_blur");
		m_blur_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.shader");
		m_blur_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/BlurFS.shader");
		m_blur_shader->Init();
		m_blur_shader->AddUniform("u_horizontal");


		//LIGHTING FB
		/* LIGHTING FB */
		Texture2DSpec color_render_texture_spec;
		color_render_texture_spec.format = GL_RGB;
		color_render_texture_spec.internal_format = GL_RGB16F;
		color_render_texture_spec.storage_type = GL_UNSIGNED_BYTE;
		color_render_texture_spec.width = Window::GetWidth();
		color_render_texture_spec.height = Window::GetHeight();

		m_lighting_fb = &mp_framebuffer_library->CreateFramebuffer("lighting");
		m_lighting_fb->AddRenderbuffer();
		m_lighting_fb->Add2DTexture("render_texture", GL_COLOR_ATTACHMENT0, color_render_texture_spec);

		/* GBUFFER FB */
		m_gbuffer_fb = &mp_framebuffer_library->CreateFramebuffer("gbuffer");

		Texture2DSpec gbuffer_spec_1;
		gbuffer_spec_1.format = GL_RGBA;
		gbuffer_spec_1.internal_format = GL_RGBA16F;
		gbuffer_spec_1.storage_type = GL_FLOAT;
		gbuffer_spec_1.width = Window::GetWidth();
		gbuffer_spec_1.height = Window::GetHeight();

		Texture2DSpec gbuffer_spec_2;
		gbuffer_spec_2.format = GL_RG_INTEGER;
		gbuffer_spec_2.internal_format = GL_RG32UI;
		gbuffer_spec_2.storage_type = GL_UNSIGNED_INT;
		gbuffer_spec_2.width = Window::GetWidth();
		gbuffer_spec_2.height = Window::GetHeight();


		Texture2DSpec gbuffer_depth_spec;
		gbuffer_depth_spec.format = GL_DEPTH_COMPONENT;
		gbuffer_depth_spec.internal_format = GL_DEPTH_COMPONENT24;
		gbuffer_depth_spec.storage_type = GL_FLOAT;
		gbuffer_depth_spec.width = Window::GetWidth();
		gbuffer_depth_spec.height = Window::GetHeight();

		m_gbuffer_fb->Add2DTexture("world_positions", GL_COLOR_ATTACHMENT0, gbuffer_spec_1);
		m_gbuffer_fb->Add2DTexture("normals", GL_COLOR_ATTACHMENT1, gbuffer_spec_1);
		m_gbuffer_fb->Add2DTexture("albedo", GL_COLOR_ATTACHMENT2, gbuffer_spec_1);
		m_gbuffer_fb->Add2DTexture("tangents", GL_COLOR_ATTACHMENT3, gbuffer_spec_1);
		m_gbuffer_fb->Add2DTexture("material_ids", GL_COLOR_ATTACHMENT4, gbuffer_spec_2);
		m_gbuffer_fb->Add2DTexture("shared_depth", GL_DEPTH_ATTACHMENT, gbuffer_depth_spec);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
		m_gbuffer_fb->EnableDrawBuffers(5, buffers);


		/* DIRECTIONAL LIGHT DEPTH FB */
		m_depth_fb = &mp_framebuffer_library->CreateFramebuffer("dir_depth");

		Texture2DArraySpec depth_spec;
		depth_spec.format = GL_DEPTH_COMPONENT;
		depth_spec.internal_format = GL_DEPTH_COMPONENT24;
		depth_spec.storage_type = GL_FLOAT;
		depth_spec.min_filter = GL_NEAREST;
		depth_spec.mag_filter = GL_NEAREST;
		depth_spec.wrap_params = GL_CLAMP_TO_EDGE;
		depth_spec.layer_count = 3;
		depth_spec.width = m_shadow_map_resolution;
		depth_spec.height = m_shadow_map_resolution;

		Texture2DArraySpec spotlight_depth_spec = depth_spec;
		spotlight_depth_spec.layer_count = 8;
		m_depth_fb->Add2DTextureArray("dir_depth", depth_spec);
		m_depth_fb->Add2DTextureArray("spotlight_depth", spotlight_depth_spec);

		Texture2DSpec ping_pong_spec;
		ping_pong_spec.format = GL_RGBA;
		ping_pong_spec.internal_format = GL_RGBA16F;
		ping_pong_spec.storage_type = GL_FLOAT;
		ping_pong_spec.width = Window::GetWidth();
		ping_pong_spec.height = Window::GetHeight();
		ping_pong_spec.min_filter = GL_NEAREST;
		ping_pong_spec.mag_filter = GL_NEAREST;
		ping_pong_spec.wrap_params = GL_CLAMP_TO_EDGE;

		// Ping pong fb's used for gaussian blur
		m_ping_pong_1_fb = &mp_framebuffer_library->CreateFramebuffer("ping_pong_1");
		m_ping_pong_1_fb->Add2DTexture("tex1", GL_COLOR_ATTACHMENT0, ping_pong_spec);

		m_ping_pong_2_fb = &mp_framebuffer_library->CreateFramebuffer("ping_pong_2");
		m_ping_pong_2_fb->Add2DTexture("tex1", GL_COLOR_ATTACHMENT0, ping_pong_spec);


		// Fog
		m_fog_shader = &mp_shader_library->CreateShader("fog");
		m_fog_shader->AddStage(GL_COMPUTE_SHADER, "res/shaders/FogCS.shader");
		m_fog_shader->Init();
		m_fog_shader->AddUniforms({
			"u_fog_color",
			"u_time",
			"u_scattering_anistropy",
			"u_absorption_coef",
			"u_scattering_coef",
			"u_density_coef",
			"u_step_count",
			"u_dir_light_matrices[0]",
			"u_dir_light_matrices[1]",
			"u_dir_light_matrices[2]",
			});

		Texture2DSpec fog_overlay_spec;
		fog_overlay_spec.format = GL_RGBA;
		fog_overlay_spec.internal_format = GL_RGBA16F;
		fog_overlay_spec.storage_type = GL_FLOAT;
		fog_overlay_spec.width = Window::GetWidth() / 2;
		fog_overlay_spec.height = Window::GetHeight() / 2;
		fog_overlay_spec.min_filter = GL_NEAREST;
		fog_overlay_spec.mag_filter = GL_NEAREST;
		fog_overlay_spec.wrap_params = GL_CLAMP_TO_EDGE;
		m_fog_output_tex.SetSpec(fog_overlay_spec, true);

		FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
		noise->SetFrequency(0.05f);
		noise->SetCellularReturnType(FastNoiseSIMD::Distance);
		noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);
		float* noise_set = noise->GetCellularSet(0, 0, 0, 64, 64, 64);

		Texture3DSpec fog_noise_spec;
		fog_noise_spec.format = GL_RED;
		fog_noise_spec.internal_format = GL_R16F;
		fog_noise_spec.storage_type = GL_FLOAT;
		fog_noise_spec.width = 64;
		fog_noise_spec.height = 64;
		fog_noise_spec.layer_count = 64;
		fog_noise_spec.min_filter = GL_NEAREST;
		fog_noise_spec.mag_filter = GL_NEAREST;
		fog_noise_spec.wrap_params = GL_MIRRORED_REPEAT;
		m_3d_noise_tex.SetSpec(fog_noise_spec);
		glTexImage3D(GL_TEXTURE_3D, 0, fog_noise_spec.internal_format, fog_noise_spec.width, fog_noise_spec.height, fog_noise_spec.layer_count, 0, fog_noise_spec.format, fog_noise_spec.storage_type, noise_set);

		// POST PROCESSING FB
		m_post_processing_fb = &mp_framebuffer_library->CreateFramebuffer("post_processing");
		m_post_processing_fb->AddRenderbuffer();
		m_post_processing_fb->Add2DTexture("shared_render_texture", GL_COLOR_ATTACHMENT0, color_render_texture_spec);
	}




	void SceneRenderer::IPrepRenderPasses() {
		glm::mat4 cam_mat = mp_active_camera->GetViewMatrix();
		glm::mat4 proj_mat = mp_active_camera->GetProjectionMatrix();
		mp_shader_library->SetCommonUBO(mp_active_camera->GetPos(), mp_active_camera->GetTarget());
		mp_shader_library->SetMatrixUBOs(proj_mat, cam_mat);
		mp_shader_library->SetGlobalLighting(mp_scene->m_directional_light, mp_scene->m_global_ambient_lighting);
		mp_shader_library->SetPointLights(mp_scene->m_point_lights);
		mp_shader_library->SetSpotLights(mp_scene->m_spot_lights);

	}



	SceneRenderer::SceneRenderingOutput SceneRenderer::IRenderScene(const SceneRenderingSettings& settings) {

		SceneRenderer::SceneRenderingOutput output;

		IPrepRenderPasses();

		IDoGBufferPass();
		IDoDepthPass();
		IDoLightingPass();
		//IDoFogPass();
		IDoPostProcessingPass(settings.display_depth_map);

		output.final_color_texture_handle = m_post_processing_fb->GetTexture<Texture2D>("shared_render_texture").GetTextureHandle();

		return output;
	}


	void SceneRenderer::IDoGBufferPass() {

		m_gbuffer_fb->Bind();
		GL_StateManager::DefaultClearBits();
		GL_StateManager::ClearBitsUnsignedInt();

		m_gbuffer_shader->ActivateProgram();

		//Draw all meshes in scene (instanced)
		for (const auto* group : mp_scene->m_mesh_instance_groups) {

			GL_StateManager::BindSSBO(group->m_transform_ssbo_handle, 0);
			m_gbuffer_shader->SetUniform<unsigned int>("u_shader_id", group->GetShaderID());

			for (unsigned int i = 0; i < group->m_mesh_asset->m_submeshes.size(); i++) {

				unsigned int material_id = group->m_mesh_asset->m_scene_materials[group->m_mesh_asset->m_submeshes[i].material_index]->material_id;
				const Material* p_material = mp_scene->GetMaterial(material_id);

				if (p_material->diffuse_texture != nullptr) {
					GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->diffuse_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
				}

				if (p_material->specular_texture == nullptr) {
					m_gbuffer_shader->SetUniform("specular_sampler_active", 0);
				}
				else {
					m_gbuffer_shader->SetUniform("specular_sampler_active", 1);
					GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->specular_texture->GetTextureHandle(), GL_StateManager::TextureUnits::SPECULAR, false);
				}

				if (p_material->normal_map_texture == nullptr) {
					m_gbuffer_shader->SetUniform("u_normal_sampler_active", 0);
				}
				else {
					m_gbuffer_shader->SetUniform("u_normal_sampler_active", 1);
					GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->normal_map_texture->GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP, false);
				}

				m_gbuffer_shader->SetUniform<unsigned int>("u_material_id", material_id);
				Renderer::DrawSubMeshInstanced(group->m_mesh_asset, group->GetInstanceCount(), i);
			}
		}


		/* textures */
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, mp_scene->m_terrain.m_diffuse_texture_array.GetTextureHandle(), GL_StateManager::TextureUnits::DIFFUSE_ARRAY, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, mp_scene->m_terrain.m_normal_texture_array.GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_ARRAY, false);

		/* uniforms */
		m_gbuffer_shader->SetUniform("u_terrain_mode", 1);
		m_gbuffer_shader->SetUniform<unsigned int>("u_material_id", mp_scene->m_terrain.m_material_id);
		m_gbuffer_shader->SetUniform<unsigned int>("u_shader_id", m_lighting_shader->m_shader_id);
		IDrawTerrain();
		m_gbuffer_shader->SetUniform("u_terrain_mode", 0);

		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, mp_scene->m_skybox.GetCubeMapTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_CUBEMAP, false);
		m_gbuffer_shader->SetUniform("u_skybox_mode", 1);
		m_gbuffer_shader->SetUniform<unsigned int>("u_material_id", ShaderLibrary::INVALID_MATERIAL_ID);
		m_gbuffer_shader->SetUniform<unsigned int>("u_shader_id", ShaderLibrary::INVALID_SHADER_ID);
		DrawSkybox();
		m_gbuffer_shader->SetUniform("u_skybox_mode", 0);


		/*if (sample_world_pos) {
			glm::vec2 mouse_coords = glm::min(glm::max(Input::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

			GLfloat* pixels = new GLfloat[4];
			glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RGB, GL_FLOAT, pixels);
			m_sampled_world_pos = glm::vec3(pixels[0], pixels[1], pixels[2]);
			delete[] pixels;
		}*/
	}





	void SceneRenderer::IDoDepthPass() {
		const float aspect_ratio = static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetHeight());

		const DirectionalLight& light = mp_scene->GetDirectionalLight();

		const glm::mat4 cam_view_matrix = mp_active_camera->GetViewMatrix();
		const float fov = glm::radians(mp_active_camera->GetFovDegrees() / 2.f);

		const glm::mat4 dir_light_space_matrix = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, 0.1f, light.cascade_ranges[0]), cam_view_matrix, light, light.z_mults[0], m_shadow_map_resolution);
		const glm::mat4 dir_light_space_matrix_2 = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, light.cascade_ranges[0], light.cascade_ranges[1]), cam_view_matrix, light, light.z_mults[1], m_shadow_map_resolution);
		const glm::mat4 dir_light_space_matrix_3 = ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, light.cascade_ranges[1], light.cascade_ranges[2]), cam_view_matrix, light, light.z_mults[2], m_shadow_map_resolution);

		m_light_space_matrices[0] = dir_light_space_matrix;
		m_light_space_matrices[1] = dir_light_space_matrix_2;
		m_light_space_matrices[2] = dir_light_space_matrix_3;

		m_depth_fb->Bind();
		m_depth_shader->ActivateProgram();

		for (int i = 0; i < 3; i++) {
			glViewport(0, 0, m_shadow_map_resolution, m_shadow_map_resolution);
			m_depth_fb->BindTextureLayerToFBAttachment(m_depth_fb->GetTexture<Texture2DArray>("dir_depth").GetTextureHandle(), GL_DEPTH_ATTACHMENT, i);
			GL_StateManager::ClearDepthBits();

			m_depth_shader->SetUniform("u_light_pv_matrix", m_light_space_matrices[i]);

			for (const auto* group : mp_scene->m_mesh_instance_groups) {
				const MeshAsset* mesh_data = group->GetMeshData();
				GL_StateManager::BindSSBO(group->m_transform_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);

				for (int i = 0; i < mesh_data->m_submeshes.size(); i++) {
					Renderer::DrawSubMeshInstanced(mesh_data, group->m_instances.size(), i);
				}

			}

			m_depth_shader->SetUniform("u_terrain_mode", 1);
			IDrawTerrain();
			m_depth_shader->SetUniform("u_terrain_mode", 0);

		}


		int depth_map_index = -1;
		for (int i = 0; i < mp_scene->m_spot_lights.size(); i++) {
			SpotLightComponent* p_light = mp_scene->m_spot_lights[i];

			if (!p_light)
				continue;

			depth_map_index++;
			m_depth_fb->BindTextureLayerToFBAttachment(m_depth_fb->GetTexture<Texture2DArray>("spotlight_depth").GetTextureHandle(), GL_DEPTH_ATTACHMENT, depth_map_index);
			GL_StateManager::ClearDepthBits();

			m_depth_shader->SetUniform("u_light_pv_matrix", p_light->GetLightSpaceTransformMatrix());

			for (const auto* group : mp_scene->m_mesh_instance_groups) {
				const MeshAsset* mesh_data = group->GetMeshData();
				GL_StateManager::BindSSBO(group->m_transform_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);

				for (int i = 0; i < mesh_data->m_submeshes.size(); i++) {
					Renderer::DrawSubMeshInstanced(mesh_data, group->m_instances.size(), i);
				}

			}

			m_depth_shader->SetUniform("u_terrain_mode", 1);
			IDrawTerrain();
			m_depth_shader->SetUniform("u_terrain_mode", 0);
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
		//draw fog texture
		m_fog_shader->ActivateProgram();

		m_fog_shader->SetUniform("u_scattering_coef", mp_scene->m_global_fog.scattering_coef);
		m_fog_shader->SetUniform("u_absorption_coef", mp_scene->m_global_fog.absorption_coef);
		m_fog_shader->SetUniform("u_density_coef", mp_scene->m_global_fog.density_coef);
		m_fog_shader->SetUniform("u_scattering_anistropy", mp_scene->m_global_fog.scattering_anistropy);
		m_fog_shader->SetUniform("u_fog_color", mp_scene->m_global_fog.color);
		m_fog_shader->SetUniform("u_step_count", mp_scene->m_global_fog.step_count);
		m_fog_shader->SetUniform("u_time", static_cast<float>(glfwGetTime()));
		m_fog_shader->SetUniform("u_dir_light_matrices[0]", m_light_space_matrices[0]);
		m_fog_shader->SetUniform("u_dir_light_matrices[1]", m_light_space_matrices[1]);
		m_fog_shader->SetUniform("u_dir_light_matrices[2]", m_light_space_matrices[2]);

		GL_StateManager::BindTexture(GL_TEXTURE_3D, m_3d_noise_tex.GetTextureHandle(), GL_StateManager::TextureUnits::DATA_3D);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("shared_depth").GetTextureHandle(), GL_StateManager::TextureUnits::DEPTH);

		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOR, m_fog_output_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute(Window::GetWidth() / 16, Window::GetHeight() / 8, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);


		//blur fog texture
		m_blur_shader->ActivateProgram();
		Framebuffer* active_fb = m_ping_pong_1_fb;

		m_ping_pong_2_fb->Bind();
		GL_StateManager::DefaultClearBits();
		GL_StateManager::BindTexture(
			GL_TEXTURE_2D, m_fog_output_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2
		);
		m_blur_shader->SetUniform("u_horizontal", 1);
		Renderer::DrawQuad();

		for (int i = 0; i < 2; i++) {
			active_fb->Bind();
			GL_StateManager::DefaultClearBits();
			m_blur_shader->SetUniform("u_horizontal", active_fb == m_ping_pong_1_fb ? 0 : 1);

			if (active_fb == m_ping_pong_1_fb) {
				GL_StateManager::BindTexture(
					GL_TEXTURE_2D, m_ping_pong_2_fb->GetTexture<Texture2D>("tex1").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2
				);
			}
			else {
				GL_StateManager::BindTexture(
					GL_TEXTURE_2D, m_ping_pong_1_fb->GetTexture<Texture2D>("tex1").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2
				);

			}
			Renderer::DrawQuad();
			active_fb = active_fb == m_ping_pong_1_fb ? m_ping_pong_2_fb : m_ping_pong_1_fb;
		}

	}



	void SceneRenderer::IDoLightingPass() {

		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_depth_fb->GetTexture<Texture2DArray>("dir_depth").GetTextureHandle(), GL_StateManager::TextureUnits::DIR_SHADOW_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, m_depth_fb->GetTexture<Texture2DArray>("spotlight_depth").GetTextureHandle(), GL_StateManager::TextureUnits::SPOT_SHADOW_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("albedo").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("normals").GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("material_ids").GetTextureHandle(), GL_StateManager::TextureUnits::SHADER_MATERIAL_IDS, false);

		m_lighting_fb->Bind();
		GL_StateManager::DefaultClearBits();

		m_lighting_shader->ActivateProgram();

		BaseLight& ambient_light = mp_scene->m_global_ambient_lighting;
		DirectionalLight& light = mp_scene->m_directional_light;
		m_lighting_shader->SetUniform("u_dir_light_matrices[0]", m_light_space_matrices[0]);
		m_lighting_shader->SetUniform("u_dir_light_matrices[1]", m_light_space_matrices[1]);
		m_lighting_shader->SetUniform("u_dir_light_matrices[2]", m_light_space_matrices[2]);

		glDisable(GL_DEPTH_TEST);
		Renderer::DrawQuad();
		glEnable(GL_DEPTH_TEST);

		//mp_shader_library->GetShader("skybox").ActivateProgram();
		//GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, Renderer::GetScene()->m_skybox.GetCubeMapTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		//Renderer::DrawSkybox();

		/* DRAW AABBS */
		/*mp_shader_library->GetShader("highlight").ActivateProgram();
		for (auto& mesh : scene->m_mesh_components) {
			if (!mesh) continue;
			mp_shader_library->GetShader("highlight").SetUniform("transform", mesh->GetWorldTransform()->GetMatrix());
			Renderer::DrawBoundingBox(*mesh->GetMeshData());
		}

		mp_shader_library->GetShader("reflection").ActivateProgram();
		mp_shader_library->GetShader("reflection").SetUniform("camera_pos", Renderer::GetActiveCamera()->GetPos());
		Renderer::DrawGroupsWithShader("reflection");
		*/
	}



	void SceneRenderer::IDrawTerrain() {

		std::vector<TerrainQuadtree*> node_array;
		mp_scene->m_terrain.m_quadtree->QueryChunks(node_array, mp_active_camera->GetPos(), mp_scene->m_terrain.m_width);
		for (auto& node : node_array) {
			const TerrainChunk* chunk = node->GetChunk();
			if (chunk->m_bounding_box.IsOnFrustum(mp_active_camera->m_view_frustum)) {
				Renderer::DrawVAO_Elements(GL_QUADS, chunk->m_vao);
			}
		}
	}


	void SceneRenderer::IDoPostProcessingPass(bool depth_display_active) {
		glDisable(GL_DEPTH_TEST);
		m_post_process_shader->ActivateProgram();

		m_post_processing_fb->Bind();
		GL_StateManager::DefaultClearBits();

		m_post_process_shader->SetUniform("exposure", mp_scene->m_exposure_level);
		if (depth_display_active) {
			m_post_process_shader->SetUniform("u_show_depth_map", 1);
		}
		else {
			m_post_process_shader->SetUniform("u_show_depth_map", 0);
		}


		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_gbuffer_fb->GetTexture<Texture2D>("world_positions").GetTextureHandle(), GL_StateManager::TextureUnits::WORLD_POSITIONS, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_lighting_fb->GetTexture<Texture2D>("render_texture").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_ping_pong_2_fb->GetTexture<Texture2D>("tex1").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2, false);


		glDisable(GL_DEPTH_TEST);
		Renderer::DrawQuad();
		glEnable(GL_DEPTH_TEST);

	}
}