#pragma once
#include "components/lights/BaseLight.h"
#include "components/WorldTransform.h"

namespace ORNG {

	class DirectionalLight : public BaseLight {
	public:
		friend class EditorLayer;
		DirectionalLight();
		auto GetLightDirection() const { return m_light_direction; };
		void SetLightDirection(const glm::vec3& dir) { m_light_direction = glm::normalize(dir); };
	private:
		glm::vec3 m_light_direction = glm::vec3(0.0f, 0.5f, 0.5f);
	};

}

