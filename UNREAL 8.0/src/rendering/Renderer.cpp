#include <iostream>
#include <glm/gtx/matrix_major_storage.hpp>
#include "Renderer.h"
#include "ExtraMath.h"
#include <future>


void Renderer::Init() {

	shaderLibrary.Init();
	grid_mesh.Init();
	skybox.Init();


	auto cube = scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
	auto cube2 = scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
	auto cube3 = scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
	for (float i = 0; i < 36.0f; i++) {
		auto l = scene.CreateLight();
		l->SetColor(i / 33.0f, cosf(ExtraMath::ToRadians(i * 10)), sinf(ExtraMath::ToRadians(i * 10)));
	}
	scene.LoadScene();
	cube->SetPosition(10.0f, 0.0f, 0.0f);
	cube2->SetPosition(0.0f, 0.0f, 0.0f);
	cube3->SetPosition(5.0f, 0.0f, 0.0f);
	cube3->SetScale(50.0f, 5.0f, 50.0f);

	PrintUtils::PrintSuccess("Renderer initialized");
}

void Renderer::DrawGrid() {
	shaderLibrary.grid_shader.ActivateProgram();
	shaderLibrary.grid_shader.SetProjection(glm::colMajor4(projectionMatrix));
	shaderLibrary.grid_shader.SetCamera(glm::colMajor4(p_camera->GetMatrix()));
	shaderLibrary.grid_shader.SetCameraPos(p_camera->GetPos());
	grid_mesh.CheckBoundary(p_camera->GetPos());
	grid_mesh.Draw();
}


void Renderer::RenderScene() {
	static glm::fvec3 atten_vals = glm::fvec3(1.0f, 0.05f, 0.01f);
	static float angle = 0;
	static float x = 0.0f;
	static float z = 0.0f;
	float angle_offset = 0.0f;

	shaderLibrary.flat_color_shader.ActivateProgram();
	for (auto light : scene.GetPointLights()) {
		light->SetAttenuation(atten_vals.x, atten_vals.y, atten_vals.z);
		angle += 0.01f;
		angle_offset += 10.0f;
		x = 30.0f * cosf(ExtraMath::ToRadians(angle + angle_offset));
		z = 30.0f * sinf(ExtraMath::ToRadians(angle + angle_offset));
		light->SetPosition(x, 10.0f, z);
		shaderLibrary.flat_color_shader.SetWVP(glm::colMajor4(light->GetWorldTransform().GetMatrix()) * glm::colMajor4(p_camera->GetMatrix()) * glm::colMajor4(projectionMatrix));

		glm::fvec3 light_color = light->GetColor();
		shaderLibrary.flat_color_shader.SetColor(light_color.x, light_color.y, light_color.z);
		light->GetCubeVisual()->GetMeshData()->Render(1);
	}

	shaderLibrary.lighting_shader.ActivateProgram();
	shaderLibrary.lighting_shader.SetProjection(glm::colMajor4(projectionMatrix));
	shaderLibrary.lighting_shader.SetCamera(glm::colMajor4(p_camera->GetMatrix()));

	shaderLibrary.lighting_shader.SetPointLights(scene.GetPointLights());
	shaderLibrary.lighting_shader.SetAmbientLight(scene.GetAmbientLighting());
	shaderLibrary.lighting_shader.SetViewPos(p_camera->GetPos());

	for (auto group : scene.GetGroupMeshEntities()) {
		if (group->GetMeshData()->GetLoadStatus() == true && group->GetMeshData()->GetShaderMode() == MeshShaderMode::LIGHTING) {
			shaderLibrary.lighting_shader.SetMaterial(group->GetMeshData()->GetMaterial());
			group->GetMeshData()->Render(group->GetInstances());
		}
	}

	skybox.Draw(ExtraMath::GetCameraTransMatrix(p_camera->GetPos()) * p_camera->GetMatrix() * projectionMatrix);
	DrawGrid();

	ControlWindow::CreateBaseWindow();
	atten_vals = ControlWindow::DisplayAttenuationControls();
	ControlWindow::Render();



}

