#pragma once
#include "components/lights/PointLightComponent.h"

namespace ORNG {

	class SpotLightComponent : public PointLightComponent {
	public:
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class SpotlightSystem;
		explicit SpotLightComponent(SceneEntity* p_entity);
		void SetLightDirection(float i, float j, float k);
		void SetAperture(float angle) { m_aperture = cosf(glm::radians(angle)); }

		glm::mat4 GetLightSpaceTransform() { return m_light_transform_matrix; }
		auto GetLightDirection() const { return m_light_direction_vec; }
		auto GetAperture() const { return m_aperture; }
	private:
		glm::mat4 m_light_transform_matrix;
		glm::vec3 m_light_direction_vec = glm::vec3(0, 0, 1);
		float m_aperture = 0.9396f;
	};

}