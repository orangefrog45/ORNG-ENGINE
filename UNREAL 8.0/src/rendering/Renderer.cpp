#include <iostream>
#include <glm/gtx/matrix_major_storage.hpp>
#include <glm/gtx/transform.hpp>
#include <format>
#include <GLErrorHandling.h>
#include "Renderer.h"
#include "ExtraMath.h"
#include "RendererData.h"


void Renderer::Init() {
	double time_start = glfwGetTime();

	shaderLibrary.Init();
	framebuffer_library.Init();
	grid_mesh.Init();
	skybox.Init();
	render_quad.Load();


	auto& cube = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& orange4 = scene.CreateMeshComponent("./res/meshes/oranges/orange.obj", MeshShaderMode::LIGHTING);
	auto& cube2 = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& cube3 = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& cube4 = scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
	auto& orange = scene.CreateMeshComponent("./res/meshes/light meshes/cone.obj", MeshShaderMode::REFLECT);
	auto& orange3 = scene.CreateMeshComponent("./res/meshes/light meshes/cone.obj", MeshShaderMode::REFLECT);
	auto& orange2 = scene.CreateMeshComponent("./res/meshes/oranges/orange.obj", MeshShaderMode::REFLECT);
	/*for (float i = 0; i < 18.0f; i++) {
		auto l = scene.CreatePointLight();
		l->SetColor(i / 108.0f, cosf(ExtraMath::ToRadians(i * 10.0f / 3.0f)), sinf(ExtraMath::ToRadians(i * 10.0f / 3.0f)));
	}*/
	orange.SetPosition(0.0f, 7.0f, 0.0f);
	orange3.SetPosition(2.0f, 7.0f, 0.0f);
	orange2.SetPosition(10.0f, 20.0f, 0.0f);
	orange2.SetScale(1.0f, 1.0f, 1.0f);
	cube2.SetPosition(50.0f, 10.0f, 0.0f);
	cube3.SetScale(50.0f, 1.0f, 50.0f);
	cube3.SetRotation(0.0f, 0.0f, 0.0f);
	cube4.SetPosition(0.0f, 3.0f, 10.0f);
	cube4.SetScale(3.0f, 3.0f, 3.0f);
	cube4.SetRotation(0.0f, 45.0f, 0.0f);
	cube.SetPosition(0.0f, 0.0f, -25.0f);
	cube.SetScale(50.0f, 50.0f, 1.0f);
	cube.SetRotation(0.0f, 0.0f, 0.0f);
	auto& sl = scene.CreateSpotLight();
	auto& sl2 = scene.CreateSpotLight();
	auto& sl3 = scene.CreateSpotLight();
	auto& sl4 = scene.CreateSpotLight();

	sl.SetPosition(0.0f, 10.0f, 20.0f);
	sl2.SetPosition(20.0f, 10.0f, 20.0f);
	sl3.SetPosition(-20.0f, 10.0f, 20.0f);
	sl4.SetPosition(40.0f, 10.0f, 20.0f);
	sl.SetColor(0.0f, 1.f, 0.0f);
	sl2.SetColor(0.0f, 0.0f, 1.f);
	sl3.SetColor(1.f, 0.0f, 0.0f);
	sl4.SetColor(1.f, 1.f, 0.f);
	sl2.SetLightDirection(0.0f, 0.0f, -1.0f);
	sl3.SetLightDirection(0.0f, 0.0f, -1.0f);
	sl4.SetLightDirection(0.0f, 0.0f, -1.0f);
	sl.SetLightDirection(0.0f, 0.0f, -1.0f);
	static float angle = 0;
	static float x = 0.0f;
	static float z = 0.0f;
	float angle_offset = 0.0f;

	for (auto& light : scene.GetPointLights()) {
		angle += 0.005 / 3.0f;
		angle_offset += 10.0f / 3.0f;
		x = 30.0f * cosf(ExtraMath::ToRadians(angle + angle_offset));
		z = 30.0f * sinf(ExtraMath::ToRadians(angle + angle_offset));
		light.SetPosition(x, 10.0f, z);
	}
	scene.LoadScene();

	PrintUtils::PrintSuccess(std::format("Renderer initialized in {}ms", PrintUtils::RoundDouble((glfwGetTime() - time_start) * 1000)));
}

void Renderer::RenderWindow() {
	static LightConfigValues light_vals;
	static DebugConfigValues config_vals;
	static DirectionalLightConfigValues dir_light_vals;
	static SceneData scene_debug_data;
	//SHADOW MAP PASS
	DrawShadowMap();

	//MAIN DRAW
	scene.GetDirectionalLight().SetLightDirection(dir_light_vals.light_position);
	scene.GetDirectionalLight().SetColor(dir_light_vals.light_color.x, dir_light_vals.light_color.y, dir_light_vals.light_color.z);
	DrawScene();

	//DRAW TO QUAD
	glClear(GL_COLOR_BUFFER_BIT);
	shaderLibrary.basic_sampler_shader.ActivateProgram();
	glActiveTexture(RendererData::TextureUnits::COLOR);
	glBindTexture(GL_TEXTURE_2D_ARRAY, framebuffer_library.main_view_framebuffer.GetTexture());

	glDisable(GL_DEPTH_TEST);

	glBindTexture(GL_TEXTURE_2D, framebuffer_library.main_view_framebuffer.GetTexture());
	shaderLibrary.basic_sampler_shader.SetTransform(render_quad.GetTransform().GetMatrix());
	render_quad.Draw();

	glEnable(GL_DEPTH_TEST);

	unsigned int total_vertices = 0;
	unsigned int total_lights = 0;

	total_lights += scene.GetSpotLights().size();
	total_lights += scene.GetPointLights().size();

	for (const auto& group : scene.GetGroupMeshEntities()) {
		total_vertices += group.GetInstances() * group.GetMeshData()->GetIndicesCount();
	}

	scene_debug_data.total_vertices = total_vertices;
	scene_debug_data.num_lights = total_lights;

	ControlWindow::CreateBaseWindow();
	ControlWindow::DisplaySceneData(scene_debug_data);
	ControlWindow::DisplayPointLightControls(scene.GetPointLights().size(), ActivateLightingControls(light_vals));
	ControlWindow::DisplayDebugControls(config_vals, framebuffer_library.dir_depth_fb.GetDepthMap());
	ControlWindow::DisplayDirectionalLightControls(dir_light_vals);
	ControlWindow::Render();

}
void Renderer::DrawToQuad() {

}

void Renderer::DrawScene() {
	glViewport(0, 0, m_window_width, m_window_height);

	shaderLibrary.reflection_shader.ActivateProgram();
	framebuffer_library.main_view_framebuffer.Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(RendererData::TextureUnits::DIR_SHADOW_MAP);
	glBindTexture(GL_TEXTURE_2D, framebuffer_library.dir_depth_fb.GetDepthMap());

	glActiveTexture(RendererData::TextureUnits::SPOT_SHADOW_MAP);
	glBindTexture(GL_TEXTURE_2D_ARRAY, framebuffer_library.spotlight_depth_fb.GetDepthMap());

	RenderLightMeshVisuals();
	RenderLightingEntities();
	RenderReflectShaderEntities();
	glActiveTexture(RendererData::TextureUnits::COLOR);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.GetCubeMapTexture());
	skybox.Draw(glm::mat3(p_camera->GetMatrix()));
	DrawGrid();

	framebuffer_library.UnbindAllFramebuffers();
}

void Renderer::DrawShadowMap() {
	//BIND FOR DRAW
	shaderLibrary.depth_shader.ActivateProgram();

	//DIRECTIONAL LIGHT
	framebuffer_library.dir_depth_fb.BindForDraw();
	glClear(GL_DEPTH_BUFFER_BIT);

	shaderLibrary.depth_shader.SetPVMatrix(scene.GetDirectionalLight().GetTransformMatrix());
	DrawLightingGroups(shaderLibrary.depth_shader);
	DrawReflectGroups(shaderLibrary.depth_shader);

	//SPOT LIGHTS
	framebuffer_library.spotlight_depth_fb.BindForDraw();

	auto& lights = scene.GetSpotLights();
	for (unsigned int i = 0; i < lights.size(); i++) {

		framebuffer_library.spotlight_depth_fb.SetDepthTexLayer(i); // index 0 = directional light depth map
		glClear(GL_DEPTH_BUFFER_BIT);

		shaderLibrary.depth_shader.SetPVMatrix(lights[i].GetTransformMatrix());
		DrawLightingGroups(shaderLibrary.depth_shader);
		DrawReflectGroups(shaderLibrary.depth_shader);


	}

	framebuffer_library.UnbindAllFramebuffers();
}

LightConfigValues& Renderer::ActivateLightingControls(LightConfigValues& light_vals) {

	for (auto& light : scene.GetPointLights()) {
		light.SetAttenuation(light_vals.atten_constant, light_vals.atten_linear, light_vals.atten_exp);
		light.SetMaxDistance(light_vals.max_distance);
		if (!light_vals.lights_enabled) {
			light.SetMaxDistance(0);
		}
	}


	return light_vals;
}


void Renderer::RenderLightMeshVisuals() {
	shaderLibrary.flat_color_shader.ActivateProgram();
	for (const auto& light : scene.GetPointLights()) {
		glm::fvec3 light_color = light.GetColor();
		shaderLibrary.flat_color_shader.SetWorldTransform(light.GetMeshVisual()->GetWorldTransform()->GetMatrix());
		shaderLibrary.flat_color_shader.SetColor(light_color.x, light_color.y, light_color.z);
		DrawMeshWithShader(light.GetMeshVisual()->GetMeshData(), 1, shaderLibrary.flat_color_shader);
	}

	for (const auto& light : scene.GetSpotLights()) {
		glm::fvec3 light_color = light.GetColor();
		shaderLibrary.flat_color_shader.SetWorldTransform(light.GetMeshVisual()->GetWorldTransform()->GetMatrix());
		shaderLibrary.flat_color_shader.SetColor(light_color.x, light_color.y, light_color.z);
		DrawMeshWithShader(light.GetMeshVisual()->GetMeshData(), 1, shaderLibrary.flat_color_shader);
	}
}

void Renderer::RenderReflectShaderEntities() {

	shaderLibrary.reflection_shader.ActivateProgram();
	shaderLibrary.reflection_shader.SetCameraPos(p_camera->GetPos());
	glActiveTexture(RendererData::TextureUnits::COLOR);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.GetCubeMapTexture());

	DrawReflectGroups(shaderLibrary.reflection_shader);
}


void Renderer::RenderLightingEntities() {

	shaderLibrary.lighting_shader.ActivateProgram();
	auto cam_mat = p_camera->GetMatrix();

	shaderLibrary.lighting_shader.SetPointLights(scene.GetPointLights());
	shaderLibrary.lighting_shader.SetSpotLights(scene.GetSpotLights());
	shaderLibrary.lighting_shader.SetAmbientLight(scene.GetAmbientLighting());
	shaderLibrary.lighting_shader.SetDirectionLight(scene.GetDirectionalLight());
	shaderLibrary.lighting_shader.SetViewPos(p_camera->GetPos());
	shaderLibrary.lighting_shader.SetMatrixUBOs(projectionMatrix, cam_mat);
	shaderLibrary.lighting_shader.SetLightSpaceMatrix(scene.GetDirectionalLight().GetTransformMatrix());

	DrawLightingGroups(shaderLibrary.lighting_shader);
}

void Renderer::DrawGrid() {
	shaderLibrary.grid_shader.ActivateProgram();
	shaderLibrary.grid_shader.SetProjection(projectionMatrix);
	shaderLibrary.grid_shader.SetCamera(p_camera->GetMatrix());
	shaderLibrary.grid_shader.SetCameraPos(p_camera->GetPos());
	grid_mesh.CheckBoundary(p_camera->GetPos());
	grid_mesh.Draw();
}

template <typename T> void Renderer::DrawLightingGroups(T& shader) {
	for (auto& group : scene.GetGroupMeshEntities()) {
		if (group.GetMeshData()->GetLoadStatus() == true && (group.GetShaderType() == MeshShaderMode::LIGHTING)) {
			MeshData* mesh_data = group.GetMeshData();

			if (group.GetMeshData()->GetIsShared() == true) group.UpdateMeshTransformBuffers();

			DrawMeshWithShader(mesh_data, group.GetInstances(), shader);
		}
	}
}

template <typename T> void Renderer::DrawReflectGroups(T& shader) {
	for (auto& group : scene.GetGroupMeshEntities()) {
		if (group.GetMeshData()->GetLoadStatus() == true && (group.GetShaderType() == MeshShaderMode::REFLECT)) {
			MeshData* mesh_data = group.GetMeshData();

			if (group.GetMeshData()->GetIsShared() == true) group.UpdateMeshTransformBuffers();

			DrawMeshWithShader(mesh_data, group.GetInstances(), shader);
		}
	}
};

template <typename T> void Renderer::DrawMeshWithShader(MeshData* mesh_data, unsigned int t_instances, T& shader) const {
	GLCall(glBindVertexArray(mesh_data->m_VAO));

	for (unsigned int i = 0; i < mesh_data->m_meshes.size(); i++) {

		unsigned int materialIndex = mesh_data->m_meshes[i].materialIndex;
		ASSERT(materialIndex < mesh_data->m_textures.size());

		if (mesh_data->m_materials[materialIndex].specular_texture != nullptr) mesh_data->m_materials[materialIndex].specular_texture->Bind(RendererData::TextureUnits::SPECULAR);
		mesh_data->m_materials[materialIndex].diffuse_texture->Bind(RendererData::TextureUnits::COLOR); // no check required as there will always be a texture due to the default texture system

		shader.SetMaterial(mesh_data->m_materials[materialIndex]);

		GLCall(glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
			mesh_data->m_meshes[i].numIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * mesh_data->m_meshes[i].baseIndex),
			t_instances,
			mesh_data->m_meshes[i].baseVertex));

	}
	GLCall(glBindVertexArray(0))
}

