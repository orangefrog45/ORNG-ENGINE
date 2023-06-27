#include "pch/pch.h"
#include "components/ComponentManagers.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
#include "core/GLStateManager.h"


namespace ORNG {


	SpotLightComponent* SpotlightComponentManager::GetComponent(unsigned long entity_id) {
		auto it = std::find_if(m_spotlight_components.begin(), m_spotlight_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
		return it == m_spotlight_components.end() ? nullptr : *it;
	}




	void SpotlightComponentManager::DeleteComponent(SceneEntity* p_entity) {
		auto it = std::find_if(m_spotlight_components.begin(), m_spotlight_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); });

		if (it == m_spotlight_components.end()) {
			OAR_CORE_ERROR("No pointlight component found in entity '{0}', not deleted.", p_entity->name);
			return;
		}

		auto* p_comp = *it;
		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		p_transform->update_callbacks.erase(TransformComponent::CallbackType::SPOTLIGHT);

		delete* it;
		m_spotlight_components.erase(it);
	}



	void SpotlightComponentManager::OnUnload() {
		for (auto* p_light : m_spotlight_components) {
			DeleteComponent(p_light->GetEntity());
		}

		glDeleteBuffers(1, &m_spotlight_ssbo_handle);
	}


	void SpotlightComponentManager::OnLoad() {
		m_spotlight_ssbo_handle = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_spotlight_ssbo_handle, GL_StateManager::SSBO_BindingPoints::SPOT_LIGHTS);
	}


	void SpotlightComponentManager::OnUpdate() {


		if (m_spotlight_components.size() == 0)
			return;

		constexpr unsigned int spot_light_fs_num_float = 36; // amount of floats in spotlight struct in shaders
		std::vector<float> light_array;

		light_array.resize(m_spotlight_components.size() * spot_light_fs_num_float);


		for (int i = 0; i < m_spotlight_components.size() * spot_light_fs_num_float; i) {

			auto& light = m_spotlight_components[i / spot_light_fs_num_float];
			//0 - START COLOR
			auto color = light->color;
			light_array[i++] = color.x;
			light_array[i++] = color.y;
			light_array[i++] = color.z;
			light_array[i++] = 0; //padding
			//16 - END COLOR - START POS
			auto pos = light->p_transform->GetPosition();
			light_array[i++] = pos.x;
			light_array[i++] = pos.y;
			light_array[i++] = pos.z;
			light_array[i++] = 0; //padding
			//32 - END POS, START DIR
			glm::vec3 rotation = light->p_transform->GetAbsoluteTransforms()[2];
			auto dir = glm::normalize(glm::mat3(ExtraMath::Init3DRotateTransform(rotation.x, rotation.y, rotation.z)) * light->GetLightDirection());
			light_array[i++] = dir.x;
			light_array[i++] = dir.y;
			light_array[i++] = dir.z;
			light_array[i++] = 0; // padding
			//48 - END DIR, START LIGHT TRANSFORM MAT
			glm::mat4 mat = light->GetLightSpaceTransformMatrix();
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
			light_array[i++] = light->max_distance;
			//44 - END MA++TANCE - START ATTENUATION
			auto& atten = light->attenuation;
			light_array[i++] = atten.constant;
			light_array[i++] = atten.linear;
			light_array[i++] = atten.exp;
			//52 - END AT++TION - START APERTURE
			light_array[i++] = light->GetAperture();
			light_array[i++] = 0; //padding
			light_array[i++] = 0; //padding
			light_array[i++] = 0; //padding
		}

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_spotlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_STREAM_DRAW);
	}

	SpotLightComponent* SpotlightComponentManager::AddComponent(SceneEntity* p_entity) {

		if (GetComponent(p_entity->GetID())) {
			OAR_CORE_WARN("Pointlight component not added, entity '{0}' already has a pointlight component", p_entity->name);
			return nullptr;
		}

		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		SpotLightComponent* comp = new SpotLightComponent(p_entity, p_transform);
		p_transform->update_callbacks[TransformComponent::CallbackType::SPOTLIGHT] = ([comp] {
			comp->UpdateLightTransform();
			});

		m_spotlight_components.push_back(comp);
		return comp;
	}

}