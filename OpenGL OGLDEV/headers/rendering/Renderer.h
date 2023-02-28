#pragma once
#include <memory>
#include "ExtraMath.h"
#include "ShaderLibrary.h"
#include "BasicMesh.h"
#include "GridMesh.h"
#include "Skybox.h"
#include "Scene.h"
#include "Camera.h"



class Renderer {
public:
	Renderer(std::shared_ptr<Camera> cam) : p_camera(cam) {};
	void Init();
	std::vector<std::shared_ptr<BasicMesh>> lightingShaderMeshes;


	glm::fvec3 lightColor = glm::fvec3(1.0f, 0.0f, 0.5f);

	void AnimateGeometry();
	void RenderScene();
	void DrawGrid();

	Scene scene;

private:
	float FOV = 60.0f;
	float zNear = 0.01f;
	float zFar = 1000.0f;
	int m_window_width = RenderData::WINDOW_WIDTH;
	int m_window_height = RenderData::WINDOW_HEIGHT;

	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(FOV, m_window_width, m_window_height, zNear, zFar);
	std::shared_ptr<Camera> p_camera = nullptr;
	Skybox skybox;
	GridMesh grid_mesh;
	ShaderLibrary shaderLibrary;

};