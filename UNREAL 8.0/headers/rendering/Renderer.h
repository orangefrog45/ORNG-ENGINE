#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include <gui/ControlWindow.h>
#include "ExtraMath.h"
#include "ShaderLibrary.h"
#include "BasicMesh.h"
#include "GridMesh.h"
#include "Skybox.h"
#include "Scene.h"
#include "Camera.h"
#include "FramebufferLibrary.h"
#include "Quad.h"




class Renderer {
public:
	explicit Renderer(std::shared_ptr<Camera> cam) : p_camera(cam) {};
	void Init();
	void RenderScene();
	void DrawGrid();
	void DrawLightingEntities();
	void DrawLightMeshVisuals();
	void DrawReflectShaderEntities();
	void DrawToQuad();
	void DrawScene();
	void DrawShadowMap();
	LightConfigValues& ActivateLightingControls(LightConfigValues& light_vals);
	template <typename T> void DrawMeshWithShader(BasicMesh* mesh_data, unsigned int t_instances, T& shader);
	Scene scene;

private:
	unsigned int depth_map_fbo;
	unsigned int depth_map;
	unsigned int m_display_quad_vao;
	float FOV = 60.0f;
	float zNear = 0.01f;
	float zFar = 1000.0f;
	int m_window_width = RenderData::WINDOW_WIDTH;
	int m_window_height = RenderData::WINDOW_HEIGHT;

	glm::fmat4x4 projectionMatrix = glm::rowMajor4(glm::perspective(45.0f, static_cast<float>(m_window_width) / static_cast<float>(m_window_height), zNear, zFar));
	std::shared_ptr<Camera> p_camera = nullptr;
	Skybox skybox;
	GridMesh grid_mesh;
	ShaderLibrary shaderLibrary;
	FramebufferLibrary framebuffer_library;
	Quad render_quad;
};