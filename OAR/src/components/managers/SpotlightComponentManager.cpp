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

		(*it)->transform.RemoveParentTransform();
		delete* it;
		m_spotlight_components.erase(it);
	}



	void SpotlightComponentManager::OnUnload() {
		for (auto* p_light : m_spotlight_components) {
			delete p_light;
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


		for (int i = 0; i < m_spotlight_components.size() * spot_light_fs_num_float; i += spot_light_fs_num_float) {

			auto& light = m_spotlight_components[i / spot_light_fs_num_float];
			//0 - START COLOR
			auto color = light->color;
			light_array[i] = color.x;
			light_array[i + 1] = color.y;
			light_array[i + 2] = color.z;
			light_array[i + 3] = 0; //padding
			//16 - END COLOR - START POS
			auto pos = light->transform.GetPosition();
			light_array[i + 4] = pos.x;
			light_array[i + 5] = pos.y;
			light_array[i + 6] = pos.z;
			light_array[i + 7] = 0; //padding
			//32 - END POS, START DIR
			auto dir = light->GetLightDirection();
			light_array[i + 8] = dir.x;
			light_array[i + 9] = dir.y;
			light_array[i + 10] = dir.z;
			light_array[i + 11] = 0; // padding
			//48 - END DIR, START LIGHT TRANSFORM MAT
			glm::mat4 mat = light->GetLightSpaceTransformMatrix();
			light_array[i + 12] = mat[0][0];
			light_array[i + 13] = mat[0][1];
			light_array[i + 14] = mat[0][2];
			light_array[i + 15] = mat[0][3];
			light_array[i + 16] = mat[1][0];
			light_array[i + 17] = mat[1][1];
			light_array[i + 18] = mat[1][2];
			light_array[i + 19] = mat[1][3];
			light_array[i + 20] = mat[2][0];
			light_array[i + 21] = mat[2][1];
			light_array[i + 22] = mat[2][2];
			light_array[i + 23] = mat[2][3];
			light_array[i + 24] = mat[3][0];
			light_array[i + 25] = mat[3][1];
			light_array[i + 26] = mat[3][2];
			light_array[i + 27] = mat[3][3];
			//112   - END LIGHT TRANSFORM MAT, START INTENSITY
			light_array[i + 28] = light->ambient_intensity;
			light_array[i + 29] = light->diffuse_intensity;
			//40 - END INTENSITY - START MAX_DISTANCE
			light_array[i + 30] = light->max_distance;
			//44 - END MAX_DISTANCE - START ATTENUATION
			auto& atten = light->attenuation;
			light_array[i + 31] = atten.constant;
			light_array[i + 32] = atten.linear;
			light_array[i + 33] = atten.exp;
			//52 - END ATTENUATION - START APERTURE
			light_array[i + 34] = light->GetAperture();
			light_array[i + 35] = 0; //padding
		}

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_spotlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_DYNAMIC_DRAW);
	}

	SpotLightComponent* SpotlightComponentManager::AddComponent(SceneEntity* p_entity) {

		if (GetComponent(p_entity->GetID())) {
			OAR_CORE_WARN("Pointlight component not added, entity '{0}' already has a pointlight component", p_entity->name);
			return nullptr;
		}

		SpotLightComponent* comp = new SpotLightComponent(p_entity);
		comp->transform.SetParentTransform(p_entity->GetComponent<TransformComponent>());
		m_spotlight_components.push_back(comp);
		return comp;
	}

}