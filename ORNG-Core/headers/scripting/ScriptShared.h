#pragma once

#ifndef SCRIPT_SHARED_H
#define SCRIPT_SHARED_H
#include "entt/EnttSingleInclude.h"

class Instancer;
namespace physx {
	class PxGeometry;
}

namespace ORNG {
	class PhysicsComponent;
	class SceneEntity;
	class CameraComponent;
	

	class ScriptBase {
		friend class ScriptComponent;
		friend class EditorLayer;
		friend class ::Instancer;
	public:
		virtual ~ScriptBase() {};

		virtual void OnUpdate() {};
		virtual void OnCreate() {};
		virtual void OnDestroy() {};
		virtual void OnCollide([[maybe_unused]] SceneEntity* p_hit) {};
		virtual void OnTriggerEnter([[maybe_unused]] SceneEntity* p_entered) {};
		virtual void OnTriggerLeave([[maybe_unused]] SceneEntity* p_left) {};

		template<typename T>
		T& Get(const std::string& name) {
			if (m_member_addresses.contains(name))
				return *std::any_cast<T*>(m_member_addresses[name]);
			else
				throw std::runtime_error(std::format("ScriptBase::Get() error, tried to get non-existent member '{}'", name));
		}

	protected:
		// Used in pseudo-reflection system, PreParseScript in scriptingengine connects member variables with this
		std::unordered_map<std::string, std::any> m_member_addresses;
		SceneEntity* p_entity = nullptr;
	};


	struct RaycastResults {
		bool hit = false;
		glm::vec3 hit_pos{ 0, 0, 0 };
		glm::vec3 hit_normal{ 0, 0, 0 };
		float hit_dist = 0;
		PhysicsComponent* p_phys_comp = nullptr;
		SceneEntity* p_entity = nullptr;
	};

	struct OverlapQueryResults {
		std::vector<SceneEntity*> entities;
	};

	typedef ScriptBase* (__cdecl* InstanceCreator)();
	typedef void(__cdecl* InstanceDestroyer)(ScriptBase*);

	// ScriptInterface
	struct SI {
		std::function<SceneEntity& (const std::string&)> CreateEntity = nullptr;
		std::function<void(SceneEntity* p_entity)> DeleteEntity = nullptr;
		std::function<SceneEntity&(SceneEntity& p_entity)> DuplicateEntity = nullptr;
		std::function<SceneEntity& (entt::entity entity_handle)> GetEntityByEnttHandle = nullptr;
		std::function<SceneEntity& (uint64_t)> GetEntityByUUID = nullptr;
		std::function<SceneEntity& (const std::string&)> GetEntityByName = nullptr;
		std::function<SceneEntity& (uint64_t)> InstantiatePrefab = nullptr;

		std::function<float()> GetSceneTimeElapsed = nullptr;
		std::function<CameraComponent* ()> GetActiveCamera = nullptr;
		std::function<OverlapQueryResults(physx::PxGeometry&, glm::vec3, unsigned)> OverlapQuery = nullptr;
		std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)> Raycast = nullptr;
	};

	typedef void(__cdecl* InputSetter)(void*);
	typedef void(__cdecl* EventInstanceSetter)(void*);
	typedef void(__cdecl* FrameTimingSetter)(void*);
	typedef void(__cdecl* SI_Setter)(void*);

	struct ScriptSymbols {
		// Even if script fails to load, path must be preserved
		ScriptSymbols(const std::string& path) : script_path(path) {};

		bool loaded = false;
		std::string script_path;
		InstanceCreator CreateInstance = [] {return new ScriptBase(); };
		InstanceDestroyer DestroyInstance = [](ScriptBase* p_base) { delete p_base; };

		InputSetter _InputSetter;
		EventInstanceSetter _EventInstanceSetter;
		FrameTimingSetter _FrameTimingSetter;
		SI_Setter _SI_Setter;
	};

}

#endif