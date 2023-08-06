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

		std::array<float, 3> cascade_ranges = { 20.f, 75.f, 200.f };
		//Amount the CameraComponent is pulled back from the center of the depth map
		std::array<float, 3> z_mults = { 5.f, 5.f, 5.f };

		float blocker_search_size = 2.f;
		float light_size = 10.f;

	private:
		glm::vec3 m_light_direction = normalize(glm::vec3(0.0f, 0.5f, 0.5f));
	};

}

