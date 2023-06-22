#pragma once
#include "components/lights/BaseLight.h"
#include "components/TransformComponent.h"

namespace ORNG {

	class DirectionalLight : public BaseLight {
	public:
		friend class EditorLayer;
		DirectionalLight();
		auto GetLightDirection() const { return m_light_direction; };
		void SetLightDirection(const glm::vec3& dir) { m_light_direction = glm::normalize(dir); };

		std::array<float, 3> cascade_ranges = { 100.f, 350.f, 1000.f };
		//Amount the CameraComponent is pulled back from the center of the depth map
		std::array<float, 3> z_mults = { 4.f, 2.f, 2.f };

	private:
		glm::vec3 m_light_direction = normalize(glm::vec3(0.0f, 0.5f, 0.5f));
	};

}

