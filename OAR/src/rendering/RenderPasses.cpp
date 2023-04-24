#include "pch/pch.h"

#include "rendering/Renderpasses.h"
#include "rendering/Renderer.h"
#include "components/Camera.h"
#include "rendering/Quad.h"
#include "Input.h"

void RenderPasses::InitPasses() {

	ShaderLibrary& shader_library = Renderer::GetShaderLibrary();
	FramebufferLibrary& fb_library = Renderer::GetFramebufferLibrary();

	Shader& highlight_shader = shader_library.CreateShader("highlight", { "res/shaders/HighlightVS.shader", "res/shaders/HighlightFS.shader" });
	highlight_shader.AddUniform("transform");

	/* 2D quad will just be a place on the screen to render textures onto, currently postprocessing */
	Shader& quad_shader = shader_library.CreateShader("2d_quad", { "res/shaders/QuadVS.shader", "res/shaders/QuadFS.shader" });

	quad_shader.AddUniforms({
		"quad_sampler",
		"world_position_sampler",
		"screen_transform",
		"camera_pos",
		"time"
		});

	quad_shader.SetUniform("quad_sampler", Renderer::TextureUnitIndexes::COLOR);
	quad_shader.SetUniform("world_position_sampler", Renderer::TextureUnitIndexes::WORLD_POSITIONS);

	Shader& reflection_shader = shader_library.CreateShader("reflection", { "./res/shaders/ReflectionVS.shader", "./res/shaders/ReflectionFS.shader" });
	reflection_shader.AddUniform("camera_pos");

	Shader& gbuffer_shader = shader_library.CreateShader("gbuffer", { "res/shaders/GBufferVS.shader", "res/shaders/GBufferFS.shader" });

	Shader& skybox_shader = shader_library.CreateShader("skybox", { "res/shaders/SkyboxVS.shader", "res/shaders/SkyboxFS.shader" });
	skybox_shader.AddUniform("sky_texture");
	skybox_shader.SetUniform("sky_texture", Renderer::TextureUnitIndexes::COLOR);

	Shader& picking_shader = shader_library.CreateShader("picking", { "res/shaders/PickingVS.shader", "res/shaders/PickingFS.shader" });
	picking_shader.AddUniform("comp_id");
	picking_shader.AddUniform("world_transform");

	/* LIGHTING FB */
	Framebuffer& lighting_fb = fb_library.CreateFramebuffer("lighting");
	lighting_fb.Init();
	lighting_fb.AddRenderbuffer();
	lighting_fb.Add2DTexture("render_texture", Renderer::GetWindowWidth(), Renderer::GetWindowHeight(), GL_COLOR_ATTACHMENT0, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	/* GBUFFER FB */
	Framebuffer& gbuffer = fb_library.CreateFramebuffer("gbuffer");
	gbuffer.Init();
	gbuffer.AddRenderbuffer();
	gbuffer.Add2DTexture("world_positions", Renderer::GetWindowWidth(), Renderer::GetWindowHeight(), GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	Framebuffer& picking = fb_library.CreateFramebuffer("picking");
	picking.Init();
	picking.AddRenderbuffer();
	picking.Add2DTexture("component_ids", Renderer::GetWindowWidth(), Renderer::GetWindowHeight(), GL_COLOR_ATTACHMENT0, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


}



void RenderPasses::DoPickingPass() {
	Shader& picking_shader = Renderer::GetShaderLibrary().GetShader("picking");
	Framebuffer& picking_fb = Renderer::GetFramebufferLibrary().GetFramebuffer("picking");
	picking_fb.Bind();
	picking_shader.ActivateProgram();
	GLuint clear_color = 0;
	glClear(GL_DEPTH_BUFFER_BIT);
	glClearBufferuiv(GL_COLOR, 0, &clear_color);


	for (auto& mesh : Renderer::GetScene()->GetMeshComponents()) {
		if (!mesh || !mesh->GetMeshData()) continue;

		if (mesh->GetMeshData()->GetLoadStatus() == true) {
			picking_shader.SetUniform<unsigned int>("comp_id", mesh->GetEntityHandle());
			picking_shader.SetUniform("world_transform", mesh->GetWorldTransform()->GetMatrix());
			Renderer::DrawMesh(mesh->GetMeshData(), false);
		}
	}

	glm::vec2 mouse_coords = glm::min(glm::max(Input::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Renderer::GetWindowWidth() - 1, Renderer::GetWindowHeight() - 1));

	GLuint* pixels = new GLuint[1];
	glReadPixels(mouse_coords.x, Renderer::GetWindowHeight() - mouse_coords.y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, pixels);
	current_entity_id = pixels[0];
	delete[] pixels;
}

void RenderPasses::DoLightingPass() {

	ShaderLibrary& shader_library = Renderer::GetShaderLibrary();
	FramebufferLibrary& framebuffer_library = Renderer::GetFramebufferLibrary();
	LightingShader& lighting_shader = shader_library.lighting_shader;

	glViewport(0, 0, Renderer::GetWindowWidth(), Renderer::GetWindowHeight());
	Renderer::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("dir_depth").GetTexture("depth").tex_ref, Renderer::TextureUnits::DIR_SHADOW_MAP);
	Renderer::BindTexture(GL_TEXTURE_2D_ARRAY, framebuffer_library.GetFramebuffer("spotlight_depth").GetTexture("depth").tex_ref, Renderer::TextureUnits::SPOT_SHADOW_MAP);

	framebuffer_library.GetFramebuffer("lighting").Bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	lighting_shader.ActivateProgram();

	Scene* scene = Renderer::GetScene();

	//lighting_shader.SetPointLights(scene->m_point_lights);
	//lighting_shader.SetSpotLights(scene->m_spot_lights);
	lighting_shader.SetAmbientLight(scene->m_global_ambient_lighting);
	lighting_shader.SetDirectionLight(scene->m_directional_light);
	lighting_shader.SetViewPos(Renderer::GetActiveCamera()->GetPos());
	lighting_shader.SetUniform("dir_light_matrix", scene->m_directional_light.GetTransformMatrix());

	Renderer::DrawGroupsWithBaseShader();

	shader_library.lighting_shader.SetMaterial(scene->m_terrain.m_terrain_top_mat);
	Renderer::BindTexture(GL_TEXTURE_2D_ARRAY, scene->m_terrain.m_diffuse_texture_array.GetTextureRef(), Renderer::TextureUnits::DIFFUSE_ARRAY);
	Renderer::BindTexture(GL_TEXTURE_2D_ARRAY, scene->m_terrain.m_normal_texture_array.GetTextureRef(), Renderer::TextureUnits::NORMAL_ARRAY);
	shader_library.lighting_shader.SetUniform("terrain_mode", 1);
	shader_library.lighting_shader.SetUniform("normal_sampler_active", 1);
	shader_library.lighting_shader.SetUniform("specular_sampler_active", 0);

	//Renderer::DrawTerrain();

	shader_library.lighting_shader.SetUniform("terrain_mode", 0);

	/* DRAW AABBS */
	/*shader_library.GetShader("highlight").ActivateProgram();
	for (auto& mesh : scene->m_mesh_components) {
		if (!mesh) continue;
		shader_library.GetShader("highlight").SetUniform("transform", mesh->GetWorldTransform()->GetMatrix());
		Renderer::DrawBoundingBox(*mesh->GetMeshData());
	}*/

	shader_library.GetShader("skybox").ActivateProgram();
	Renderer::BindTexture(GL_TEXTURE_CUBE_MAP, Renderer::GetScene()->m_skybox.GetCubeMapTexture().GetTextureRef(), Renderer::TextureUnits::COLOR);
	Renderer::DrawSkybox();

	shader_library.GetShader("reflection").ActivateProgram();
	shader_library.GetShader("reflection").SetUniform("camera_pos", Renderer::GetActiveCamera()->GetPos());
	Renderer::DrawGroupsWithShader("reflection");
}

void RenderPasses::DoDrawToQuadPass(Quad* quad) {
	glDisable(GL_DEPTH_TEST);
	Shader& quad_shader = Renderer::GetShaderLibrary().GetShader("2d_quad");
	FramebufferLibrary& framebuffer_library = Renderer::GetFramebufferLibrary();
	framebuffer_library.UnbindAllFramebuffers();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	quad_shader.ActivateProgram();

	//quad_shader.SetUniform<float>("time", );
	quad_shader.SetUniform("camera_pos", Renderer::GetActiveCamera()->GetPos());
	quad_shader.SetUniform("screen_transform", quad->GetTransform().GetMatrix());

	Renderer::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("gbuffer").GetTexture("world_positions").tex_ref, Renderer::TextureUnits::WORLD_POSITIONS);
	Renderer::BindTexture(GL_TEXTURE_2D, framebuffer_library.GetFramebuffer("lighting").GetTexture("render_texture").tex_ref, Renderer::TextureUnits::COLOR);
	Renderer::DrawQuad(*quad);

	glEnable(GL_DEPTH_TEST);
}


void RenderPasses::DoGBufferPass() {
	glViewport(0, 0, Renderer::GetWindowWidth(), Renderer::GetWindowHeight());
	Framebuffer& gbuffer = Renderer::GetFramebufferLibrary().GetFramebuffer("gbuffer");

	gbuffer.Bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	Renderer::GetShaderLibrary().GetShader("gbuffer").ActivateProgram();
	Renderer::DrawAllMeshesInstanced();
	Renderer::DrawTerrain();

	if (gbuffer_positions_sample_flag) {
		glm::vec2 mouse_coords = glm::min(glm::max(Input::GetMousePos(), glm::vec2(1, 1)), glm::vec2(Renderer::GetWindowWidth() - 1, Renderer::GetWindowHeight() - 1));

		GLfloat* pixels = new GLfloat[4];
		glReadPixels(mouse_coords.x, Renderer::GetWindowHeight() - mouse_coords.y, 1, 1, GL_RGB, GL_FLOAT, pixels);
		current_pos = glm::vec3(pixels[0], pixels[1], pixels[2]);
		delete[] pixels;
		gbuffer_positions_sample_flag = false;
	}
}
