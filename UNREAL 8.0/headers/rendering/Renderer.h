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
#include "FramebufferLibrary.h"
#include "Quad.h"
#include "Terrain.h"
#include "RendererResources.h"




class Renderer {
public:
	friend class Application;
	Renderer() = default;
	void Init();

	/* Render methods setup appropiate shaders, textures then issue draw calls */
	void RenderWindow();
	void RenderLightingEntities();
	void RenderLightMeshVisuals();
	void RenderReflectShaderEntities();
	void RenderSkybox(Skybox& skybox);
	void RenderGrid();

	/* Draw methods don't set shaders, only issue draw calls */
	void DrawTerrain(const Terrain& terrain);
	void DrawToQuad();
	void RenderScene();
	void DrawShadowMap();

	template <typename T> void DrawLightingGroups(T& shader);
	template <typename T> void DrawReflectGroups(T& shader);
	template <typename T> void DrawMeshWithShader(MeshData* mesh_data, unsigned int t_instances, T& shader) const;

	ControlWindow::LightConfigData& ActivateLightingControls(ControlWindow::LightConfigData& light_vals);
	Scene scene;

private:
	unsigned int m_display_quad_vao;

	Camera* m_active_camera = nullptr;
	GridMesh grid_mesh;
	ShaderLibrary shader_library;
	FramebufferLibrary framebuffer_library;
	Quad render_quad; // 2D quad that is rendered to for display
	RendererResources m_resources;
};