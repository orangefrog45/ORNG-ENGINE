#include "pch/pch.h"

#include <GLErrorHandling.h>
#include "rendering/Renderer.h"
#include "TimeStep.h"
#include "util/Log.h"
#include "rendering/Quad.h"
#include "components/MeshComponent.h"
#include "rendering/MeshInstanceGroup.h"
#include "components/lights/SpotLightComponent.h"
#include "components/lights/PointLightComponent.h"
#include "components/Camera.h"
#include "components/lights/DirectionalLight.h"


void Renderer::I_Init() {
	TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

	m_framebuffer_library.Init();
	m_shader_library.Init();

	OAR_CORE_INFO("Renderer initialized in {0}ms", time.GetTimeInterval());
}

void Renderer::BindTexture(int target, int texture, int tex_unit) {
	/* Check if texture is already bound, return if it is */
	/*switch (target) {
	case GL_TEXTURE_2D:
		if (Get().m_current_2d_texture_binding == texture)
			return;

		Get().m_current_2d_texture_binding = texture;
		break;

	case GL_TEXTURE_2D_ARRAY:
		if (Get().m_current_2d_array_texture_binding == texture)
			return;

		Get().m_current_2d_array_texture_binding = texture;
		break;
	}*/

	GLCall(glActiveTexture(tex_unit));
	GLCall(glBindTexture(target, texture));
}


void Renderer::IDrawSkybox() {
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	glBindVertexArray(mp_active_scene->m_skybox.m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}

void Renderer::DrawQuad(Quad& quad) {
	GLCall(glBindVertexArray(quad.m_vao));
	GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
	GLCall(glBindVertexArray(0));
}

void Renderer::IDrawTerrain() {

	std::vector<TerrainQuadtree*> node_array;
	mp_active_scene->m_terrain.m_quadtree->QueryChunks(node_array, m_active_camera->GetPos(), mp_active_scene->m_terrain.m_width);
	for (auto& node : node_array) {
		if (node->m_chunk->m_terrain_data.bounding_box.IsOnFrustum(m_active_camera->m_view_frustum)) {
			glBindVertexArray(node->m_chunk->m_vao);

			GLCall(glDrawElements(GL_QUADS,
				node->m_chunk->m_terrain_data.indices.size(),
				GL_UNSIGNED_INT,
				nullptr));

			glBindVertexArray(0);
			m_draw_call_amount++;
		}
	}
}

/*void Renderer::DrawShadowMap() {

	Shader& depth_shader = m_shader_library.GetShader("depth");
	//BIND FOR DRAW
	depth_shader.ActivateProgram();

	//DIRECTIONAL LIGHT
	m_framebuffer_library.GetFramebuffer("dir_depth").Bind();
	glClear(GL_DEPTH_BUFFER_BIT);

	depth_shader.SetUniform("pv_matrix", mp_active_scene->GetDirectionalLight().GetTransformMatrix());
	//DrawLightingGroups(false);
	//DrawReflectGroups(false);
	//DrawTerrain(mp_active_scene.m_terrain);

	//SPOT LIGHTS
	Framebuffer& spotlight_fb = m_framebuffer_library.GetFramebuffer("spotlight_depth");
	spotlight_fb.Bind();
	auto& lights = mp_active_scene->m_spot_lights;

	for (unsigned int i = 0; i < lights.size(); i++) {
		if (lights[i] == nullptr) continue;

		spotlight_fb.BindTextureLayerToFBAttachment(spotlight_fb.GetTexture("depth").tex_ref, GL_DEPTH_ATTACHMENT, i); // index 0 = directional light depth map
		depth_shader.SetUniform("pv_matrix", lights[i]->GetLightSpaceTransformMatrix());

		//DrawLightingGroups(false);
		//DrawReflectGroups(false);
		//DrawTerrain(mp_active_scene.m_terrain);
	}

	m_framebuffer_library.UnbindAllFramebuffers();
}*/


void Renderer::RenderReflectShaderEntities() {
	Shader& reflection_shader = m_shader_library.GetShader("reflection");
	reflection_shader.ActivateProgram();
	reflection_shader.SetUniform("camera_pos", m_active_camera->GetPos());
	BindTexture(GL_TEXTURE_CUBE_MAP, mp_active_scene->m_skybox.GetCubeMapTexture().GetTextureRef(), TextureUnits::COLOR);
	IDrawGroupsWithShader("reflection");
}


void Renderer::IRenderGrid() {
	Shader* grid_shader = &m_shader_library.GetShader("grid");
	grid_shader->ActivateProgram();
	const std::string cam = "camera_pos";
	grid_shader->SetUniform(cam, glm::vec3(m_active_camera->GetPos()));
	//mp_active_scene->grid_mesh.CheckBoundary(m_active_camera->GetPos());
	//mp_active_scene->grid_mesh.Draw();
}

void Renderer::IDrawAllMeshesInstanced() {
	for (auto group : mp_active_scene->m_mesh_instance_groups) {
		const MeshAsset* mesh_data = group->GetMeshData();

		if (group->GetMeshData()->GetIsShared() == true) group->UpdateWorldMatBuffer();

		IDrawMeshInstanced(mesh_data, group->m_meshes.size(), true);
	}
}

void Renderer::IDrawGroupsWithBaseShader() {
	for (auto group : mp_active_scene->m_mesh_instance_groups) {
		if (group->GetMeshData()->GetLoadStatus() == true && (group->GetShaderType() == 0)) {
			const MeshAsset* mesh_data = group->GetMeshData();

			if (group->GetMeshData()->GetIsShared() == true) group->UpdateWorldMatBuffer();

			IDrawMeshInstanced(mesh_data, group->m_meshes.size(), true);
		}
	}
};

void Renderer::IDrawGroupsWithShader(const char* shader_name) {
	for (auto group : mp_active_scene->m_mesh_instance_groups) {
		if (group->GetMeshData()->GetLoadStatus() == true && (group->GetShaderType() == m_shader_library.GetShader(shader_name).m_shader_id)) {
			const MeshAsset* mesh_data = group->GetMeshData();

			if (group->GetMeshData()->GetIsShared() == true) group->UpdateWorldMatBuffer();

			IDrawMeshInstanced(mesh_data, group->m_meshes.size(), false);
		}
	}
};

void Renderer::DrawBoundingBox(const MeshAsset& asset) {
	GLCall(glBindVertexArray(asset.m_aabb_vao));
	GLCall(glDrawArrays(GL_QUADS, 0, 24));
	GLCall(glBindVertexArray(0));
	Get().m_draw_call_amount++;
}


void Renderer::IDrawMesh(const MeshAsset* data, bool bind_materials) {
	GLCall(glBindVertexArray(data->m_VAO));

	for (unsigned int i = 0; i < data->m_meshes.size(); i++) {

		unsigned int materialIndex = data->m_meshes[i].materialIndex;

		if (bind_materials) {
			m_shader_library.lighting_shader.SetMaterial(data->m_materials[materialIndex]);
		}

		GLCall(glDrawElementsBaseVertex(GL_TRIANGLES,
			data->m_meshes[i].numIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * data->m_meshes[i].baseIndex),
			data->m_meshes[i].baseVertex));

		m_draw_call_amount++;
	}
	GLCall(glBindVertexArray(0));
}

void Renderer::IDrawMeshInstanced(const MeshAsset* mesh_data, unsigned int t_instances, bool bind_materials) {
	GLCall(glBindVertexArray(mesh_data->m_VAO));

	for (unsigned int i = 0; i < mesh_data->m_meshes.size(); i++) {

		unsigned int materialIndex = mesh_data->m_meshes[i].materialIndex;

		if (bind_materials) {
			m_shader_library.lighting_shader.SetMaterial(mesh_data->m_materials[materialIndex]);
		}

		GLCall(glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
			mesh_data->m_meshes[i].numIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * mesh_data->m_meshes[i].baseIndex),
			t_instances,
			mesh_data->m_meshes[i].baseVertex));

		m_draw_call_amount++;
	}
	GLCall(glBindVertexArray(0));
}
