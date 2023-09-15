#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {


	void PointlightSystem::OnLoad() {
		m_shadow_pointlight_ssbo_handle = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_shadow_pointlight_ssbo_handle, GL_StateManager::SSBO_BindingPoints::POINT_LIGHTS_SHADOW);

		m_shadowless_pointlight_ssbo_handle = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_shadowless_pointlight_ssbo_handle, GL_StateManager::SSBO_BindingPoints::POINT_LIGHTS_SHADOWLESS);

		TextureCubemapArraySpec pointlight_depth_spec;
		pointlight_depth_spec.format = GL_DEPTH_COMPONENT;
		pointlight_depth_spec.internal_format = GL_DEPTH_COMPONENT16;
		pointlight_depth_spec.storage_type = GL_FLOAT;
		pointlight_depth_spec.min_filter = GL_LINEAR;
		pointlight_depth_spec.mag_filter = GL_LINEAR;
		pointlight_depth_spec.wrap_params = GL_CLAMP_TO_EDGE;
		pointlight_depth_spec.layer_count = 8;
		pointlight_depth_spec.width = 512;
		pointlight_depth_spec.height = 512;
		m_pointlight_depth_tex.SetSpec(pointlight_depth_spec);
	}



	void PointlightSystem::WriteLightToVector(std::vector<float>& output_vec, PointLightComponent& light, int& index) {
		// - START COLOR
		auto color = light.color;
		output_vec[index++] = color.x;
		output_vec[index++] = color.y;
		output_vec[index++] = color.z;
		output_vec[index++] = 0; //padding
		// - END COLOR +- START POS
		auto pos = light.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		output_vec[index++] = pos.x;
		output_vec[index++] = pos.y;
		output_vec[index++] = pos.z;
		output_vec[index++] = 0; //padding
		// - END COLOR - START MAX_DISTANCE
		output_vec[index++] = light.shadow_distance;
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
			GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_shadowless_pointlight_ssbo_handle);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STREAM_DRAW);
			GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_shadow_pointlight_ssbo_handle);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STREAM_DRAW);
			return;
		}

		constexpr unsigned int point_light_fs_num_float = 12; // amount of floats in pointlight struct in shaders
		std::vector<float> light_array_shadowless;
		std::vector<float> light_array_shadows;
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

		light_array_shadowless.resize(num_shadowless * point_light_fs_num_float);
		light_array_shadows.resize(num_shadow * point_light_fs_num_float);

		int i = 0;
		int s = 0;
		for (auto [entity, light] : view.each()) {
			if (light.shadows_enabled)
				WriteLightToVector(light_array_shadows, light, s);
			else
				WriteLightToVector(light_array_shadowless, light, i);
		}



		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_shadowless_pointlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array_shadowless.size(), &light_array_shadowless[0], GL_STREAM_DRAW);
		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_shadow_pointlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array_shadows.size(), &light_array_shadows[0], GL_STREAM_DRAW);
	}



	void PointlightSystem::OnUnload() {
		glDeleteBuffers(1, &m_shadow_pointlight_ssbo_handle);
		glDeleteBuffers(1, &m_shadowless_pointlight_ssbo_handle);

	}




}