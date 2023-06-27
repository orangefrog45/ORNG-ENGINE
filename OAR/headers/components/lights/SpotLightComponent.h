#pragma once
#include "components/lights/PointLightComponent.h"

namespace ORNG {

	class SpotLightComponent : public PointLightComponent {
	public:
		friend class EditorLayer;
		explicit SpotLightComponent(SceneEntity* p_entity, TransformComponent* p_transform);
		void SetLightDirection(float i, float j, float k);
		void SetAperture(float angle) { aperture = cosf(glm::radians(angle)); }

		auto GetLightDirection() const { return m_light_direction_vec; }
		auto GetAperture() const { return aperture; }
		const glm::mat4& GetLightSpaceTransformMatrix() const { return m_light_space_transform; }
		void UpdateLightTransform();
	private:
		glm::mat4 m_light_space_transform;
		glm::vec3 m_light_direction_vec = glm::vec3(1, 0, 0);
		float aperture = 0.9396f;
	};

}