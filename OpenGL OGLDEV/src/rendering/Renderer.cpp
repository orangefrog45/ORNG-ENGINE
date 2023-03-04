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
	auto light = scene.CreateLight();
	auto light2 = scene.CreateLight();
	auto light3 = scene.CreateLight();
	scene.LoadScene();
	light->SetColor(0.0f, 0.0f, 1.0f);
	light->SetPosition(10.0f, 10.0f, 0.0f);
	light2->SetPosition(0.0f, 10.0f, 0.0f);
	light3->SetPosition(5.0f, 10.0f, 5.0f);
	light3->SetColor(0.0f, 1.0f, 0.0f);
	light2->SetColor(1.0f, 0.0f, 0.0f);
	cube->SetPosition(10.0f, 0.0f, 0.0f);
	cube2->SetPosition(0.0f, 0.0f, 0.0f);
	cube3->SetPosition(5.0f, 0.0f, 0.0f);
	cube3->SetScale(50.0f, 5.0f, 50.0f);

	/*static float angle = 0.0f;
	static float row_offset = 0.0f;
	static float x = 0.0f;
	static float z = 0.0f;

	for (unsigned int i = 0; i < scene.GetPointLights().size(); i++) {
		angle += (360.0f / 128.0f);
		x += 4.0f * cosf(ExtraMath::ToRadians((angle)));
		z += 4.0f * sinf(ExtraMath::ToRadians((angle)));
		if (angle >= 360) {
			angle = 0.0f;
			x = 0.0f;
		}
		if (z > 30.0f) {
			z = 0.0f;
		}

		scene.GetPointLights()[i]->SetColor(1.0f, 1.0f, 1.0f);
		scene.GetPointLights()[i]->SetPosition(x, 0.0f, z);

	}*/
}

void Renderer::DrawGrid() {
	shaderLibrary.grid_shader.ActivateProgram();
	shaderLibrary.grid_shader.SetProjection(glm::colMajor4(projectionMatrix));
	shaderLibrary.grid_shader.SetCamera(glm::colMajor4(p_camera->GetMatrix()));
	shaderLibrary.grid_shader.SetCameraPos(p_camera->GetPos());
	grid_mesh.CheckBoundary(p_camera->GetPos());
	grid_mesh.Draw();
}

/*void Renderer::AnimateGeometry() {
	auto& transforms = scene.GetMeshEntities()[0].;
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float angle = 0.0f;
	static double offset = 0.0f;
	float rowOffset = 0.0f;
	static float coefficient = 1.0f;

	for (unsigned int i = 0; i < transforms.size(); i++) {
		angle += 10.0f;
		x += 4 * cosf(ExtraMath::ToRadians((angle + rowOffset) * coefficient));
		y += 4 * sinf(ExtraMath::ToRadians((angle + rowOffset) * coefficient));
		if (angle == 180.0f) {
			//coefficient *= -1.0;
		}
		if (angle >= 360) {
			coefficient *= 1.0f;
			angle = 0.0f;
			z += 4.0f;
			x = 0.0f;
			y = 0.0f;
			rowOffset += 9.0f;
		}
		offset += 0.00005f;
		transforms[i].SetPosition((cosf(ExtraMath::ToRadians(offset)) * x) - (sinf(ExtraMath::ToRadians(offset)) * y), (sinf(ExtraMath::ToRadians(offset)) * x) + (cosf(ExtraMath::ToRadians(offset)) * y), z);
		transforms[i].SetRotation(offset, -offset, offset);
		transforms[i].SetScale(1.f, 1.f, 1.f);

	}
}*/

void Renderer::RenderScene() {
	static glm::fvec3 atten_vals = glm::fvec3(0, 0, 0);

	shaderLibrary.flat_color_shader.ActivateProgram();
	for (auto light : scene.GetPointLights()) {
		light->SetAttenuation(atten_vals.x, atten_vals.y, atten_vals.z);
		shaderLibrary.flat_color_shader.SetWVP(glm::colMajor4(light->GetWorldTransform().GetMatrix()) * glm::colMajor4(p_camera->GetMatrix()) * glm::colMajor4(projectionMatrix));
		shaderLibrary.flat_color_shader.SetColor(light->color.x, light->color.y, light->color.z);
		light->cube_visual->GetMeshData()->Render(1);
	}

	shaderLibrary.lighting_shader.ActivateProgram();
	shaderLibrary.lighting_shader.SetProjection(glm::colMajor4(projectionMatrix));
	shaderLibrary.lighting_shader.SetCamera(glm::colMajor4(p_camera->GetMatrix()));

	shaderLibrary.lighting_shader.SetPointLights(scene.GetPointLights());
	shaderLibrary.lighting_shader.SetAmbientLight(scene.GetAmbientLighting());
	shaderLibrary.lighting_shader.SetViewPos(p_camera->GetPos());

	for (EntityInstanceGroup* group : scene.GetGroupMeshEntities()) {
		//TODO : add multiple shader functionality to scene (shadertype member in meshentity probably)
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

