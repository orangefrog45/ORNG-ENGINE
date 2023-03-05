#pragma once
#include <memory>
#include <gui/ControlWindow.h>
#include "ExtraMath.h"
#include "ShaderLibrary.h"
#include "BasicMesh.h"
#include "GridMesh.h"
#include "Skybox.h"
#include "Scene.h"
#include "Camera.h"



class Renderer {
public:
	explicit Renderer(std::shared_ptr<Camera> cam) : p_camera(cam) {};
	void Init();
	void RenderScene();
	void DrawGrid();
	void DrawLightingEntities();
	void DrawPointLights();
	template <typename T> static void DrawMeshWithShader(BasicMesh* mesh_data, unsigned int t_instances, T& shader);
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