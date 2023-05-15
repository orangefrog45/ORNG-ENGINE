#pragma once
#include "framebuffers/FramebufferLibrary.h"
#include "shaders/ShaderLibrary.h"
#include "scene/Scene.h"

namespace ORNG {

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

		inline static void DrawTerrain() {
			Get().IDrawTerrain();
		}

		inline static Scene* GetScene() {
			return Get().mp_active_scene;
		}

		/* Draw all mesh components in scene, does not bind shaders */
		inline static void DrawAllMeshesInstanced(bool bind_materials) {
			Get().IDrawAllMeshesInstanced(bind_materials);
		}

		static void DrawQuad() {
			Get().IDrawQuad();
		};

		static void DrawCube() {
			Get().IDrawUnitCube();
		};

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

		static const unsigned int max_point_lights = 8;
		static const unsigned int max_spot_lights = 8;
		static const unsigned int max_materials = 128;


	private:
		void I_Init();

		unsigned int m_draw_call_amount = 0;

		Renderer() = default;

		void IDrawTerrain();
		void IDrawAllMeshesInstanced(bool bind_materials);
		void IDrawMesh(const MeshAsset* data, bool bind_materials);
		void IDrawMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, bool bind_materials);
		void IDrawUnitCube() const;
		void IDrawQuad() const;

		void IDrawSkybox();
		void IRenderGrid();

		MeshAsset* mp_unit_cube = nullptr;
		Quad* mp_quad = nullptr;
		Scene* mp_active_scene = nullptr;
		Camera* m_active_camera = nullptr;
		FramebufferLibrary m_framebuffer_library;
		ShaderLibrary m_shader_library;
	};
}