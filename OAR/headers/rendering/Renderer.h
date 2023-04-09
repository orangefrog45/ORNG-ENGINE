#pragma once
#include "RendererResources.h"
#include "FramebufferLibrary.h"
#include "ShaderLibrary.h"
#include "Scene.h"

class Terrain;
class Quad;
class Camera;
class Skybox;
class MeshData;


class Renderer {
public:
	friend class Application;
	friend class EditorLayer;
	Renderer() = default;
	void Init();

	/* Render methods setup appropiate shaders, textures then issue draw calls */
	void RenderLightingEntities(Scene& scene);
	void RenderLightMeshVisuals(Scene& scene);
	void RenderReflectShaderEntities(Scene& scene);
	void RenderSkybox(Skybox& skybox);
	void RenderGrid(Scene& scene);
	void RenderScene(Scene& scene, Quad& quad);

private:
	FramebufferLibrary m_framebuffer_library;
	ShaderLibrary m_shader_library;
	void DrawTerrain(const Terrain& terrain);
	void DrawToQuad(Scene& scene);
	void DrawShadowMap(Scene& scene);
	void DrawQuad(Quad& quad);

	template <typename T> void DrawLightingGroups(Scene& scene, T& shader);
	template <typename T> void DrawReflectGroups(Scene& scene, T& shader);
	template <typename T> void DrawMeshWithShader(Scene& scene, MeshData* mesh_data, unsigned int t_instances, T& shader);

	Camera* m_active_camera = nullptr;
	RendererResources m_resources;
};