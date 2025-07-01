#pragma once

#ifndef SCRIPT_SHARED_H
#define SCRIPT_SHARED_H
#include "entt/EnttSingleInclude.h"
#include <iostream>

class Instancer;
namespace physx {
	class PxGeometry;
}

namespace ORNG {
	class PhysicsComponent;
	class SceneEntity;
	struct CameraComponent;
	struct SI;

	class ScriptBase {
		friend class ScriptComponent;
		friend class EditorLayer;
		friend class Scene;
		friend class ::Instancer;
	public:
		virtual ~ScriptBase() {};

		virtual void OnUpdate(float dt) {};
		virtual void OnRender() {};
		virtual void OnCreate() {};
		virtual void OnDestroy() {};
		virtual void OnCollide([[maybe_unused]] SceneEntity* p_hit) {};
		virtual void OnTriggerEnter([[maybe_unused]] SceneEntity* p_entered) {};
		virtual void OnTriggerLeave([[maybe_unused]] SceneEntity* p_left) {};
		virtual void OnImGuiRender() {};

		SceneEntity* GetEntity() {
			return p_entity;
		}

		void* Get(const std::string& property_name) {
			return static_cast<void*>(reinterpret_cast<std::byte*>(this) + properties[property_name]);
		}

	protected:
		SceneEntity* p_entity = nullptr;
		Scene* p_scene = nullptr;

		// DO NOT ACCESS
		inline static std::unordered_map<std::string, uint32_t> properties;
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
	typedef void(__cdecl* SingletonPtrSetter)(void*, void*, void*, void*, void*, void*, void*, void*);
	typedef void(__cdecl* ImGuiContextSetter)(void*, void*, void*);
	typedef void(__cdecl* UnloadFunc)();
	typedef uint64_t(__cdecl* ScriptGetUuidFunc)();

	struct ScriptSymbols {
		// Even if script fails to load, name must be preserved
		ScriptSymbols(const std::string& _script_name) : script_name(_script_name) {};

		bool loaded = false;
		uint64_t uuid = INVALID_SCRIPT_UUID;

		static constexpr uint64_t INVALID_SCRIPT_UUID = 0;

		std::string script_name;

		InstanceCreator CreateInstance = [] {return new ScriptBase(); };
		InstanceDestroyer DestroyInstance = [](ScriptBase* p_base) { delete p_base; };
		UnloadFunc Unload = nullptr;
	};

}

#endif