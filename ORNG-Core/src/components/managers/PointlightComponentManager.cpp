#include "pch/pch.h"
#include "rendering/SceneRenderer.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {


	void PointlightSystem::OnLoad() {
		if (!m_pointlight_ssbo.IsInitialized())
			m_pointlight_ssbo.Init();

		TextureCubemapArraySpec pointlight_depth_spec;
		pointlight_depth_spec.format = GL_DEPTH_COMPONENT;
		pointlight_depth_spec.internal_format = GL_DEPTH_COMPONENT16;
		pointlight_depth_spec.storage_type = GL_FLOAT;
		pointlight_depth_spec.min_filter = GL_LINEAR;
		pointlight_depth_spec.mag_filter = GL_LINEAR;
		pointlight_depth_spec.wrap_params = GL_CLAMP_TO_EDGE;
		pointlight_depth_spec.layer_count = 8;
		pointlight_depth_spec.width = POINTLIGHT_SHADOW_MAP_RES;
		pointlight_depth_spec.height = POINTLIGHT_SHADOW_MAP_RES;
		m_pointlight_depth_tex.SetSpec(pointlight_depth_spec);
	}



	void PointlightSystem::WriteLightToVector(std::vector<float>& output_vec, PointLightComponent& light, int& index) {
		// - START colour
		auto colour = light.colour;
		output_vec[index++] = colour.x;
		output_vec[index++] = colour.y;
		output_vec[index++] = colour.z;
		output_vec[index++] = 0; //padding
		// - END colour +- START POS
		auto pos = std::get<0>(light.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms());
		output_vec[index++] = pos.x;
		output_vec[index++] = pos.y;
		output_vec[index++] = pos.z;
		output_vec[index++] = 0; //padding
		// - END colour - START MAX_DISTANCE
		output_vec[index++] = light.shadows_enabled ?  light.shadow_distance : -1.f;
		// - END MAX_DISTANCE - START ATTENUATION
		auto& atten = light.attenuation;
		output_vec[index++] = atten.constant;
		output_vec[index++] = atten.linear;
		output_vec[index++] = atten.exp;
		// - END ATTENUATION
	}

	void PointlightSystem::OnUpdate(entt::registry* p_registry) {
		auto view = p_registry->view<PointLightComponent>();
		if (view.size() == 0) {
			glNamedBufferData(m_pointlight_ssbo.GetHandle(), 0, nullptr, GL_STREAM_DRAW);
			return;
		}
		m_pointlight_ssbo.data.clear();

		constexpr unsigned int point_light_fs_num_float = 12; // amount of floats in pointlight struct in shaders

		int num_shadowless = 0;
		int num_shadow = 0;
		for (auto [entity, light] : view.each()) {
			if (light.shadows_enabled)
				num_shadow++;
			else
				num_shadowless++;
		}

		auto& spec = m_pointlight_depth_tex.GetSpec();
		[[unlikely]] if (spec.layer_count < num_shadow) {
			auto spec_copy = m_pointlight_depth_tex.GetSpec();
			spec_copy.layer_count = num_shadow * 3 / 2;
			m_pointlight_depth_tex.SetSpec(spec_copy);
		}
		else [[unlikely]] if (spec.layer_count > 4 && spec.layer_count > num_shadow * 3) {
			auto spec_copy = m_pointlight_depth_tex.GetSpec();
			spec_copy.layer_count /= 2;
			m_pointlight_depth_tex.SetSpec(spec_copy);
		}

		m_pointlight_ssbo.data.resize((num_shadowless + num_shadow) * point_light_fs_num_float);

		int i = 0;
		for (auto [entity, light] : view.each()) {
			WriteLightToVector(m_pointlight_ssbo.data, light, i);
		}

		m_pointlight_ssbo.FillBuffer();

		GL_StateManager::BindSSBO(m_pointlight_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::POINT_LIGHTS);
	}



	void PointlightSystem::OnUnload() {
	}




}