#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "rendering/renderpasses/DepthPass.h"
#include "shaders/Shader.h"
#include "rendering/Renderer.h"
#include "rendering/RenderGraph.h"
#include "util/Timers.h"
#include "components/systems/PointlightSystem.h"
#include "components/systems/SpotlightSystem.h"
#include "scene/MeshInstanceGroup.h"
#include "components/systems/MeshInstancingSystem.h"
#include "rendering/MeshAsset.h"
#include "rendering/SceneRenderer.h"

using namespace ORNG;

enum class DepthSV {
	DIRECTIONAL,
	SPOTLIGHT,
	POINTLIGHT
};

void DepthPass::Init()
{
	sv.SetPath(GL_VERTEX_SHADER, "res/core-res/shaders/DepthVS.glsl");
	sv.SetPath(GL_FRAGMENT_SHADER, "res/core-res/shaders/DepthFS.glsl");
	sv.AddVariant(static_cast<unsigned>(DepthSV::DIRECTIONAL), { "ORTHOGRAPHIC" }, { "u_alpha_test", "u_light_pv_matrix" });
	sv.AddVariant(static_cast<unsigned>(DepthSV::SPOTLIGHT), { "PERSPECTIVE", "SPOTLIGHT" }, { "u_alpha_test", "u_light_pv_matrix", "u_light_pos" });
	sv.AddVariant(static_cast<unsigned>(DepthSV::POINTLIGHT), { "PERSPECTIVE", "POINTLIGHT" }, { "u_alpha_test", "u_light_pv_matrix", "u_light_pos", "u_light_zfar" });

	p_scene = mp_graph->GetData<Scene>("Scene");
	p_spotlight_system = &p_scene->GetSystem<SpotlightSystem>();
	p_pointlight_system = &p_scene->GetSystem<PointlightSystem>();

	fb.Init();

	Texture2DArraySpec depth_spec;
	depth_spec.format = GL_DEPTH_COMPONENT;
	depth_spec.internal_format = GL_DEPTH_COMPONENT16;
	depth_spec.storage_type = GL_FLOAT;
	depth_spec.min_filter = GL_NEAREST;
	depth_spec.mag_filter = GL_NEAREST;
	depth_spec.wrap_params = GL_CLAMP_TO_EDGE;
	depth_spec.layer_count = 3;
	depth_spec.width = DirectionalLight::SHADOW_RESOLUTION;
	depth_spec.height = DirectionalLight::SHADOW_RESOLUTION;

	directional_light_depth_tex.SetSpec(depth_spec);
}

void DepthPass::DoPass()
{
	ORNG_PROFILE_FUNC_GPU();

	if (p_scene->directional_light.shadows_enabled) {
		// Render cascades
		fb.Bind();
		sv.Activate(static_cast<unsigned>(DepthSV::DIRECTIONAL));
		for (unsigned i = 0; i < 3; i++) {
			glViewport(0, 0, DirectionalLight::SHADOW_RESOLUTION, DirectionalLight::SHADOW_RESOLUTION);
			fb.BindTextureLayerToFBAttachment(directional_light_depth_tex.GetTextureHandle(), GL_DEPTH_ATTACHMENT, i);
			GL_StateManager::ClearDepthBits();

			sv.SetUniform("u_light_pv_matrix", p_scene->directional_light.GetLightSpaceMatrix(i));
			DrawAllMeshesDepth(SOLID);
		}
	}

	// Spotlights
	glViewport(0, 0, SpotlightSystem::SPOTLIGHT_SHADOW_MAP_RES, SpotlightSystem::SPOTLIGHT_SHADOW_MAP_RES);
	sv.Activate(static_cast<unsigned>(DepthSV::SPOTLIGHT));
	auto spotlights = p_scene->GetRegistry().view<SpotLightComponent, TransformComponent>();

	unsigned index = 0;
	for (auto [entity, light, transform] : spotlights.each()) {
		if (!light.shadows_enabled)
			continue;

		fb.BindTextureLayerToFBAttachment(p_spotlight_system->GetDepthTex().GetTextureHandle(), GL_DEPTH_ATTACHMENT, index++);
		GL_StateManager::ClearDepthBits();

		sv.SetUniform("u_light_pv_matrix", light.GetLightSpaceTransform());
		sv.SetUniform("u_light_pos", transform.GetAbsPosition());
		DrawAllMeshesDepth(SOLID);
	}

	// Pointlights
	index = 0;
	glViewport(0, 0, PointlightSystem::POINTLIGHT_SHADOW_MAP_RES, PointlightSystem::POINTLIGHT_SHADOW_MAP_RES);
	sv.Activate(static_cast<unsigned>(DepthSV::POINTLIGHT));
	auto pointlights = p_scene->GetRegistry().view<PointLightComponent, TransformComponent>();

	for (auto [entity, pointlight, transform] : pointlights.each()) {
		if (!pointlight.shadows_enabled)
			continue;
		glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, pointlight.shadow_distance);
		glm::vec3 light_pos = transform.GetAbsPosition();

		std::array<glm::mat4, 6> capture_views =
		{
		   glm::lookAt(light_pos, light_pos + glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(light_pos, light_pos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(light_pos, light_pos + glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		sv.SetUniform("u_light_pos", light_pos);
		sv.SetUniform("u_light_zfar", pointlight.shadow_distance);

		// Draw depth cubemap
		for (unsigned i = 0; i < 6; i++) {
			fb.BindTextureLayerToFBAttachment(p_pointlight_system->GetDepthTex().GetTextureHandle(), GL_DEPTH_ATTACHMENT, index * 6 + i);
			GL_StateManager::ClearDepthBits();

			sv.SetUniform("u_light_pv_matrix", capture_projection * capture_views[i]);
			DrawAllMeshesDepth(SOLID);
		}

		index++;
	}
}

void DepthPass::DrawAllMeshesDepth(RenderGroup render_group) {
	for (const auto* group : p_scene->GetSystem<MeshInstancingSystem>().GetInstanceGroups()) {
		GL_StateManager::BindSSBO(group->GetTransformSSBO().GetHandle(), GL_StateManager::SSBO_BindingPoints::TRANSFORMS);

		auto* p_group_mesh = group->GetMeshAsset();
		const auto& submeshes = p_group_mesh->GetSubmeshes();

		for (unsigned int i = 0; i < submeshes.size(); i++) {
			const Material* p_material = group->GetMaterialIDs()[submeshes[i].material_index];

			if (p_material->render_group != render_group)
				continue;

			if (p_material->base_colour_texture && p_material->base_colour_texture->GetSpec().format == GL_RGBA) {
				sv.SetUniform("u_alpha_test", true);
				GL_StateManager::BindTexture(GL_TEXTURE_2D, p_material->base_colour_texture->GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
			}
			else {
				sv.SetUniform("u_alpha_test", false);
			}

			bool state_changed = SceneRenderer::SetGL_StateFromMatFlags(p_material->GetFlags());

			Renderer::DrawSubMeshInstanced(group->GetMeshAsset(), static_cast<int>(group->GetRenderCount()), static_cast<int>(i), GL_TRIANGLES);

			if (state_changed)
				SceneRenderer::UndoGL_StateModificationsFromMatFlags(p_material->GetFlags());
		}
	}
}

