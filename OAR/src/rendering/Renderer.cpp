#include "pch/pch.h"

#include "rendering/Renderer.h"
#include "util/TimeStep.h"
#include "util/Log.h"
#include "rendering/Quad.h"
#include "components/MeshComponent.h"
#include "rendering/MeshInstanceGroup.h"
#include "components/lights/SpotLightComponent.h"
#include "components/lights/PointLightComponent.h"
#include "components/Camera.h"
#include "components/lights/DirectionalLight.h"
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
		mp_unit_cube->is_loaded = true;

		OAR_CORE_INFO("Renderer initialized in {0}ms", time.GetTimeInterval());
	}




	void Renderer::IDrawSkybox() {
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		DrawCube();

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);

	}

	void Renderer::IDrawUnitCube() const
	{
		DrawMesh(mp_unit_cube, false);
	};

	void Renderer::IDrawQuad() const
	{
		GL_StateManager::BindVAO(mp_quad->m_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void Renderer::IDrawTerrain() {

		std::vector<TerrainQuadtree*> node_array;
		mp_active_scene->m_terrain.m_quadtree->QueryChunks(node_array, m_active_camera->GetPos(), mp_active_scene->m_terrain.m_width);
		for (auto& node : node_array) {
			if (node->m_chunk->m_bounding_box.IsOnFrustum(m_active_camera->m_view_frustum)) {
				GL_StateManager::BindVAO(node->m_chunk->m_vao);

				glDrawElements(GL_QUADS,
					node->m_chunk->m_vao.vertex_data.indices.size(),
					GL_UNSIGNED_INT,
					nullptr);

				m_draw_call_amount++;
			}
		}
	}



	void Renderer::IRenderGrid() {
		Shader* grid_shader = &m_shader_library.GetShader("grid");
		grid_shader->ActivateProgram();
		const std::string cam = "camera_pos";
		grid_shader->SetUniform(cam, glm::vec3(m_active_camera->GetPos()));
		//mp_active_scene->grid_mesh.CheckBoundary(m_active_camera->GetPos());
		//mp_active_scene->grid_mesh.Draw();
	}

	void Renderer::IDrawAllMeshesInstanced(bool bind_materials) {
		for (const auto* group : mp_active_scene->m_mesh_instance_groups) {
			const MeshAsset* mesh_data = group->GetMeshData();

			GL_StateManager::BindSSBO(group->m_transform_ssbo_handle, GL_StateManager::SSBO_BindingPoints::TRANSFORMS);

			IDrawMeshInstanced(mesh_data, group->m_meshes.size(), false);

		}
	}

	void Renderer::DrawBoundingBox(const MeshAsset& asset) {
		GL_StateManager::BindVAO(asset.m_vao);

		glDrawArrays(GL_QUADS, 0, 24);
		Get().m_draw_call_amount++;
	}


	void Renderer::IDrawMesh(const MeshAsset* data, bool bind_materials) {
		GL_StateManager::BindVAO(data->m_vao);

		for (unsigned int i = 0; i < data->m_meshes.size(); i++) {

			if (bind_materials) {
				unsigned int material_index = data->m_meshes[i].material_index;
				m_shader_library.SetGBufferMaterial(data->m_materials[material_index]);
			}

			glDrawElementsBaseVertex(GL_TRIANGLES,
				data->m_meshes[i].num_indices,
				GL_UNSIGNED_INT,
				(void*)(sizeof(unsigned int) * data->m_meshes[i].base_index),
				data->m_meshes[i].base_vertex);

			m_draw_call_amount++;
		}
	}

	void Renderer::IDrawMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, bool bind_materials) {
		GL_StateManager::BindVAO(mesh_data->m_vao);

		for (unsigned int i = 0; i < mesh_data->m_meshes.size(); i++) {

			if (bind_materials) {
				unsigned int material_index = mesh_data->m_meshes[i].material_index;
				m_shader_library.SetGBufferMaterial(mesh_data->m_materials[material_index]);
			}

			glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
				mesh_data->m_meshes[i].num_indices,
				GL_UNSIGNED_INT,
				(void*)(sizeof(unsigned int) * mesh_data->m_meshes[i].base_index),
				t_instances,
				mesh_data->m_meshes[i].base_vertex);

			m_draw_call_amount++;
		}
	}
}