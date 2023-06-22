#include "pch/pch.h"
#include "components/ComponentManagers.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {


	void PointlightComponentManager::OnLoad() {
		m_pointlight_ssbo_handle = GL_StateManager::GenBuffer();
		GL_StateManager::BindSSBO(m_pointlight_ssbo_handle, GL_StateManager::SSBO_BindingPoints::POINT_LIGHTS);
	}




	void PointlightComponentManager::OnUpdate() {
		//if (!m_light_buffer_update_flag)
			//return;

		if (m_pointlight_components.size() == 0)
			return;

		m_light_buffer_update_flag = false;
		constexpr unsigned int point_light_fs_num_float = 16; // amount of floats in pointlight struct in shaders
		std::vector<float> light_array;

		light_array.resize(m_pointlight_components.size() * point_light_fs_num_float);

		for (int i = 0; i < m_pointlight_components.size() * point_light_fs_num_float; i += point_light_fs_num_float) {

			auto& light = m_pointlight_components[i / point_light_fs_num_float];

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
			//32 - END POS, START INTENSITY
			light_array[i + 8] = light->ambient_intensity;
			light_array[i + 9] = light->diffuse_intensity;
			//40 - END INTENSITY - START MAX_DISTANCE
			light_array[i + 10] = light->max_distance;
			//44 - END MAX_DISTANCE - START ATTENUATION
			auto& atten = light->attenuation;
			light_array[i + 11] = atten.constant;
			light_array[i + 12] = atten.linear;
			light_array[i + 13] = atten.exp;
			//56 - END ATTENUATION
			light_array[i + 14] = 0; //padding
			light_array[i + 15] = 0; //padding
			//64 BYTES TOTAL
		}

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_DYNAMIC_DRAW);
	}




	PointLightComponent* PointlightComponentManager::GetComponent(unsigned long entity_id) {
		auto it = std::find_if(m_pointlight_components.begin(), m_pointlight_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
		return it == m_pointlight_components.end() ? nullptr : *it;
	}




	void PointlightComponentManager::DeleteComponent(SceneEntity* p_entity) {
		auto it = std::find_if(m_pointlight_components.begin(), m_pointlight_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); });

		if (it == m_pointlight_components.end()) {
			OAR_CORE_ERROR("No pointlight component found in entity '{0}', not deleted.", p_entity->name);
			return;
		}
		(*it)->transform.RemoveParentTransform();
		delete* it;

		m_pointlight_components.erase(it);
	}



	void PointlightComponentManager::OnUnload() {
		for (auto* p_light : m_pointlight_components) {
			delete p_light;
		}

		glDeleteBuffers(1, &m_pointlight_ssbo_handle);
	}




	PointLightComponent* PointlightComponentManager::AddComponent(SceneEntity* p_entity) {

		if (GetComponent(p_entity->GetID())) {
			OAR_CORE_WARN("Pointlight component not added, entity '{0}' already has a pointlight component", p_entity->name);
			return nullptr;
		}

		PointLightComponent* comp = new PointLightComponent(p_entity);
		comp->transform.SetParentTransform(p_entity->GetComponent<TransformComponent>());
		m_pointlight_components.push_back(comp);
		return comp;
	}
}