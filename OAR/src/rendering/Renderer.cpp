#include "pch/pch.h"

#include "rendering/Renderer.h"
#include "util/TimeStep.h"
#include "util/Log.h"
#include "rendering/Quad.h"
#include "core/GLStateManager.h"

namespace ORNG {

	void Renderer::I_Init() {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		m_framebuffer_library.Init();
		m_shader_library.Init();

		mp_quad = new Quad();
		mp_quad->Load();

		mp_unit_cube = new MeshAsset{ "./res/meshes/cube/cube.obj" };
		mp_unit_cube->LoadMeshData();
		mp_unit_cube->PopulateBuffers();
		mp_unit_cube->importer.FreeScene();
		mp_unit_cube->m_is_loaded = true;

		OAR_CORE_INFO("Renderer initialized in {0}ms", time.GetTimeInterval());
	}



	void Renderer::IDrawUnitCube() const
	{
		for (int i = 0; i < mp_unit_cube->m_submeshes.size(); i++) {
			DrawSubMesh(mp_unit_cube, i);
		}
	};

	void Renderer::IDrawQuad() const
	{
		GL_StateManager::BindVAO(mp_quad->m_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}



	void Renderer::IDrawVAO_Elements(GLenum primitive_type, const VAO& vao) {
		GL_StateManager::BindVAO(vao);

		glDrawElements(primitive_type,
			vao.vertex_data.indices.size(),
			GL_UNSIGNED_INT,
			nullptr);

		m_draw_call_amount++;
	};


	void Renderer::IDrawVAO_ArraysInstanced(GLenum primitive_type, const VAO& vao, unsigned int instance_count) {
		GL_StateManager::BindVAO(vao);

		glDrawArraysInstanced(primitive_type,
			0,
			vao.vertex_data.positions.size(),
			instance_count
		);

		m_draw_call_amount++;
	};


	void Renderer::DrawBoundingBox(const MeshAsset& asset) {
		GL_StateManager::BindVAO(asset.m_vao);

		glDrawArrays(GL_QUADS, 0, 24);
		Get().m_draw_call_amount++;
	}



	void Renderer::IDrawSubMesh(const MeshAsset* data, unsigned int submesh_index) {
		GL_StateManager::BindVAO(data->m_vao);


		glDrawElementsBaseVertex(GL_TRIANGLES,
			data->m_submeshes[submesh_index].num_indices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * data->m_submeshes[submesh_index].base_index),
			data->m_submeshes[submesh_index].base_vertex);

		m_draw_call_amount++;
	}

	void Renderer::IDrawSubMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, unsigned int submesh_index) {
		GL_StateManager::BindVAO(mesh_data->m_vao);

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
			mesh_data->m_submeshes[submesh_index].num_indices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * mesh_data->m_submeshes[submesh_index].base_index),
			t_instances,
			mesh_data->m_submeshes[submesh_index].base_vertex);

		m_draw_call_amount++;
	}
}