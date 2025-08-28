#include "pch/pch.h"

#include "rendering/SceneRenderer.h"
#include "shaders/ShaderLibrary.h"
#include "rendering/Renderer.h"
#include "core/GLStateManager.h"
#include "rendering/Material.h"
#include "assets/AssetManager.h"
#include "scene/MeshInstanceGroup.h"

namespace ORNG {
	SceneRenderer::~SceneRenderer() {
	}

	void SceneRenderer::SetGBufferMaterial(ShaderVariants* p_shader, const Material* p_material) {
		if (p_material->base_colour_texture) {
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->base_colour_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		}
		else { // Replace with 1x1 white pixel texture
			GL_StateManager::BindTexture(GL_TEXTURE_2D, AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE))->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		}

		if (p_material->roughness_texture == nullptr) {
			p_shader->SetUniform("u_roughness_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_roughness_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->roughness_texture->GetTextureHandle(), GL_StateManager::TextureUnits::ROUGHNESS);
		}

		if (p_material->metallic_texture == nullptr) {
			p_shader->SetUniform("u_metallic_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_metallic_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->metallic_texture->GetTextureHandle(), GL_StateManager::TextureUnits::METALLIC);
		}

		if (p_material->ao_texture == nullptr) {
			p_shader->SetUniform("u_ao_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_ao_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->ao_texture->GetTextureHandle(), GL_StateManager::TextureUnits::AO);
		}

		if (p_material->normal_map_texture == nullptr) {
			p_shader->SetUniform("u_normal_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_normal_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->normal_map_texture->GetTextureHandle(), GL_StateManager::TextureUnits::NORMAL_MAP);
		}

		if (p_material->displacement_texture == nullptr) {
			p_shader->SetUniform("u_displacement_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_material.displacement_scale", p_material->displacement_scale);
			p_shader->SetUniform<unsigned int>("u_num_parallax_layers", p_material->parallax_layers);
			p_shader->SetUniform("u_displacement_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->displacement_texture->GetTextureHandle(), GL_StateManager::TextureUnits::DISPLACEMENT);
		}

		if (p_material->emissive_texture == nullptr) {
			p_shader->SetUniform("u_emissive_sampler_active", 0);
		}
		else {
			p_shader->SetUniform("u_emissive_sampler_active", 1);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->emissive_texture->GetTextureHandle(), GL_StateManager::TextureUnits::EMISSIVE);
		}

		p_shader->SetUniform("u_material.flags", static_cast<unsigned>(p_material->flags));
		p_shader->SetUniform("u_material.base_colour", p_material->base_colour);
		p_shader->SetUniform("u_material.roughness", p_material->roughness);
		p_shader->SetUniform("u_material.ao", p_material->ao);
		p_shader->SetUniform("u_material.metallic", p_material->metallic);
		p_shader->SetUniform("u_material.tile_scale", p_material->tile_scale);
		p_shader->SetUniform("u_material.emissive_strength", p_material->emissive_strength);

		p_shader->SetUniform("u_material.sprite_data.num_rows", p_material->spritesheet_data.num_rows);
		p_shader->SetUniform("u_material.sprite_data.num_cols", p_material->spritesheet_data.num_cols);
		p_shader->SetUniform("u_material.sprite_data.fps", p_material->spritesheet_data.fps);

		p_shader->SetUniform("u_material.alpha_cutoff", p_material->alpha_cutoff);
	}

	std::vector<std::string> SceneRenderer::GetGBufferUniforms() {
		return {
			"u_roughness_sampler_active",
			"u_metallic_sampler_active",
			"u_emissive_sampler_active",
			"u_normal_sampler_active",
			"u_ao_sampler_active",
			"u_displacement_sampler_active",
			"u_num_parallax_layers",
			"u_material.base_colour",
			"u_material.metallic",
			"u_material.roughness",
			"u_material.ao",
			"u_material.tile_scale",
			"u_material.emissive_strength",
			"u_material.flags",
			"u_material.displacement_scale",
			"u_shader_id",
			"u_material.sprite_data.num_rows",
			"u_material.sprite_data.num_cols",
			"u_material.sprite_data.fps",
			"u_material.alpha_cutoff"
		};
	}

	/*void SceneRenderer::RenderVehicles(ShaderVariants* p_shader, RenderGroup render_group, Scene* p_scene) {
		p_shader->SetUniform("u_bloom_threshold", p_scene->post_processing.bloom.threshold);

		for (auto [entity, vehicle] : p_scene->m_registry.view<VehicleComponent>().each()) {
			PxShape* shapes[5];
			vehicle.m_vehicle.mPhysXState.physxActor.rigidBody->getShapes(&shapes[0], 5);
			auto pose = vehicle.m_vehicle.mPhysXState.physxActor.rigidBody->getGlobalPose();

			glm::mat4 b_scale = ExtraMath::Init3DScaleTransform(vehicle.body_scale.x, vehicle.body_scale.y, vehicle.body_scale.z);
			for (unsigned int i = 0; i < vehicle.p_body_mesh->m_submeshes.size(); i++) {
				const Material* p_material = vehicle.m_body_materials[vehicle.p_body_mesh->m_submeshes[i].material_index];

				if (p_material->render_group != render_group)
					continue;
				p_shader->SetUniform("u_transform", PxTransformToGlmMat4(pose * shapes[0]->getLocalPose()) * b_scale);
				p_shader->SetUniform<unsigned int>("u_shader_id", (p_material->flags & ORNG_MatFlags_EMISSIVE) ? ShaderLibrary::INVALID_SHADER_ID : p_material->shader_id);

				SetGBufferMaterial(p_shader, p_material);

				Renderer::DrawSubMesh(vehicle.p_body_mesh, i);
			}

			glm::mat4 w_scale = ExtraMath::Init3DScaleTransform(vehicle.wheel_scale.x, vehicle.wheel_scale.y, vehicle.wheel_scale.z);
			for (unsigned int wheel = 0; wheel < 4; wheel++) {
				for (unsigned int i = 0; i < vehicle.p_wheel_mesh->m_submeshes.size(); i++) {
					const Material* p_material = vehicle.m_wheel_materials[vehicle.p_wheel_mesh->m_submeshes[i].material_index];

					if (p_material->render_group != render_group)
						continue;

					p_shader->SetUniform("u_transform", PxTransformToGlmMat4(pose * shapes[wheel + 1]->getLocalPose()) * w_scale);
					p_shader->SetUniform<unsigned int>("u_shader_id", (p_material->flags & ORNG_MatFlags_EMISSIVE) ? ShaderLibrary::INVALID_SHADER_ID : p_material->shader_id);

					SetGBufferMaterial(p_shader, p_material);

					Renderer::DrawSubMesh(vehicle.p_wheel_mesh, i);
				}
			}
		}
	}*/


	void SceneRenderer::DrawInstanceGroupGBuffer(ShaderVariants* p_shader, const MeshInstanceGroup* group, RenderGroup render_group, 
		MaterialFlags mat_flags, MaterialFlags mat_flags_excluded, bool allow_state_changes, GLenum primitive_type) {
		GL_StateManager::BindSSBO(group->m_transform_ssbo.GetHandle(), 0);

		DrawMeshGBuffer(p_shader, group->m_mesh_asset, render_group, group->GetRenderCount(), group->m_materials.data(), mat_flags, 
			mat_flags_excluded, allow_state_changes, primitive_type);
	}



	void SceneRenderer::DrawMeshGBuffer(ShaderVariants* p_shader, const MeshAsset* p_mesh, RenderGroup render_group, unsigned instances, 
		const Material* const* materials, MaterialFlags mat_flags, MaterialFlags mat_flags_excluded, bool allow_state_changes, GLenum primitive_type) {
		for (unsigned int i = 0; i < p_mesh->m_submeshes.size(); i++) {
			const Material* p_material = materials[p_mesh->m_submeshes[i].material_index];

			if (p_material->render_group != render_group || !(mat_flags & p_material->flags) || mat_flags_excluded & p_material->flags)
				continue;

			p_shader->SetUniform<unsigned int>("u_shader_id", (p_material->flags & ORNG_MatFlags_EMISSIVE) ? ShaderLibrary::INVALID_SHADER_ID : p_material->shader_id);

			SetGBufferMaterial(p_shader, p_material);

			bool state_changed = allow_state_changes ? SetGL_StateFromMatFlags(p_material->flags) : false;

			Renderer::DrawSubMeshInstanced(p_mesh, instances, i, primitive_type);

			if (state_changed)
				UndoGL_StateModificationsFromMatFlags(p_material->flags);
		}
	}

	



	//void SceneRenderer::DrawTerrain(CameraComponent* p_cam, Scene* p_scene) {
	//	std::vector<TerrainQuadtree*> node_array;

	//	p_scene->terrain.m_quadtree->QueryChunks(node_array, p_cam->GetEntity()->GetComponent<TransformComponent>()->GetAbsPosition(), p_scene->terrain.m_width);
	//	for (auto& node : node_array) {
	//		const TerrainChunk* chunk = node->GetChunk();
	//		if (chunk->m_bounding_box.IsOnFrustum(p_cam->view_frustum)) {
	//			Renderer::DrawVAO_Elements(GL_TRIANGLES, chunk->m_vao);
	//		}
	//	}
	//}


	void SceneRenderer::UndoGL_StateModificationsFromMatFlags(MaterialFlags flags) {
		if (flags & MaterialFlags::ORNG_MatFlags_DISABLE_BACKFACE_CULL)
			glEnable(GL_CULL_FACE);
	}

	bool SceneRenderer::SetGL_StateFromMatFlags(MaterialFlags flags) {
		bool ret = false;

		if (flags & MaterialFlags::ORNG_MatFlags_DISABLE_BACKFACE_CULL) {
			ret = true;
			glDisable(GL_CULL_FACE);
		}

		return ret;
	}
}
