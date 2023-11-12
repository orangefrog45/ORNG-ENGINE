#pragma once

namespace ORNG {
	class PhysicsComponent;
	class SceneEntity;

	class ScriptBase {
	public:
		virtual void OnUpdate() {};
		virtual void OnCreate() {};
		virtual void OnDestroy() {};
		virtual void OnCollide() {};
	};

	struct RaycastResults {
		bool hit = false;
		glm::vec3 hit_pos{0, 0, 0};
		glm::vec3 hit_normal{0, 0, 0};
		float hit_dist = 0;
		PhysicsComponent* p_phys_comp = nullptr;
		SceneEntity* p_entity = nullptr;
	};

}