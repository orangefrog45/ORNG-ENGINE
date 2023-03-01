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
	auto robot = scene.CreateMeshEntity("./res/meshes/robot/Robot.obj");
	auto cube = scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
	auto cube2 = scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
	auto cube3 = scene.CreateMeshEntity("./res/meshes/cube/cube.obj");
	auto orange = scene.CreateMeshEntity("./res/meshes/oranges/orange.obj");
	scene.LoadScene();
	cube->SetPosition(10.0f, 0.0f, 0.0f);
	cube2->SetPosition(0.0f, 0.0f, 0.0f);
	cube3->SetPosition(5.0f, 0.0f, 0.0f);
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
	shaderLibrary.lighting_shader.ActivateProgram();
	shaderLibrary.lighting_shader.SetProjection(glm::colMajor4(projectionMatrix));
	shaderLibrary.lighting_shader.SetCamera(glm::colMajor4(p_camera->GetMatrix()));

	PointLight point_light = PointLight(glm::fvec3(0.0f, 10.0f, 0.0f), glm::fvec3(1.0f, 1.0f, 1.0f));

	BaseLight& base_light = scene.GetAmbientLighting();
	base_light.color = glm::fvec3(1.0f, 1.0f, 1.0f);
	base_light.ambient_intensity = 0.2f;
	shaderLibrary.lighting_shader.SetPointLight(point_light);
	shaderLibrary.lighting_shader.SetAmbientLight(scene.GetAmbientLighting());
	shaderLibrary.lighting_shader.SetViewPos(p_camera->GetPos());

	for (EntityInstanceGroup* group : scene.GetGroupMeshEntities()) {
		//TODO : add multiple shader functionality to scene (shadertype member in meshentity probably)
		if (group->m_mesh_data->GetLoadStatus() == true) {
			shaderLibrary.lighting_shader.SetMaterial(group->m_mesh_data->GetMaterial());
			group->m_mesh_data->Render(group->m_instances);
		}
	}

	skybox.Draw(ExtraMath::GetCameraTransMatrix(p_camera->GetPos()) * p_camera->GetMatrix() * projectionMatrix);
	DrawGrid();
}
