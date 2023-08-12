#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {


	void PointlightSystem::OnLoad() {
		m_pointlight_ssbo_handle = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_pointlight_ssbo_handle, GL_StateManager::SSBO_BindingPoints::POINT_LIGHTS);

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




	void PointlightSystem::OnUpdate() {
		auto view = mp_registry->view<PointLightComponent>();
		if (view.size() == 0)
			return;

		constexpr unsigned int point_light_fs_num_float = 12; // amount of floats in pointlight struct in shaders
		std::vector<float> light_array;

		light_array.resize(view.size() * point_light_fs_num_float);

		int i = 0;
		for (auto [entity, light] : view.each()) {
			// - START COLOR
			auto color = light.color;
			light_array[i++] = color.x;
			light_array[i++] = color.y;
			light_array[i++] = color.z;
			light_array[i++] = 0; //padding
			// - END COLOR +- START POS
			auto pos = light.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
			light_array[i++] = pos.x;
			light_array[i++] = pos.y;
			light_array[i++] = pos.z;
			light_array[i++] = 0; //padding
			// - END COLOR - START MAX_DISTANCE
			light_array[i++] = light.shadow_distance;
			// - END MAX_DISTANCE - START ATTENUATION
			auto& atten = light.attenuation;
			light_array[i++] = atten.constant;
			light_array[i++] = atten.linear;
			light_array[i++] = atten.exp;
			// - END ATTENUATION
		}


		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_STREAM_DRAW);
	}



	void PointlightSystem::OnUnload() {
		glDeleteBuffers(1, &m_pointlight_ssbo_handle);
	}


	/*void PointlightSystem::OnDepthMapUpdate() {
		int num_shadow_casting_lights = 0;
		std::ranges::for_each(m_pointlight_components, [&](auto p_comp) {if (p_comp->shadows_enabled) num_shadow_casting_lights++; });

		if (auto spec = m_pointlight_depth_tex.GetSpec(); spec.layer_count != num_shadow_casting_lights) {
			spec.layer_count = num_shadow_casting_lights;
			m_pointlight_depth_tex.SetSpec(spec);
		}
	}*/

}