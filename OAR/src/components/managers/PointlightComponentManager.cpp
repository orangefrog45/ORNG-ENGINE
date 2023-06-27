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
		if (m_pointlight_components.size() == 0)
			return;

		constexpr unsigned int point_light_fs_num_float = 12; // amount of floats in pointlight struct in shaders
		std::vector<float> light_array;

		light_array.resize(m_pointlight_components.size() * point_light_fs_num_float);

		for (int i = 0; i < m_pointlight_components.size() * point_light_fs_num_float; i) {

			auto& light = m_pointlight_components[i / point_light_fs_num_float];

			// - START COLOR
			auto color = light->color;
			light_array[i++] = color.x;
			light_array[i++] = color.y;
			light_array[i++] = color.z;
			light_array[i++] = 0; //padding
			// - END COLOR +- START POS
			auto pos = light->p_transform->GetAbsoluteTransforms()[0];
			light_array[i++] = pos.x;
			light_array[i++] = pos.y;
			light_array[i++] = pos.z;
			light_array[i++] = 0; //padding
			// - END COLOR - START MAX_DISTANCE
			light_array[i++] = light->max_distance;
			// - END MAX_DISTANCE - START ATTENUATION
			auto& atten = light->attenuation;
			light_array[i++] = atten.constant;
			light_array[i++] = atten.linear;
			light_array[i++] = atten.exp;
			// - END ATTENUATION
		}

		GL_StateManager::BindBuffer(GL_SHADER_STORAGE_BUFFER, m_pointlight_ssbo_handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * light_array.size(), &light_array[0], GL_STREAM_DRAW);
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
		delete* it;

		m_pointlight_components.erase(it);
	}



	void PointlightComponentManager::OnUnload() {
		for (auto* p_light : m_pointlight_components) {
			DeleteComponent(p_light->GetEntity());
		}

		glDeleteBuffers(1, &m_pointlight_ssbo_handle);
	}




	PointLightComponent* PointlightComponentManager::AddComponent(SceneEntity* p_entity) {

		if (GetComponent(p_entity->GetID())) {
			OAR_CORE_WARN("Pointlight component not added, entity '{0}' already has a pointlight component", p_entity->name);
			return nullptr;
		}

		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		PointLightComponent* comp = new PointLightComponent(p_entity, p_transform);
		m_pointlight_components.push_back(comp);
		return comp;
	}
}