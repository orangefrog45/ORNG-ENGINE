#pragma once
#include "framebuffers/FramebufferLibrary.h"
#include "shaders/ShaderLibrary.h"
#include "scene/Scene.h"
#include "rendering/Quad.h"

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

		static void Init(Renderer* p_instance = nullptr) {
			if (p_instance) {
				ASSERT(!mp_instance);
				mp_instance = p_instance;
			}
			else {
				mp_instance = new Renderer();
				Get().I_Init();
			}
		}

		static void Shutdown() {
			if (mp_instance) delete mp_instance;
		}

		inline static Renderer& Get() {
			DEBUG_ASSERT(mp_instance);
			return *mp_instance;
		}

		static void DrawQuad() {
			Get().IDrawQuad();
		}

		static void DrawScaledQuad(glm::vec2 min, glm::vec2 max) {
			Get().IDrawScaledQuad(min, max);
		}

		static void DrawCube() {
			Get().IDrawUnitCube();
		}

		static void DrawSphere();

		static void DrawVAO_Elements(GLenum primitive_type, const MeshVAO& vao) {
			Get().IDrawVAO_Elements(primitive_type, vao);
		}

		static void DrawMeshInstanced(const MeshAsset* p_mesh, int instance_count) {
			Get().IDrawMeshInstanced(p_mesh, instance_count);
		}

		static void DrawVAO_ArraysInstanced(GLenum primitive_type, const MeshVAO& vao, int instance_count) {
			Get().IDrawVAO_ArraysInstanced(primitive_type, vao, instance_count);
		}

		inline static void DrawSubMesh(const MeshAsset* data, int submesh_index) {
			Get().IDrawSubMesh(data, submesh_index);
		}

		inline static void DrawSubMeshInstanced(const MeshAsset* mesh_data, int t_instances, int submesh_index, GLenum primitive_type) {
			Get().IDrawSubMeshInstanced(mesh_data, t_instances, submesh_index, primitive_type);
		}

		static void DrawVAOArrays(const VAO& vao, int num_indices, GLenum primitive_type) {
			Get().IDrawVAOArrays(vao, num_indices, primitive_type);
		}

		static unsigned GetDrawCalls() {
			return Get().m_draw_call_amount;
		}


		inline static ShaderLibrary& GetShaderLibrary() {
			return Get().m_shader_library;
		}

		static void ResetDrawCallCounter() {
			Get().m_draw_call_amount = 0;
		}

		static void DrawBoundingBox(const MeshAsset& asset);

	private:
		inline static Renderer* mp_instance = nullptr;

		void I_Init();

		void IDrawScaledQuad(glm::vec2 min, glm::vec2 max);

		unsigned int m_draw_call_amount = 0;

		Renderer() = default;
		void IDrawVAOArrays(const VAO& vao, int indices_count, GLenum primitive_type);
		void IDrawVAO_Elements(GLenum primitive_type, const MeshVAO& vao);
		void IDrawVAO_ArraysInstanced(GLenum primitive_type, const MeshVAO& vao, int instance_count);
		void IDrawSubMesh(const MeshAsset* data, int submesh_index);
		void IDrawSubMeshInstanced(const MeshAsset* mesh_data, int t_instances, int submesh_index, GLenum primitive_type);
		void IDrawUnitCube() const;
		void IDrawQuad() const;
		void IDrawMeshInstanced(const MeshAsset* p_mesh, int instance_count);

		std::unique_ptr<Quad> mp_quad = nullptr;
		ShaderLibrary m_shader_library;
	};
}
