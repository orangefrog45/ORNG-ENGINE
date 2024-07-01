#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
#include "core/GLStateManager.h"


namespace ORNG {

	void SpotlightSystem::OnUnload() {
	}


	void SpotlightSystem::OnLoad() {
		if (!m_spotlight_ssbo.IsInitialized())
			m_spotlight_ssbo.Init();

		GL_StateManager::BindSSBO(m_spotlight_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::SPOT_LIGHTS);

		Texture2DArraySpec spotlight_depth_spec;
		spotlight_depth_spec.internal_format = GL_DEPTH_COMPONENT24;
		spotlight_depth_spec.width = 1024;
		spotlight_depth_spec.height = 1024;
		spotlight_depth_spec.layer_count = 8;
		spotlight_depth_spec.format = GL_DEPTH_COMPONENT;
		spotlight_depth_spec.storage_type = GL_FLOAT;
		spotlight_depth_spec.min_filter = GL_NEAREST;
		spotlight_depth_spec.mag_filter = GL_NEAREST;
		spotlight_depth_spec.wrap_params = GL_CLAMP_TO_EDGE;

		m_spotlight_depth_tex.SetSpec(spotlight_depth_spec);
	}

	static glm::mat4 CalculateLightSpaceTransform(SpotLightComponent& light) {
		float z_near = 0.01f;
		float z_far = light.shadow_distance;
		glm::mat4 light_perspective = glm::perspective(glm::degrees(acosf(light.GetAperture())), 1.0f, z_near, z_far);
		glm::vec3 pos = light.GetEntity()->GetComponent<TransformComponent>()->GetAbsPosition();
		glm::vec3 light_dir = light.GetEntity()->GetComponent<TransformComponent>()->forward;
		glm::mat4 spot_light_view = glm::lookAt(pos, pos + light_dir, glm::vec3(0.0f, 1.0f, 0.0f));

		return glm::mat4(light_perspective * spot_light_view);
	}

	void SpotlightSystem::WriteLightToVector(std::vector<float>& output_vec, SpotLightComponent& light, int& index) {
		auto color = light.color;
		output_vec[index++] = color.x;
		output_vec[index++] = color.y;
		output_vec[index++] = color.z;
		output_vec[index++] = 0; //padding
		//16 - END COLOR - START POS
		auto pos = std::get<0>(light.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms());
		output_vec[index++] = pos.x;
		output_vec[index++] = pos.y;
		output_vec[index++] = pos.z;
		output_vec[index++] = 0; //padding
		//32 - END POS, START DIR
		auto dir = light.GetEntity()->GetComponent<TransformComponent>()->forward;
		output_vec[index++] = dir.x;
		output_vec[index++] = dir.y;
		output_vec[index++] = dir.z;
		output_vec[index++] = 0; // padding
		//48 - END DIR, START LIGHT TRANSFORM MAT
		glm::mat4 mat = CalculateLightSpaceTransform(light);
		light.m_light_transform_matrix = mat;
		output_vec[index++] = mat[0][0];
		output_vec[index++] = mat[0][1];
		output_vec[index++] = mat[0][2];
		output_vec[index++] = mat[0][3];
		output_vec[index++] = mat[1][0];
		output_vec[index++] = mat[1][1];
		output_vec[index++] = mat[1][2];
		output_vec[index++] = mat[1][3];
		output_vec[index++] = mat[2][0];
		output_vec[index++] = mat[2][1];
		output_vec[index++] = mat[2][2];
		output_vec[index++] = mat[2][3];
		output_vec[index++] = mat[3][0];
		output_vec[index++] = mat[3][1];
		output_vec[index++] = mat[3][2];
		output_vec[index++] = mat[3][3];
		//40 - END INTENSITY - START MAX_DISTANCE
		output_vec[index++] = light.shadows_enabled ? light.shadow_distance : -1.f;
		//44 - END MA++TANCE - START ATTENUATION
		auto& atten = light.attenuation;
		output_vec[index++] = atten.constant;
		output_vec[index++] = atten.linear;
		output_vec[index++] = atten.exp;
		//52 - END AT++TION - START APERTURE
		output_vec[index++] = light.GetAperture();
		output_vec[index++] = 0; //padding
		output_vec[index++] = 0; //padding
		output_vec[index++] = 0; //padding
	}



	void SpotlightSystem::OnUpdate(entt::registry* p_registry) {

		auto view = p_registry->view<SpotLightComponent>();
		m_spotlight_ssbo.data.clear();

		if (view.empty()) {
			glNamedBufferData(m_spotlight_ssbo.GetHandle(), 0, nullptr, GL_STREAM_DRAW);
		}

		unsigned num_shadow = 0;
		unsigned num_shadowless = 0;

		for (auto [entity, light] : view.each()) {
			if (light.shadows_enabled)
				num_shadow++;
			else
				num_shadowless++;
		}

		auto& spec = m_spotlight_depth_tex.GetSpec();
		[[unlikely]] if (spec.layer_count < num_shadow) {
			auto spec_copy = m_spotlight_depth_tex.GetSpec();
			spec_copy.layer_count = num_shadow * 3 / 2;
			m_spotlight_depth_tex.SetSpec(spec_copy);
		}
		else [[unlikely]] if (spec.layer_count > 4 && spec.layer_count > num_shadow * 3) {
			auto spec_copy = m_spotlight_depth_tex.GetSpec();
			spec_copy.layer_count /= 2;
			m_spotlight_depth_tex.SetSpec(spec_copy);
		}

		constexpr unsigned int spot_light_fs_num_float = 36; // amount of floats in spotlight struct in shaders
		m_spotlight_ssbo.data.resize((num_shadow + num_shadowless) * spot_light_fs_num_float);

		int index = 0;
		for (auto [entity, light] : view.each()) {
			WriteLightToVector(m_spotlight_ssbo.data, light, index);
		}

		m_spotlight_ssbo.FillBuffer();

		GL_StateManager::BindSSBO(m_spotlight_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::SPOT_LIGHTS);
	}


}