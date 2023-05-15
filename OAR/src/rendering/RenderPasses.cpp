#include "pch/pch.h"

#include "rendering/Renderpasses.h"
#include "rendering/Renderer.h"
#include "components/Camera.h"
#include "rendering/Quad.h"
#include "core/Input.h"
#include "core/GLStateManager.h"
#include "fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "core/Window.h"

namespace ORNG {

	void RenderPasses::InitPasses() { // this entire class is a crime but i can't find another place to do renderpasses right now, and dont need a full on lib for them yet

		ShaderLibrary& shader_library = Renderer::GetShaderLibrary();
		FramebufferLibrary& fb_library = Renderer::GetFramebufferLibrary();

		Shader& grid_shader = shader_library.CreateShader("grid");
		grid_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/GridVS.shader");
		grid_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/GridFS.shader");
		grid_shader.Init();
		grid_shader.AddUniform("camera_pos");


		Shader& gbuffer_shader = shader_library.CreateShader("gbuffer");
		gbuffer_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/GBufferVS.shader");
		gbuffer_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/GBufferFS.shader");
		gbuffer_shader.Init();
		gbuffer_shader.AddUniforms(std::vector<std::string> {
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

		gbuffer_shader.SetUniform("diffuse_sampler", GL_StateManager::TextureUnitIndexes::COLOR);
		gbuffer_shader.SetUniform("specular_sampler", GL_StateManager::TextureUnitIndexes::SPECULAR);
		gbuffer_shader.SetUniform("normal_map_sampler", GL_StateManager::TextureUnitIndexes::NORMAL_MAP);
		gbuffer_shader.SetUniform("diffuse_array_sampler", GL_StateManager::TextureUnitIndexes::DIFFUSE_ARRAY);
		gbuffer_shader.SetUniform("normal_array_sampler", GL_StateManager::TextureUnitIndexes::NORMAL_ARRAY);
		gbuffer_shader.SetUniform("displacement_sampler", GL_StateManager::TextureUnitIndexes::DISPLACEMENT);


		Shader& highlight_shader = shader_library.CreateShader("highlight");
		highlight_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/HighlightVS.shader");
		highlight_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/HighlightFS.shader");
		highlight_shader.Init();
		highlight_shader.AddUniform("transform");

		/* 2D quad will just be a place on the screen to render textures onto, currently postprocessing */
		Shader& quad_shader = shader_library.CreateShader("2d_quad");
		quad_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.shader");
		quad_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/QuadFS.shader");
		quad_shader.Init();

		quad_shader.AddUniforms({
			"quad_sampler",
			"world_position_sampler",
			"camera_pos",
			"time",
			"u_show_depth_map"
			});

		blue_noise = new Texture2D(); // blue noise for post-processing effects in quad
		Texture2DSpec spec;
		spec.filepath = "./res/textures/BlueNoise64Tiled.png";
		spec.min_filter = GL_NEAREST;
		spec.mag_filter = GL_NEAREST;
		spec.wrap_params = GL_REPEAT;
		spec.storage_type = GL_UNSIGNED_BYTE;
		spec.format = GL_RGBA;
		spec.internal_format = GL_RGBA8;

		blue_noise->SetSpec(spec, false);
		blue_noise->LoadFromFile();

		quad_shader.SetUniform("quad_sampler", GL_StateManager::TextureUnitIndexes::COLOR);
		quad_shader.SetUniform("world_position_sampler", GL_StateManager::TextureUnitIndexes::WORLD_POSITIONS);

		GL_StateManager::BindTexture(GL_TEXTURE_2D, blue_noise->GetTextureHandle(), GL_StateManager::TextureUnits::BLUE_NOISE, false);


		Shader& reflection_shader = shader_library.CreateShader("reflection");
		reflection_shader.AddStage(GL_VERTEX_SHADER, "./res/shaders/ReflectionVS.shader");
		reflection_shader.AddStage(GL_FRAGMENT_SHADER, "./res/shaders/ReflectionFS.shader");
		reflection_shader.Init();
		reflection_shader.AddUniform("camera_pos");


		Shader& skybox_shader = shader_library.CreateShader("skybox");
		skybox_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/SkyboxVS.shader");
		skybox_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/SkyboxFS.shader");
		skybox_shader.Init();
		skybox_shader.AddUniform("sky_texture");
		skybox_shader.SetUniform("sky_texture", GL_StateManager::TextureUnitIndexes::COLOR);

		Shader& picking_shader = shader_library.CreateShader("picking");
		picking_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/PickingVS.shader");
		picking_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/PickingFS.shader");
		picking_shader.Init();
		picking_shader.AddUniform("comp_id");
		picking_shader.AddUniform("world_transform");

		Shader& depth_shader = shader_library.CreateShader("depth");
		depth_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/DepthVS.shader");
		depth_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/DepthFS.shader");
		depth_shader.Init();
		depth_shader.AddUniform("u_terrain_mode");
		depth_shader.AddUniform("u_light_pv_matrix");

		Shader& blur_shader = shader_library.CreateShader("gaussian_blur");
		blur_shader.AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.shader");
		blur_shader.AddStage(GL_FRAGMENT_SHADER, "res/shaders/BlurFS.shader");
		blur_shader.Init();
		blur_shader.AddUniform("u_horizontal");

		/* SPOTLIGHT DEPTH FB
		constexpr float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		Framebuffer& spot_depth = fb_library.CreateFramebuffer("spotlight_depth");
		spot_depth.Init();
		spot_depth.Add2DTextureArray("depth", 1024, 1024, Renderer::max_spot_lights, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_color);
		fb_library.UnbindAllFramebuffers();*/


		/* GBUFFER FB */
		Framebuffer& gbuffer = fb_library.CreateFramebuffer("gbuffer");
		gbuffer.AddRenderbuffer();

		Texture2DSpec gbuffer_spec_1;
		gbuffer_spec_1.format = GL_RGBA;
		gbuffer_spec_1.internal_format = GL_RGBA32F;
		gbuffer_spec_1.storage_type = GL_FLOAT;
		gbuffer_spec_1.width = Window::GetWidth();
		gbuffer_spec_1.height = Window::GetHeight();

		Texture2DSpec gbuffer_spec_2;
		gbuffer_spec_2.format = GL_RG_INTEGER;
		gbuffer_spec_2.internal_format = GL_R32UI;
		gbuffer_spec_2.storage_type = GL_UNSIGNED_INT;
		gbuffer_spec_2.width = Window::GetWidth();
		gbuffer_spec_2.height = Window::GetHeight();

		gbuffer.Add2DTexture("world_positions", GL_COLOR_ATTACHMENT0, gbuffer_spec_1);
		gbuffer.Add2DTexture("normals", GL_COLOR_ATTACHMENT1, gbuffer_spec_1);
		gbuffer.Add2DTexture("albedo", GL_COLOR_ATTACHMENT2, gbuffer_spec_1);
		gbuffer.Add2DTexture("tangents", GL_COLOR_ATTACHMENT3, gbuffer_spec_1);
		gbuffer.Add2DTexture("material_ids", GL_COLOR_ATTACHMENT4, gbuffer_spec_2);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
		gbuffer.EnableDrawBuffers(5, buffers);


		/*PICKING FB*/
		Framebuffer& picking = fb_library.CreateFramebuffer("picking");
		picking.AddRenderbuffer();

		Texture2DSpec picking_spec;
		picking_spec.format = GL_RED_INTEGER;
		picking_spec.internal_format = GL_R32UI;
		picking_spec.storage_type = GL_UNSIGNED_INT;
		picking_spec.width = Window::GetWidth();
		picking_spec.height = Window::GetHeight();

		picking.Add2DTexture("component_ids", GL_COLOR_ATTACHMENT0, picking_spec);

		/* DIRECTIONAL LIGHT DEPTH FB */
		Framebuffer& dir_depth = fb_library.CreateFramebuffer("dir_depth");

		Texture2DArraySpec depth_spec;
		depth_spec.format = GL_DEPTH_COMPONENT;
		depth_spec.internal_format = GL_DEPTH_COMPONENT24;
		depth_spec.storage_type = GL_FLOAT;
		depth_spec.layer_count = 3;
		depth_spec.width = shadow_map_resolution;
		depth_spec.height = shadow_map_resolution;

		dir_depth.Add2DTextureArray("depth", depth_spec);



		Texture2DSpec ping_pong_spec;
		ping_pong_spec.format = GL_RGBA;
		ping_pong_spec.internal_format = GL_RGBA32F;
		ping_pong_spec.storage_type = GL_FLOAT;
		ping_pong_spec.width = Window::GetWidth();
		ping_pong_spec.height = Window::GetHeight();
		ping_pong_spec.min_filter = GL_LINEAR;
		ping_pong_spec.mag_filter = GL_LINEAR;

		// Ping pong fb's used for gaussian blur
		Framebuffer& ping_pong_1_fb = fb_library.CreateFramebuffer("ping_pong_1");
		ping_pong_1_fb.Add2DTexture("tex1", GL_COLOR_ATTACHMENT0, ping_pong_spec);

		Framebuffer& ping_pong_2_fb = fb_library.CreateFramebuffer("ping_pong_2");
		ping_pong_2_fb.Add2DTexture("tex1", GL_COLOR_ATTACHMENT0, ping_pong_spec);

		Shader& fog_shader = shader_library.CreateShader("fog");
		fog_shader.AddStage(GL_COMPUTE_SHADER, "res/shaders/FogCS.shader");
		fog_shader.Init();
		fog_shader.AddUniform("u_camera_pos");
		fog_shader.AddUniform("u_fog_hardness");
		fog_shader.AddUniform("u_fog_intensity");
		fog_shader.AddUniform("u_fog_color");
		fog_shader.AddUniform("u_time");
		fog_shader.AddUniform("u_dir_light_matrices[0]");
		fog_shader.AddUniform("u_dir_light_matrices[1]");
		fog_shader.AddUniform("u_dir_light_matrices[2]");
	}


	void RenderPasses::DoPickingPass() {
		Shader& picking_shader = Renderer::GetShaderLibrary().GetShader("picking");
		Framebuffer& picking_fb = Renderer::GetFramebufferLibrary().GetFramebuffer("picking");
		picking_fb.Bind();
		picking_shader.ActivateProgram();

		GL_StateManager::ClearDepthBits();
		GL_StateManager::ClearBitsUnsignedInt();

		for (auto& mesh : Renderer::GetScene()->GetMeshComponents()) {
			if (!mesh || !mesh->GetMeshData()) continue;

			if (mesh->GetMeshData()->GetLoadStatus() == true) {
				picking_shader.SetUniform<unsigned int>("comp_id", mesh->GetEntityHandle());
				picking_shader.SetUniform("world_transform", mesh->GetWorldTransform()->GetMatrix());
				Renderer::DrawMesh(mesh->GetMeshData(), false);
			}
		}

		glm::vec2 mouse_coords = glm::min(glm::max(Input::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

		GLuint* pixels = new GLuint[1];
		glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, pixels);
		current_entity_id = pixels[0];
		delete[] pixels;
	}

	void RenderPasses::DoDepthPass() {
		const Camera* p_cam = Renderer::GetActiveCamera();
		const float aspect_ratio = static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetHeight());
		const float cascade_1_znear = 0.1f;
		const float cascade_1_zfar = 200.f;
		const float cascade_2_zfar = 500.f;
		const float cascade_3_zfar = 1200.f;

		const DirectionalLight& light = Renderer::GetScene()->GetDirectionalLight();

		const glm::mat4 cam_view_matrix = p_cam->GetViewMatrix();
		const float fov = glm::radians(p_cam->GetFovDegrees() / 2.f);

		const glm::mat4 dir_light_space_matrix = glm::rowMajor4(ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, cascade_1_znear, cascade_1_zfar), cam_view_matrix, light, 5, shadow_map_resolution));
		const glm::mat4 dir_light_space_matrix_2 = glm::rowMajor4(ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, cascade_1_zfar, cascade_2_zfar), cam_view_matrix, light, 3, shadow_map_resolution));
		const glm::mat4 dir_light_space_matrix_3 = glm::rowMajor4(ExtraMath::CalculateLightSpaceMatrix(glm::perspective(fov, aspect_ratio, cascade_2_zfar, cascade_3_zfar), cam_view_matrix, light, 3, shadow_map_resolution));

		dir_light_space_mat_outputs[0] = dir_light_space_matrix;
		dir_light_space_mat_outputs[1] = dir_light_space_matrix_2;
		dir_light_space_mat_outputs[2] = dir_light_space_matrix_3;

		Framebuffer& dir_depth_fb = Renderer::GetFramebufferLibrary().GetFramebuffer("dir_depth");
		Shader& depth_shader = Renderer::GetShaderLibrary().GetShader("depth");
		dir_depth_fb.Bind();
		depth_shader.ActivateProgram();

		glViewport(0, 0, shadow_map_resolution, shadow_map_resolution);
		for (int i = 0; i < 3; i++) {
			dir_depth_fb.BindTextureLayerToFBAttachment(dir_depth_fb.GetTexture<Texture2DArray>("depth").GetTextureHandle(), GL_DEPTH_ATTACHMENT, i);
			GL_StateManager::ClearDepthBits();

			depth_shader.SetUniform("u_light_pv_matrix", dir_light_space_mat_outputs[i]);
			Renderer::DrawAllMeshesInstanced(false);

			depth_shader.SetUniform("u_terrain_mode", 1);
			Renderer::DrawTerrain();
			depth_shader.SetUniform("u_terrain_mode", 0);

		}

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	}

	void RenderPasses::DoFogPass() {
		ShaderLibrary& shader_library = Renderer::GetShaderLibrary();
		FramebufferLibrary& framebuffer_library = Renderer::GetFramebufferLibrary();
		Shader& fog_shader = shader_library.GetShader("fog");
		fog_shader.ActivateProgram();

		//draw fog texture

		static Texture2D fog_texture;
		static Texture3D fog_noise_texture;

		static bool texture_loaded = false;
		if (!texture_loaded) {
			Texture2DSpec fog_spec;
			fog_spec.format = GL_RGBA;
			fog_spec.internal_format = GL_RGBA32F;
			fog_spec.storage_type = GL_FLOAT;
			fog_spec.width = Window::GetWidth() / 2;
			fog_spec.height = Window::GetHeight() / 2;
			fog_spec.min_filter = GL_NEAREST;
			fog_spec.mag_filter = GL_NEAREST;
			fog_spec.wrap_params = GL_REPEAT;
			fog_texture.SetSpec(fog_spec, true);

			FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
			noise->SetFrequency(0.05f);
			noise->SetCellularReturnType(FastNoiseSIMD::Distance);
			noise->SetNoiseType(FastNoiseSIMD::Cellular);
			float* noise_set = noise->GetPerlinFractalSet(0, 0, 0, 128, 128, 128);

			Texture3DSpec fog_noise_spec;
			fog_noise_spec.format = GL_RED;
			fog_noise_spec.internal_format = GL_R32F;
			fog_noise_spec.storage_type = GL_FLOAT;
			fog_noise_spec.width = 128;
			fog_noise_spec.height = 128;
			fog_noise_spec.layer_count = 128;
			fog_noise_spec.min_filter = GL_LINEAR;
			fog_noise_spec.mag_filter = GL_LINEAR;
			fog_noise_spec.wrap_params = GL_MIRRORED_REPEAT;
			fog_noise_texture.SetSpec(fog_noise_spec);
			glTexImage3D(GL_TEXTURE_3D, 0, fog_noise_spec.internal_format, fog_noise_spec.width, fog_noise_spec.height, fog_noise_spec.layer_count, 0, fog_noise_spec.format, fog_noise_spec.storage_type, noise_set);
			texture_loaded = true;
		}
		fog_shader.SetUniform("u_camera_pos", Renderer::GetActiveCamera()->GetPos());
		fog_shader.SetUniform("u_fog_intensity", fog_data.intensity);
		fog_shader.SetUniform("u_fog_hardness", fog_data.hardness);
		fog_shader.SetUniform("u_fog_color", fog_data.color);
		fog_shader.SetUniform("u_time", static_cast<float>(glfwGetTime()));
		fog_shader.SetUniform("u_dir_light_matrices[0]", dir_light_space_mat_outputs[0]);
		fog_shader.SetUniform("u_dir_light_matrices[1]", dir_light_space_mat_outputs[1]);
		fog_shader.SetUniform("u_dir_light_matrices[2]", dir_light_space_mat_outputs[2]);

		GL_StateManager::BindTexture(GL_TEXTURE_3D, fog_noise_texture.GetTextureHandle(), GL_StateManager::TextureUnits::DATA_3D);

		glBindImageTexture(GL_StateManager::TextureUnitIndexes::COLOR, fog_texture.GetTextureHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glDispatchCompute(ceil(Window::GetWidth() / 16), ceil(Window::GetHeight() / 8), 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);



		//blur fog texture
		Shader& blur_shader = shader_library.GetShader("gaussian_blur");
		blur_shader.ActivateProgram();


		Framebuffer* ping_pong_1 = &framebuffer_library.GetFramebuffer("ping_pong_1");
		Framebuffer* ping_pong_2 = &framebuffer_library.GetFramebuffer("ping_pong_2");

		Framebuffer* active_fb = ping_pong_2;

		ping_pong_1->Bind();
		GL_StateManager::DefaultClearBits();
		GL_StateManager::BindTexture(
			GL_TEXTURE_2D, fog_texture.GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2
		);
		Renderer::DrawQuad();

		for (int i = 0; i < 7; i++) {
			active_fb->Bind();
			GL_StateManager::DefaultClearBits();
			blur_shader.SetUniform("u_horizontal", active_fb == ping_pong_1 ? 0 : 1);

			if (active_fb == ping_pong_1) {
				GL_StateManager::BindTexture(
					GL_TEXTURE_2D, ping_pong_2->GetTexture<Texture2D>("tex1").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2
				);
			}
			else {
				GL_StateManager::BindTexture(
					GL_TEXTURE_2D, ping_pong_1->GetTexture<Texture2D>("tex1").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2
				);
			}
			Renderer::DrawQuad();
			active_fb = active_fb == ping_pong_1 ? ping_pong_2 : ping_pong_1;
		}


	}

	void RenderPasses::DoLightingPass() {

		ShaderLibrary& shader_library = Renderer::GetShaderLibrary();
		FramebufferLibrary& framebuffer_library = Renderer::GetFramebufferLibrary();
		LightingShader& lighting_shader = shader_library.lighting_shader;

		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, framebuffer_library.GetFramebuffer("dir_depth").GetTexture<Texture2DArray>("depth").GetTextureHandle(), GL_StateManager::TextureUnits::DIR_SHADOW_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("gbuffer").GetTexture<Texture2D>("albedo").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("gbuffer").GetTexture<Texture2D>("normals").GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("gbuffer").GetTexture<Texture2D>("material_ids").GetTextureHandle(), GL_StateManager::TextureUnits::SHADER_MATERIAL_IDS, false);

		framebuffer_library.GetFramebuffer("lighting").Bind();
		GL_StateManager::DefaultClearBits();

		lighting_shader.ActivateProgram();

		Scene* scene = Renderer::GetScene();


		lighting_shader.SetPointLights(scene->m_point_lights);

		//lighting_shader.SetSpotLights(scene->m_spot_lights);
		lighting_shader.SetAmbientLight(scene->m_global_ambient_lighting);
		lighting_shader.SetDirectionLight(scene->m_directional_light);
		lighting_shader.SetUniform("u_dir_light_matrices[0]", dir_light_space_mat_outputs[0]);
		lighting_shader.SetUniform("u_dir_light_matrices[1]", dir_light_space_mat_outputs[1]);
		lighting_shader.SetUniform("u_dir_light_matrices[2]", dir_light_space_mat_outputs[2]);
		lighting_shader.SetUniform("u_view_pos", Renderer::GetActiveCamera()->GetPos());

		glDisable(GL_DEPTH_TEST);
		Renderer::DrawQuad();
		glEnable(GL_DEPTH_TEST);

		//shader_library.GetShader("skybox").ActivateProgram();
		//GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, Renderer::GetScene()->m_skybox.GetCubeMapTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		//Renderer::DrawSkybox();

		/* DRAW AABBS */
		/*shader_library.GetShader("highlight").ActivateProgram();
		for (auto& mesh : scene->m_mesh_components) {
			if (!mesh) continue;
			shader_library.GetShader("highlight").SetUniform("transform", mesh->GetWorldTransform()->GetMatrix());
			Renderer::DrawBoundingBox(*mesh->GetMeshData());
		}

		shader_library.GetShader("reflection").ActivateProgram();
		shader_library.GetShader("reflection").SetUniform("camera_pos", Renderer::GetActiveCamera()->GetPos());
		Renderer::DrawGroupsWithShader("reflection");
		*/
	}

	void RenderPasses::DoDrawToQuadPass() {
		glDisable(GL_DEPTH_TEST);
		Shader& quad_shader = Renderer::GetShaderLibrary().GetShader("2d_quad");
		FramebufferLibrary& framebuffer_library = Renderer::GetFramebufferLibrary();
		quad_shader.ActivateProgram();

		framebuffer_library.UnbindAllFramebuffers();
		GL_StateManager::DefaultClearBits();

		//quad_shader.SetUniform<float>("time", );
		quad_shader.SetUniform("camera_pos", Renderer::GetActiveCamera()->GetPos());
		if (depth_map_view_active) {
			quad_shader.SetUniform("u_show_depth_map", 1);
		}
		else {
			quad_shader.SetUniform("u_show_depth_map", 0);
		}


		GL_StateManager::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("gbuffer").GetTexture<Texture2D>("world_positions").GetTextureHandle(), GL_StateManager::TextureUnits::WORLD_POSITIONS, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("lighting").GetTexture<Texture2D>("render_texture").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("ping_pong_2").GetTexture<Texture2D>("tex1").GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_2, false);


		glDisable(GL_DEPTH_TEST);
		Renderer::DrawQuad();
		glEnable(GL_DEPTH_TEST);
	}


	void RenderPasses::DoGBufferPass() {
		const Framebuffer& gbuffer = Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer");
		const Scene* scene = Renderer::GetScene();
		ShaderLibrary& shader_library = Renderer::GetShaderLibrary();
		Shader& gbuffer_shader = shader_library.GetShader("gbuffer");

		gbuffer.Bind();

		GL_StateManager::DefaultClearBits();
		GL_StateManager::ClearBitsUnsignedInt();

		gbuffer_shader.ActivateProgram();

		//Draw all meshes in scene (instanced)
		for (const auto* group : scene->m_mesh_instance_groups) {
			unsigned int material_id = group->GetMaterialID();
			gbuffer_shader.SetUniform<unsigned int>("u_material_id", material_id);
			gbuffer_shader.SetUniform<unsigned int>("u_shader_id", group->GetShaderID());
			shader_library.SetGBufferMaterial(*scene->GetMaterial(material_id));
			GL_StateManager::BindSSBO(group->m_transform_ssbo_handle, 0);

			Renderer::DrawMeshInstanced(group->GetMeshData(), group->GetInstanceCount(), true);

		}


		/* textures */
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, scene->m_terrain.m_diffuse_texture_array.GetTextureHandle(), GL_StateManager::TextureUnits::DIFFUSE_ARRAY, false);
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, scene->m_terrain.m_normal_texture_array.GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_ARRAY, false);

		/* uniforms */
		gbuffer_shader.SetUniform("u_terrain_mode", 1);
		gbuffer_shader.SetUniform<unsigned int>("u_material_id", 0);
		gbuffer_shader.SetUniform<unsigned int>("u_shader_id", shader_library.lighting_shader.GetShaderID());
		Renderer::DrawTerrain();
		gbuffer_shader.SetUniform("u_terrain_mode", 0);

		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, Renderer::GetScene()->m_skybox.GetCubeMapTexture().GetTextureHandle(), GL_StateManager::TextureUnits::COLOR_CUBEMAP, false);
		gbuffer_shader.SetUniform("u_skybox_mode", 1);
		gbuffer_shader.SetUniform<unsigned int>("u_material_id", shader_library.INVALID_SHADER_ID);
		gbuffer_shader.SetUniform<unsigned int>("u_shader_id", shader_library.GetShader("skybox").GetShaderID());
		Renderer::DrawSkybox();
		gbuffer_shader.SetUniform("u_skybox_mode", 0);


		if (gbuffer_positions_sample_flag) {
			glm::vec2 mouse_coords = glm::min(glm::max(Input::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Window::GetWidth() - 1, Window::GetHeight() - 1));

			GLfloat* pixels = new GLfloat[4];
			glReadPixels(mouse_coords.x, Window::GetHeight() - mouse_coords.y, 1, 1, GL_RGB, GL_FLOAT, pixels);
			current_pos = glm::vec3(pixels[0], pixels[1], pixels[2]);
			delete[] pixels;
			gbuffer_positions_sample_flag = false;
			OAR_CORE_INFO("{0}, {1}, {2}", current_pos.x, current_pos.y, current_pos.z);
		}
	}
}