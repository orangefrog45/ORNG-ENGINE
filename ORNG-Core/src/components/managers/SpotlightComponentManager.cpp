#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
#include "core/GLStateManager.h"


namespace ORNG {

	void SpotlightSystem::OnUnload() {
		glDeleteBuffers(1, &m_spotlight_ssbo_handle);
	}


	void SpotlightSystem::OnLoad() {
		m_spotlight_ssbo_handle = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_spotlight_ssbo_handle, GL_StateManager::SSBO_BindingPoints::SPOT_LIGHTS);

		Texture2DArraySpec spotlight_depth_spec;
		spotlight_depth_spec.internal_format = GL_DEPTH_COMPONENT24;
		spotlight_depth_spec.width = 2048;
		spotlight_depth_spec.height = 2048;
		spotlight_depth_spec.layer_count = 8;
		spotlight_depth_spec.format = GL_DEPTH_COMPONENT;
		spotlight_depth_spec.storage_type = GL_FLOAT;
		spotlight_depth_spec.min_filter = GL_NEAREST;
		spotlight_depth_spec.mag_filter = GL_NEAREST;
		spotlight_depth_spec.wrap_params = GL_CLAMP_TO_EDGE;

		m_spotlight_depth_tex.SetSpec(spotlight_depth_spec);
	}

	static glm::mat4 CalculateLightSpaceTransform(SpotLightComponent& light) {
		auto transforms = light.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms();
		float z_near = 0.01f;
		float z_far = light.shadow_distance;
		glm::mat4 light_perspective = glm::perspective(glm::degrees(acosf(light.GetAperture())), 1.0f, z_near, z_far);
		glm::vec3 pos = transforms[0];
		glm::vec3 rotated_dir = glm::normalize(glm::mat3(ExtraMath::Init3DRotateTransform(transforms[2].x, transforms[2].y, transforms[2].z)) * glm::vec3(0, 0, 1));
		glm::mat4 spot_light_view = glm::lookAt(pos, pos + rotated_dir, glm::vec3(0.0f, 1.0f, 0.0f));

		return glm::mat4(light_perspective * spot_light_view);
	}

	void SpotlightSystem::OnUpdate(entt::registry* p_registry) {

		auto view = p_registry->view<SpotLightComponent>();

		if (view.empty())
			return;


		constexpr unsigned int spot_light_fs_num_float = 36; // amount of floats in spotlight struct in shaders
		std::vector<float> light_array;

		light_array.resize(view.size() * spot_light_fs_num_float);

		int i = 0;
		for (auto [entity, light] : view.each()) {
			//0 - START COLOR
			auto color = light.color;
			light_array[i++] = color.x;
			light_array[i++] = color.y;
			light_array[i++] = color.z;
			light_array[i++] = 0; //padding
			//16 - END COLOR - START POS
			auto pos = light.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
			light_array[i++] = pos.x;
			light_array[i++] = pos.y;
			light_array[i++] = pos.z;
			light_array[i++] = 0; //padding
			//32 - END POS, START DIR
			auto dir = light.GetLightDirection();
			light_array[i++] = dir.x;
			light_array[i++] = dir.y;
			light_array[i++] = dir.z;
			light_array[i++] = 0; // padding
			//48 - END DIR, START LIGHT TRANSFORM MAT
			glm::mat4 mat = CalculateLightSpaceTransform(light);
			light.m_light_transform_matrix = mat;
			light_array[i++] = mat[0][0];
			light_array[i++] = mat[0][1];
			light_array[i++] = mat[0][2];
			light_array[i++] = mat[0][3];
			light_array[i++] = mat[1][0];
			light_array[i++] = mat[1][1];
			light_array[i++] = mat[1][2];
			light_array[i++] = mat[1][3];
			light_array[i++] = mat[2][0];
			light_array[i++] = mat[2][1];
			light_array[i++] = mat[2][2];
			light_array[i++] = mat[2][3];
			light_array[i++] = mat[3][0];
			light_array[i++] = mat[3][1];
			light_array[i++] = mat[3][2];
			light_array[i++] = mat[3][3];
			//40 - END INTENSITY - START MAX_DISTANCE
			light_array[i++] = light.shadow_distance;
			//44 - END MA++TANCE - START ATTENUATION
			auto& atten = light.attenuation;
			light_array[i++] = atten.constant;
			light_array[i++] = atten.linear;
			light_array[i++] = atten.exp;
			//52 - END AT++TION - START APERTURE
			light_array[i++] = light.GetAperture();
			light_array[i++] = 0; //padding
			light_array[i++] = 0; //padding
			light_array[i++] = 0; //padding
		}


		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_spotlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_STREAM_DRAW);
	}

	/*	SpotLightComponent* SpotlightSystem::AddComponent(SceneEntity* p_entity) {

			if (GetComponent(p_entity->GetID())) {
				ORNG_CORE_WARN("Pointlight component not added, entity '{0}' already has a pointlight component", p_entity->name);
				return nullptr;
			}

			auto* p_transform = p_entity->GetComponent<TransformComponent>();
			SpotLightComponent* comp = new SpotLightComponent(p_entity, p_transform);
			p_transform->update_callbacks[TransformComponent::CallbackType::SPOTLIGHT] = ([comp](TransformComponent::UpdateType type) {
				comp->UpdateLightTransform();
				});

			m_spotlight_components.push_back(comp);
			return comp;
		}*/

}