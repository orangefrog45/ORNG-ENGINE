#include <glm/gtx/matrix_major_storage.hpp>
#include <glm/gtx/transform.hpp>
#include <GLErrorHandling.h>
#include "Renderer.h"
#include "ExtraMath.h"
#include "Log.h"

void Renderer::Init() {
	TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

	shader_library.Init();
	framebuffer_library.Init();
	grid_mesh.Init();
	m_resources.Init();
	m_active_camera = &scene.m_camera;
	render_quad.Load();
	auto& cube = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& orange4 = scene.CreateMeshComponent("./res/meshes/oranges/orange.obj");
	auto& cube2 = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& cube3 = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& cube4 = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& orange = scene.CreateMeshComponent("./res/meshes/light meshes/cone.obj", MeshData::MeshShaderMode::REFLECT);
	auto& orange3 = scene.CreateMeshComponent("./res/meshes/light meshes/cone.obj", MeshData::MeshShaderMode::REFLECT);
	auto& tank = scene.CreateMeshComponent("./res/meshes/tank/SM_TankBase_Internal.OBJ");
	auto& tankturret = scene.CreateMeshComponent("./res/meshes/tank/SM_TankTurret_Internal.OBJ");

	tank.SetPosition(0, 10.0, 0.0);
	tankturret.SetPosition(0, 10.0, 0.0);
	orange.SetPosition(0.0f, 7.0f, 0.0f);
	orange3.SetPosition(2.0f, 7.0f, 0.0f);
	cube2.SetPosition(50.0f, 10.0f, 0.0f);
	cube3.SetScale(10.0f, 10.0f, 10.0f);
	cube3.SetRotation(0.0f, 0.0f, 0.0f);
	cube4.SetPosition(0.0f, 3.0f, 10.0f);
	cube4.SetScale(3.0f, 3.0f, 3.0f);
	cube4.SetRotation(0.0f, 45.0f, 0.0f);
	cube.SetPosition(0.0f, 0.0f, -25.0f);
	cube.SetRotation(0.0f, 0.0f, 0.0f);
	auto& pl2 = scene.CreatePointLight();
	auto& pl = scene.CreatePointLight();
	auto& sl = scene.CreateSpotLight();
	auto& sl2 = scene.CreateSpotLight();
	auto& sl3 = scene.CreateSpotLight();
	auto& sl4 = scene.CreateSpotLight();

	pl.SetPosition(20.0f, 3.0f, 10.0f);
	pl.SetColor(1.0f, 0.0f, 0.0f);

	pl2.SetPosition(10.0f, 3.0f, 10.0f);
	pl2.SetColor(1.0f, 0.0f, 0.0f);

	sl.SetPosition(0.0f, 50.0f, 20.0f);
	sl2.SetPosition(20.0f, 10.0f, 50.0f);
	sl3.SetPosition(-20.0f, 3.0f, 50.0f);
	sl4.SetPosition(40.0f, 10.0f, 20.0f);
	sl.SetColor(0.0f, 1.f, 0.0f);
	sl2.SetColor(0.0f, 0.0f, 1.f);
	sl3.SetColor(1.f, 0.0f, 0.0f);
	sl4.SetColor(1.f, 1.f, 0.f);
	sl2.SetLightDirection(0.0f, 0.0f, -1.0f);
	sl3.SetLightDirection(0.0f, 0.0f, -1.0f);
	sl4.SetLightDirection(0.0f, 0.0f, -1.0f);
	sl.SetLightDirection(0.0f, 0.0f, -1.0f);
	scene.LoadScene();

	ORO_CORE_INFO("Renderer initialized in {0}ms", time.GetTimeInterval());
}

void Renderer::RenderWindow() {
	static ControlWindow::LightConfigData light_vals;
	static ControlWindow::DebugConfigData config_vals;
	static ControlWindow::DirectionalLightData dir_light_vals;
	static ControlWindow::SceneData scene_debug_data;
	static ControlWindow::TerrainConfigData terrain_data;
	static ControlWindow::TerrainConfigData saved_terrain_data;

	/* Update UBO's immediately to stop out-of-sync draws */
	glm::fmat4 cam_mat = m_active_camera->GetViewMatrix();
	glm::fmat4 proj_mat = m_active_camera->GetProjectionMatrix();
	shader_library.SetMatrixUBOs(proj_mat, cam_mat);

	//SHADOW MAP PASS
	scene.GetDirectionalLight().SetLightDirection(dir_light_vals.light_position, m_active_camera->GetPos());
	scene.GetDirectionalLight().SetColor(dir_light_vals.light_color.x, dir_light_vals.light_color.y, dir_light_vals.light_color.z);
	DrawShadowMap();

	//GBUFFER PASS
	glClearColor(10000, 10000, 10000, 10000);
	framebuffer_library.deferred_fb.BindForDraw();
	shader_library.g_buffer_shader.ActivateProgram();
	glClearColor(0, 0, 0, 0);
	DrawLightingGroups(shader_library.g_buffer_shader);
	DrawReflectGroups(shader_library.g_buffer_shader);
	DrawTerrain(scene.m_terrain);

	//MAIN DRAW
	RenderScene();

	//DRAW TO QUAD
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	shader_library.basic_sampler_shader.ActivateProgram();
	RendererResources::BindTexture(GL_TEXTURE_2D, framebuffer_library.main_view_framebuffer.GetTexture(), RendererResources::TextureUnits::COLOR);
	RendererResources::BindTexture(GL_TEXTURE_2D, framebuffer_library.main_view_framebuffer.GetDepthTexture(), RendererResources::TextureUnits::DIR_SHADOW_MAP);
	shader_library.basic_sampler_shader.SetCameraPos(m_active_camera->GetPos());
	shader_library.basic_sampler_shader.SetCameraDir(m_active_camera->GetTarget());
	shader_library.basic_sampler_shader.SetTransform(render_quad.GetTransform().GetMatrix());
	render_quad.Draw();

	glEnable(GL_DEPTH_TEST);
	unsigned int total_vertices = 0;
	unsigned int total_lights = 0;

	total_lights += scene.GetSpotLights().size();
	total_lights += scene.GetPointLights().size();

	for (const auto& group : scene.GetGroupMeshEntities()) {
		total_vertices += group->GetInstances() * group->GetMeshData()->GetIndicesCount();
	}

	total_vertices += scene.m_terrain.m_terrain_data.positions.size();

	scene_debug_data.total_vertices = total_vertices;
	scene_debug_data.num_lights = total_lights;
	if (!(saved_terrain_data == terrain_data)) {
		saved_terrain_data = terrain_data;
		scene.GetTerrain().UpdateTerrain(terrain_data.seed, 1000.0f, 1000.0f, glm::fvec3(0, -20, 0), terrain_data.resolution, terrain_data.height_scale, terrain_data.sampling_density);
	}

	ControlWindow::CreateBaseWindow();
	ControlWindow::DisplaySceneData(scene_debug_data);
	ControlWindow::DisplayPointLightControls(scene.GetPointLights().size(), ActivateLightingControls(light_vals));
	ControlWindow::DisplayDebugControls(config_vals, framebuffer_library.dir_depth_fb.GetDepthMap());
	ControlWindow::DisplayDirectionalLightControls(dir_light_vals);
	ControlWindow::DisplayTerrainConfigControls(terrain_data);
	ControlWindow::Render();

}

void Renderer::RenderSkybox(Skybox& skybox) {
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	shader_library.skybox_shader.ActivateProgram();
	RendererResources::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemap_texture.m_texture_obj, RendererResources::TextureUnits::COLOR);

	glBindVertexArray(skybox.m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}

void Renderer::RenderScene() {
	glViewport(0, 0, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight());

	shader_library.reflection_shader.ActivateProgram();
	framebuffer_library.main_view_framebuffer.Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RendererResources::BindTexture(GL_TEXTURE_2D, framebuffer_library.dir_depth_fb.GetDepthMap(), RendererResources::TextureUnits::DIR_SHADOW_MAP);
	RendererResources::BindTexture(GL_TEXTURE_2D, framebuffer_library.spotlight_depth_fb.GetDepthMap(), RendererResources::TextureUnits::SPOT_SHADOW_MAP);

	RenderLightMeshVisuals();
	RenderLightingEntities();
	RenderReflectShaderEntities();

	RenderSkybox(scene.m_skybox);
	RenderGrid();

	framebuffer_library.UnbindAllFramebuffers();
}

void Renderer::DrawTerrain(const Terrain& terrain) {
	glBindVertexArray(terrain.GetVao());

	GLCall(glDrawArrays(GL_TRIANGLES,
		0,
		terrain.m_terrain_data.positions.size()));

	glBindVertexArray(0);
}

void Renderer::DrawShadowMap() {
	//BIND FOR DRAW
	shader_library.depth_shader.ActivateProgram();

	//DIRECTIONAL LIGHT
	framebuffer_library.dir_depth_fb.BindForDraw();
	glClear(GL_DEPTH_BUFFER_BIT);

	shader_library.depth_shader.SetPVMatrix(scene.GetDirectionalLight().GetTransformMatrix());
	DrawLightingGroups(shader_library.depth_shader);
	DrawReflectGroups(shader_library.depth_shader);
	DrawTerrain(scene.m_terrain);

	//SPOT LIGHTS
	framebuffer_library.spotlight_depth_fb.BindForDraw();
	auto& lights = scene.GetSpotLights();

	for (unsigned int i = 0; i < lights.size(); i++) {

		framebuffer_library.spotlight_depth_fb.SetDepthTexLayer(i); // index 0 = directional light depth map
		shader_library.depth_shader.SetPVMatrix(lights[i]->GetTransformMatrix());
		glClear(GL_DEPTH_BUFFER_BIT);

		DrawLightingGroups(shader_library.depth_shader);
		DrawReflectGroups(shader_library.depth_shader);
		DrawTerrain(scene.m_terrain);
	}

	//POINT LIGHTS
	/*shader_library.cube_map_shadow_shader.ActivateProgram();
	framebuffer_library.pointlight_depth_fb.BindForDraw();
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	glClearColor(0.2f, 0.5f, FLT_MAX, FLT_MAX);
	auto& point_lights = scene.GetPointLights();
	for (int i = 0; i < point_lights.size(); i++) {
		shader_library.cube_map_shadow_shader.SetLightPos(point_lights[i].GetWorldTransform().GetPosition());
		for (int y = 0; y < 6; y++) {
			framebuffer_library.pointlight_depth_fb.BindForDraw();
			framebuffer_library.pointlight_depth_fb.SetCubemapFace(i, y);
			GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

			shader_library.cube_map_shadow_shader.SetPVMatrix(point_lights[i].GetLightTransforms()[y]);
			DrawLightingGroups(shader_library.cube_map_shadow_shader);
			DrawReflectGroups(shader_library.cube_map_shadow_shader);
			glReadPixels(0, 0, 1024, 1024, GL_RED, GL_FLOAT, pixels);

			PrintUtils::PrintDebug(std::to_string(pixels[20000]));
		}
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);*/

	framebuffer_library.UnbindAllFramebuffers();
}

ControlWindow::LightConfigData& Renderer::ActivateLightingControls(ControlWindow::LightConfigData& light_vals) {

	for (auto& light : scene.GetPointLights()) {
		light->SetAttenuation(light_vals.atten_constant, light_vals.atten_linear, light_vals.atten_exp);
		light->SetMaxDistance(light_vals.max_distance);
		if (!light_vals.lights_enabled) {
			light->SetMaxDistance(0);
		}
	}

	return light_vals;
}


void Renderer::RenderLightMeshVisuals() {
	shader_library.flat_color_shader.ActivateProgram();
	for (const auto& light : scene.GetPointLights()) {
		glm::fvec3 light_color = light->GetColor();
		shader_library.flat_color_shader.SetColor(light_color.x, light_color.y, light_color.z);
		DrawMeshWithShader(light->GetMeshVisual()->GetMeshData(), 1, shader_library.flat_color_shader);
	}

	for (const auto& light : scene.GetSpotLights()) {
		glm::fvec3 light_color = light->GetColor();
		shader_library.flat_color_shader.SetColor(light_color.x, light_color.y, light_color.z);
		DrawMeshWithShader(light->GetMeshVisual()->GetMeshData(), 1, shader_library.flat_color_shader);
	}
}

void Renderer::RenderReflectShaderEntities() {
	shader_library.reflection_shader.ActivateProgram();
	shader_library.reflection_shader.SetCameraPos(m_active_camera->GetPos());
	RendererResources::BindTexture(GL_TEXTURE_CUBE_MAP, scene.m_skybox.GetCubeMapTexture().GetTextureRef(), RendererResources::TextureUnits::COLOR);
	DrawReflectGroups(shader_library.reflection_shader);
}


void Renderer::RenderLightingEntities() {

	shader_library.lighting_shader.ActivateProgram();

	shader_library.lighting_shader.SetPointLights(scene.GetPointLights());
	shader_library.lighting_shader.SetSpotLights(scene.GetSpotLights());
	shader_library.lighting_shader.SetAmbientLight(scene.GetAmbientLighting());
	shader_library.lighting_shader.SetDirectionLight(scene.GetDirectionalLight());
	shader_library.lighting_shader.SetViewPos(m_active_camera->GetPos());
	shader_library.lighting_shader.SetLightSpaceMatrix(scene.GetDirectionalLight().GetTransformMatrix());

	DrawLightingGroups(shader_library.lighting_shader);

	RendererResources::BindTexture(GL_TEXTURE_2D, scene.m_terrain.m_terrain_top_mat.diffuse_texture->GetTextureRef(), RendererResources::TextureUnits::COLOR);

	shader_library.lighting_shader.SetMaterial(scene.m_terrain.m_terrain_top_mat);
	DrawTerrain(scene.GetTerrain());

}

void Renderer::RenderGrid() {
	shader_library.grid_shader.ActivateProgram();
	shader_library.grid_shader.SetCameraPos(m_active_camera->GetPos());
	grid_mesh.CheckBoundary(m_active_camera->GetPos());
	grid_mesh.Draw();
}

template <typename T> void Renderer::DrawLightingGroups(T& shader) {
	for (auto group : scene.GetGroupMeshEntities()) {
		if (group->GetMeshData()->GetLoadStatus() == true && (group->GetShaderType() == MeshData::MeshShaderMode::LIGHTING)) {
			MeshData* mesh_data = group->GetMeshData();

			if (group->GetMeshData()->GetIsShared() == true) group->UpdateMeshTransformBuffers();

			DrawMeshWithShader(mesh_data, group->GetInstances(), shader);
		}
	}
}

template <typename T> void Renderer::DrawReflectGroups(T& shader) {
	for (auto group : scene.GetGroupMeshEntities()) {
		if (group->GetMeshData()->GetLoadStatus() == true && (group->GetShaderType() == MeshData::MeshShaderMode::REFLECT)) {
			MeshData* mesh_data = group->GetMeshData();

			if (group->GetMeshData()->GetIsShared() == true) group->UpdateMeshTransformBuffers();

			DrawMeshWithShader(mesh_data, group->GetInstances(), shader);
		}
	}
};

template <typename T> void Renderer::DrawMeshWithShader(MeshData* mesh_data, unsigned int t_instances, T& shader) const {
	GLCall(glBindVertexArray(mesh_data->m_VAO));

	for (unsigned int i = 0; i < mesh_data->m_meshes.size(); i++) {

		unsigned int materialIndex = mesh_data->m_meshes[i].materialIndex;

		shader.SetMaterial(mesh_data->m_materials[materialIndex]);

		GLCall(glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
			mesh_data->m_meshes[i].numIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * mesh_data->m_meshes[i].baseIndex),
			t_instances,
			mesh_data->m_meshes[i].baseVertex));

	}
	GLCall(glBindVertexArray(0));
}

