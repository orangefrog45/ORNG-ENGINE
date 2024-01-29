#pragma once

#ifndef SCRIPT_SHARED_H
#define SCRIPT_SHARED_H
#include "entt/EnttSingleInclude.h"

class Instancer;

namespace ORNG {
	class PhysicsComponent;
	class SceneEntity;
	

	class ScriptBase {
		friend class ScriptComponent;
		friend class EditorLayer;
		friend class ::Instancer;
	public:
		virtual ~ScriptBase() {};

		virtual void OnUpdate() {};
		virtual void OnCreate() {};
		virtual void OnDestroy() {};
		virtual void OnCollide(SceneEntity* p_hit) {};
		virtual void OnTriggerEnter(SceneEntity* p_entered) {};
		virtual void OnTriggerLeave(SceneEntity* p_left) {};

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

	typedef void(__cdecl* ScriptFuncPtr)(SceneEntity*);
	typedef void(__cdecl* PhysicsEventCallback)(SceneEntity*, SceneEntity*);
	// Function ptr setters that enable the script to call some engine functions without dealing with certain unwanted libs/includes
	typedef void(__cdecl* InputSetter)(void*);
	typedef void(__cdecl* EventInstanceSetter)(void*);
	typedef void(__cdecl* FrameTimingSetter)(void*);
	typedef void(__cdecl* CreateEntitySetter)(std::function<SceneEntity& (const std::string&)>);
	typedef void(__cdecl* DeleteEntitySetter)(std::function<void(SceneEntity* p_entity)>);
	typedef void(__cdecl* GetEntitySetter)(std::function<SceneEntity& (uint64_t entity_uuid)>);
	typedef void(__cdecl* GetEntityEnttHandleSetter)(std::function<SceneEntity& (entt::entity entity_uuid)>);
	typedef void(__cdecl* DuplicateEntitySetter)(std::function<SceneEntity& (SceneEntity& p_entity)>);
	typedef void(__cdecl* InstantiatePrefabSetter)(std::function<SceneEntity& (const std::string&)>);
	typedef void(__cdecl* RaycastSetter)(std::function<RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)>);
	typedef void(__cdecl* OverlapQuerySetter)(std::function<OverlapQueryResults(glm::vec3 pos, float range)>);
	typedef ScriptBase* (__cdecl* InstanceCreator)();
	typedef void(__cdecl* InstanceDestroyer)(ScriptBase*);




	struct ScriptSymbols {
		// Even if script fails to load, path must be preserved
		ScriptSymbols(const std::string& path) : script_path(path) {};

		bool loaded = false;
		std::string script_path;
		InstanceCreator CreateInstance = [] {return new ScriptBase(); };
		InstanceDestroyer DestroyInstance = [](ScriptBase* p_base) { delete p_base; };

		// These set the appropiate callback functions in the scripts DLL so they can modify the scene, the scene class will call these methods during Update
		// Would like to expose the entire scene class but due to the amount of dependencies I would need to include doing this for now
		CreateEntitySetter SceneEntityCreationSetter = [](std::function<SceneEntity& (const std::string&)>) {};
		DeleteEntitySetter SceneEntityDeletionSetter = [](std::function<void(SceneEntity* p_entity)>) {};
		DuplicateEntitySetter SceneEntityDuplicationSetter = [](std::function<SceneEntity& (SceneEntity& p_entity)>) {};
		InstantiatePrefabSetter ScenePrefabInstantSetter = [](std::function<SceneEntity& (const std::string&)>) {};
		RaycastSetter SceneRaycastSetter = [](std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)>) {};
		GetEntitySetter SceneGetEntitySetter = [](std::function<SceneEntity& (uint64_t)>) {};
		OverlapQuerySetter SceneOverlapQuerySetter = [](std::function<OverlapQueryResults (glm::vec3, float)>) {};
		GetEntityEnttHandleSetter SceneGetEntityEnttHandleSetter = [](std::function<SceneEntity& (entt::entity entity_uuid)>) {};
	};

}

#endif