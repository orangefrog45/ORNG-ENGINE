#pragma once

#ifndef SCRIPT_SHARED_H
#define SCRIPT_SHARED_H
class Instancer;

namespace ORNG {
	class PhysicsComponent;
	class SceneEntity;

	class ScriptBase {
		friend class ScriptComponent;
		friend class ::Instancer;
	public:
		virtual ~ScriptBase() {};

		virtual void OnUpdate() {};
		virtual void OnCreate() {};
		virtual void OnDestroy() {};
		virtual void OnCollide(SceneEntity* p_hit) {};

		template<typename T>
		T& Get(const std::string& name) {
			if (m_member_addresses.contains(name))
				return *std::any_cast<T*>(m_member_addresses[name]);
			else
				throw std::runtime_error(std::format("ScriptBase::Get() error, tried to get non-existent member '{}'", name));
		}

	protected:
		// Used in pseudo-reflection system, PreParseScript in scriptingengine connects member variables with this
		std::map<std::string, std::any> m_member_addresses;
		SceneEntity* p_entity;
	};

	struct RaycastResults {
		bool hit = false;
		glm::vec3 hit_pos{ 0, 0, 0 };
		glm::vec3 hit_normal{ 0, 0, 0 };
		float hit_dist = 0;
		PhysicsComponent* p_phys_comp = nullptr;
		SceneEntity* p_entity = nullptr;
	};
}

#endif