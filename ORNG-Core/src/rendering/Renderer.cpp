#include "pch/pch.h"

#include "rendering/Renderer.h"
#include "util/TimeStep.h"
#include "util/Log.h"
#include "rendering/Quad.h"
#include "core/GLStateManager.h"
#include "assets/AssetManager.h"

namespace ORNG {
	void Renderer::I_Init() {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		m_framebuffer_library.Init();
		m_shader_library.Init();
		mp_quad = std::make_unique<Quad>();
		mp_quad->Load();

		ORNG_CORE_INFO("Renderer initialized in {0}ms", time.GetTimeInterval());

		static Events::EventListener<Events::EngineCoreEvent> listener;
		listener.OnEvent = [](const Events::EngineCoreEvent& e_event) {
			ResetDrawCallCounter();
			};
		Events::EventManager::RegisterListener(listener);
	}



	void Renderer::IDrawUnitCube() const
	{
		Get().IDrawMeshInstanced(AssetManager::GetAsset<MeshAsset>(ORNG_BASE_CUBE_ID), 1);
	};

	void Renderer::DrawSphere() {
		Get().IDrawMeshInstanced(AssetManager::GetAsset<MeshAsset>(ORNG_BASE_SPHERE_ID), 1);
	}


	void Renderer::IDrawQuad() const
	{
		GL_StateManager::BindVAO(mp_quad->m_vao->GetHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void Renderer::IDrawScaledQuad(glm::vec2 min, glm::vec2 max) {
		glDisable(GL_CULL_FACE);
		mp_quad->SetVertices(min, max);
		GL_StateManager::BindVAO(mp_quad->m_vao->GetHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
		mp_quad->SetVertices({ -1.0, -1.0 }, { 1.0, 1.0 });
		glEnable(GL_CULL_FACE);
	}


	void Renderer::IDrawVAOArrays(const VAO& vao, unsigned int num_indices, GLenum primitive_type) {
		GL_StateManager::BindVAO(vao.GetHandle());

		glDrawArrays(primitive_type,
			0,
			num_indices
		);

		m_draw_call_amount++;
	}


	void Renderer::IDrawVAO_Elements(GLenum primitive_type, const MeshVAO& vao) {
		GL_StateManager::BindVAO(vao.GetHandle());

		glDrawElements(primitive_type,
			vao.vertex_data.indices.size(),
			GL_UNSIGNED_INT,
			nullptr);

		m_draw_call_amount++;
	};


	void Renderer::IDrawVAO_ArraysInstanced(GLenum primitive_type, const MeshVAO& vao, unsigned int instance_count) {
		GL_StateManager::BindVAO(vao.GetHandle());

		glDrawArraysInstanced(primitive_type,
			0,
			vao.vertex_data.positions.size() / 3,
			instance_count
		);

		m_draw_call_amount++;
	};


	void Renderer::DrawBoundingBox(const MeshAsset& asset) {
		GL_StateManager::BindVAO(asset.m_vao.GetHandle());

		glDrawArrays(GL_QUADS, 0, 24);
		Get().m_draw_call_amount++;
	}


	void Renderer::IDrawMeshInstanced(const MeshAsset* p_mesh, unsigned int instance_count) {
		GL_StateManager::BindVAO(p_mesh->m_vao.GetHandle());

		for (int i = 0; i < p_mesh->m_submeshes.size(); i++) {
			GL_StateManager::BindVAO(p_mesh->m_vao.GetHandle());

			glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
				p_mesh->m_submeshes[i].num_indices,
				GL_UNSIGNED_INT,
				(void*)(sizeof(unsigned int) * p_mesh->m_submeshes[i].base_index),
				instance_count,
				p_mesh->m_submeshes[i].base_vertex);

			m_draw_call_amount++;
		}
	}

	void Renderer::IDrawSubMesh(const MeshAsset* data, unsigned int submesh_index) {
		GL_StateManager::BindVAO(data->m_vao.GetHandle());


		glDrawElementsBaseVertex(GL_TRIANGLES,
			data->m_submeshes[submesh_index].num_indices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * data->m_submeshes[submesh_index].base_index),
			data->m_submeshes[submesh_index].base_vertex);

		m_draw_call_amount++;
	}

	void Renderer::IDrawSubMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, unsigned int submesh_index, GLenum primitive_type) {
		GL_StateManager::BindVAO(mesh_data->m_vao.GetHandle());

		glDrawElementsInstancedBaseVertex(primitive_type,
			mesh_data->m_submeshes[submesh_index].num_indices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * mesh_data->m_submeshes[submesh_index].base_index),
			t_instances,
			mesh_data->m_submeshes[submesh_index].base_vertex);

		m_draw_call_amount++;
	}
}