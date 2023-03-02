#include <glew.h>
#include <iostream>
#include <execution>
#include <glm/gtx/matrix_major_storage.hpp>
#include "Renderer.h"
#include "ExtraMath.h"
#include <future>


void Renderer::Init() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glewInit();
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(m_window_width, m_window_height);

	glutInitWindowPosition(200, 100);

	int win = glutCreateWindow("UNREAL 8.0");

	unsigned int res = glewInit();
	if (GLEW_OK != res)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(res));
		exit(1);
	}

	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	printf("window id: %d\n", win);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(3.0);

	shaderLibrary.Init();
	grid_mesh.Init();
	skybox.Init();

	//TODO : make mesh loading multithreaded, going to require re-working the insta-load system, load meshes all at once in LoadScene().
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



	for (auto light : scene.GetPointLights()) {
		shaderLibrary.flat_color_shader.ActivateProgram();
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
}

