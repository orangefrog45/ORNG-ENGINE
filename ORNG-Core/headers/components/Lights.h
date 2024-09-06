#ifndef LIGHTS_H
#define LIGHTS_H
#include "Component.h"

namespace ORNG {
	struct BaseLight : public Component {
		virtual ~BaseLight() = default;
		BaseLight(SceneEntity* p_entity) : Component(p_entity) {};

		glm::vec3 colour = glm::vec3(1.0f, 1.0f, 1.0f);
		bool shadows_enabled = false;
	};

	struct LightAttenuation {
		float constant = 1.0f;
		float linear = 0.05f;
		float exp = 0.01f;
	};


	struct PointLightComponent : public BaseLight {
		PointLightComponent(SceneEntity* p_entity) : BaseLight(p_entity) { };

		void SetShadowsEnabled(bool v);
		float shadow_distance = 48.0f;
		LightAttenuation attenuation;
	};


	class SpotLightComponent : public PointLightComponent {
	public:
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class SpotlightSystem;
		explicit SpotLightComponent(SceneEntity* p_entity);
		void SetLightDirection(float i, float j, float k);
		void SetAperture(float angle) { m_aperture = cosf(glm::radians(angle)); }

		glm::mat4 GetLightSpaceTransform() { return m_light_transform_matrix; }
		auto GetAperture() const { return m_aperture; }
	private:
		glm::mat4 m_light_transform_matrix;
		float m_aperture = 0.9396f;
	};


	class DirectionalLight : public BaseLight {
	public:
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class ShaderLibrary;
		DirectionalLight();
		auto GetLightDirection() const { return m_light_direction; };
		void SetLightDirection(const glm::vec3& dir) { m_light_direction = glm::normalize(dir); };

		std::array<float, 3> cascade_ranges = { 20.f, 75.f, 200.f };
		//Amount the CameraComponent is pulled back from the center of the depth map
		std::array<float, 3> z_mults = { 5.f, 5.f, 5.f };

		float blocker_search_size = 2.f;
		float light_size = 10.f;

	private:
		std::array<glm::mat4, 3> m_light_space_matrices = { glm::identity<glm::mat4>() };
		glm::vec3 m_light_direction = normalize(glm::vec3(0.0f, 0.5f, 0.5f));
	};
}

#endif