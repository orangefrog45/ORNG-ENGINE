#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_major_storage.hpp>
#include <gui/ControlWindow.h>
#include "ShaderLibrary.h"
#include "MeshData.h"
#include "GridMesh.h"
#include "Skybox.h"
#include "Scene.h"
#include "Camera.h"
#include "FramebufferLibrary.h"
#include "Quad.h"
#include "RendererData.h"
#include "Terrain.h"




class Renderer {
public:
	explicit Renderer(std::shared_ptr<Camera> cam) : p_camera(cam) {};
	void Init();
	void RenderWindow();
	void DrawGrid();
	void RenderLightingEntities();
	void RenderLightMeshVisuals();
	void RenderReflectShaderEntities();
	void DrawTerrain(const Terrain& terrain);
	void DrawToQuad();
	void DrawScene();
	template <typename T> void DrawLightingGroups(T& shader);
	template <typename T> void DrawReflectGroups(T& shader);
	void DrawShadowMap();
	LightConfigValues& ActivateLightingControls(LightConfigValues& light_vals);
	template <typename T> void DrawMeshWithShader(MeshData* mesh_data, unsigned int t_instances, T& shader) const;
	Scene scene;

private:
	unsigned int depth_map_fbo;
	unsigned int depth_map;
	unsigned int m_display_quad_vao;
	float FOV = 60.0f;
	float zNear = 0.01f;
	float zFar = 1000.0f;
	int m_window_width = RendererData::WINDOW_WIDTH;
	int m_window_height = RendererData::WINDOW_HEIGHT;
	GLfloat* pixels = (GLfloat*)malloc(1024 * 1024 * sizeof(GL_FLOAT));

	Texture missing_texture = Texture(GL_TEXTURE_2D, "./res/textures/missing_texture.jpeg");
	glm::fmat4x4 projectionMatrix = glm::rowMajor4(glm::perspective(45.0f, static_cast<float>(m_window_width) / static_cast<float>(m_window_height), zNear, zFar));
	std::shared_ptr<Camera> p_camera = nullptr;
	Skybox skybox;
	GridMesh grid_mesh;
	ShaderLibrary shaderLibrary;
	FramebufferLibrary framebuffer_library;
	Quad render_quad;
};