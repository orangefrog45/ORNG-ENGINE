#pragma once
#include "framebuffers/FramebufferLibrary.h"
#include "shaders/ShaderLibrary.h"
#include "rendering/Scene.h"


class Terrain;
class Quad;
class Camera;
class Skybox;
class MeshAsset;


class Renderer {
public:
	friend class Application;
	friend class EditorLayer;
	static void Init() { Get().I_Init(); };

	static Renderer& Get() {
		static Renderer s_instance;
		return s_instance;
	}

	/* Draw meshes in scene that have shader "shader_name" active */
	inline static void DrawGroupsWithShader(const char* shader_name) {
		Get().IDrawGroupsWithShader(shader_name);
	}

	/* Draw meshes that have default lighting shader active */
	inline static void DrawGroupsWithBaseShader() {
		Get().IDrawGroupsWithBaseShader();
	}

	inline static void DrawTerrain() {
		Get().IDrawTerrain();
	}


	inline static Scene* GetScene() {
		return Get().mp_active_scene;
	}

	/* Draw all mesh components in scene, does not bind shaders */
	inline static void DrawAllMeshesInstanced() {
		Get().IDrawAllMeshesInstanced();
	}

	static void DrawQuad(Quad& quad);

	inline static void DrawMesh(const MeshAsset* data, bool bind_materials) {
		Get().IDrawMesh(data, bind_materials);
	}

	inline static void DrawMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, bool bind_materials) {
		Get().IDrawMeshInstanced(mesh_data, t_instances, bind_materials);
	}

	inline static ShaderLibrary& GetShaderLibrary() {
		return Get().m_shader_library;
	}

	inline static FramebufferLibrary& GetFramebufferLibrary() {
		return Get().m_framebuffer_library;
	}

	inline static void SetActiveScene(Scene& scene) {
		Get().mp_active_scene = &scene;
	}

	static void DrawBoundingBox(const MeshAsset& asset);

	inline static void SetActiveCamera(Camera* camera) {
		Get().m_active_camera = camera;
	}

	inline static void DrawSkybox() {
		Get().IDrawSkybox();
	}

	inline static void RenderGrid() {
		Get().IRenderGrid();
	}

	inline static Camera* GetActiveCamera() {
		return Get().m_active_camera;
	}

	static void BindTexture(int target, int texture, int tex_unit);

	inline static int GetWindowWidth() { return Get().WINDOW_WIDTH; };
	inline static int GetWindowHeight() { return Get().WINDOW_HEIGHT; };

	static const unsigned int max_point_lights = 8;
	static const unsigned int max_spot_lights = 8;

	struct UniformBindingPoints { // these layouts are set manually in shaders, remember to change those with these
		static const unsigned int PVMATRICES = 0;
		static const unsigned int POINT_LIGHTS = 1;
		static const unsigned int SPOT_LIGHTS = 2;
	};

	struct TextureUnits {
		static const unsigned int COLOR = GL_TEXTURE1;
		static const unsigned int SPECULAR = GL_TEXTURE2;

		static const unsigned int DIR_SHADOW_MAP = GL_TEXTURE3;
		static const unsigned int SPOT_SHADOW_MAP = GL_TEXTURE4;
		static const unsigned int POINT_SHADOW_MAP = GL_TEXTURE5;
		static const unsigned int WORLD_POSITIONS = GL_TEXTURE6;
		static const unsigned int NORMAL_MAP = GL_TEXTURE7;
		static const unsigned int DIFFUSE_ARRAY = GL_TEXTURE8;
		static const unsigned int DISPLACEMENT = GL_TEXTURE9;
		static const unsigned int NORMAL_ARRAY = GL_TEXTURE10;

	};

	struct TextureUnitIndexes {
		static const unsigned int COLOR = 1;
		static const unsigned int SPECULAR = 2;
		static const unsigned int DIR_SHADOW_MAP = 3;
		static const unsigned int SPOT_SHADOW_MAP = 4;
		static const unsigned int POINT_SHADOW_MAP = 5;
		static const unsigned int WORLD_POSITIONS = 6;
		static const unsigned int NORMAL_MAP = 7;
		static const unsigned int DIFFUSE_ARRAY = 8;
		static const unsigned int DISPLACEMENT = 9;
		static const unsigned int NORMAL_ARRAY = 10;
	};
private:
	void I_Init();

	int WINDOW_WIDTH = 1920;
	int WINDOW_HEIGHT = 1080;

	unsigned int m_draw_call_amount = 0;

	Renderer() = default;
	void IDrawTerrain();
	void DrawShadowMap();

	void IDrawGroupsWithBaseShader();
	void IDrawGroupsWithShader(const char* shader_name);
	void IDrawAllMeshesInstanced();
	void IDrawMesh(const MeshAsset* data, bool bind_materials);
	void IDrawMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, bool bind_materials);

	void RenderReflectShaderEntities();
	void IDrawSkybox();
	void IRenderGrid();

	unsigned int m_current_2d_texture_binding = 0;
	unsigned int m_current_2d_array_texture_binding = 0;

	Scene* mp_active_scene = nullptr;
	Camera* m_active_camera = nullptr;
	FramebufferLibrary m_framebuffer_library;
	ShaderLibrary m_shader_library;
};