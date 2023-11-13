#pragma once
#include "framebuffers/FramebufferLibrary.h"
#include "shaders/ShaderLibrary.h"
#include "scene/Scene.h"


namespace ORNG {
	class Quad;
	class Skybox;
	class MeshAsset;

	struct Line {
		glm::vec3 p1;
		glm::vec3 p2;
		glm::vec3 col;
	};

	struct Point {
		glm::vec3 p;
	};

	class Renderer {
	public:
		friend class Application;
		friend class EditorLayer;
		static void Init() { Get().I_Init(); };

		static Renderer& Get() {
			static Renderer s_instance;
			return s_instance;
		}


		static void DrawQuad() {
			Get().IDrawQuad();
		};

		static void DrawCube() {
			Get().IDrawUnitCube();
		};

		static void DrawSphere();

		static void DrawVAO_Elements(GLenum primitive_type, const MeshVAO& vao) {
			Get().IDrawVAO_Elements(primitive_type, vao);
		}

		static void DrawMeshInstanced(const MeshAsset* p_mesh, unsigned int instance_count) {
			Get().IDrawMeshInstanced(p_mesh, instance_count);
		}

		static void DrawVAO_ArraysInstanced(GLenum primitive_type, const MeshVAO& vao, unsigned int instance_count) {
			Get().IDrawVAO_ArraysInstanced(primitive_type, vao, instance_count);
		}

		inline static void DrawSubMesh(const MeshAsset* data, unsigned int submesh_index) {
			Get().IDrawSubMesh(data, submesh_index);
		}

		inline static void DrawSubMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, unsigned int submesh_index) {
			Get().IDrawSubMeshInstanced(mesh_data, t_instances, submesh_index);
		}

		static void DrawVAOArrays(const VAO& vao, unsigned int num_indices, GLenum primitive_type) {
			Get().IDrawVAOArrays(vao, num_indices, primitive_type);
		}

		static unsigned GetDrawCalls() {
			return Get().m_draw_call_amount;
		}


		inline static ShaderLibrary& GetShaderLibrary() {
			return Get().m_shader_library;
		}

		inline static FramebufferLibrary& GetFramebufferLibrary() {
			return Get().m_framebuffer_library;
		}

		static void ResetDrawCallCounter() {
			Get().m_draw_call_amount = 0;
		}

		static void DrawBoundingBox(const MeshAsset& asset);

	private:
		void I_Init();

		unsigned int m_draw_call_amount = 0;

		Renderer() = default;
		void IDrawVAOArrays(const VAO& vao, unsigned int indices_count, GLenum primitive_type);
		void IDrawVAO_Elements(GLenum primitive_type, const MeshVAO& vao);
		void IDrawVAO_ArraysInstanced(GLenum primitive_type, const MeshVAO& vao, unsigned int instance_count);
		void IDrawSubMesh(const MeshAsset* data, unsigned int submesh_index);
		void IDrawSubMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, unsigned int submesh_index);
		void IDrawUnitCube() const;
		void IDrawQuad() const;
		void IDrawMeshInstanced(const MeshAsset* p_mesh, unsigned int instance_count);

		Quad* mp_quad = nullptr;
		FramebufferLibrary m_framebuffer_library;
		ShaderLibrary m_shader_library;
	};
}