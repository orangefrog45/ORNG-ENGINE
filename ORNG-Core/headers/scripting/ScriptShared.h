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
	class CameraComponent;
	struct SI;

	class ScriptBase {
		friend class ScriptComponent;
		friend class EditorLayer;
		friend class Scene;
		friend class ::Instancer;
	public:
		virtual ~ScriptBase() {};

		virtual void OnUpdate(float dt) {};
		virtual void OnCreate() {};
		virtual void OnDestroy() {};
		virtual void OnCollide([[maybe_unused]] SceneEntity* p_hit) {};
		virtual void OnTriggerEnter([[maybe_unused]] SceneEntity* p_entered) {};
		virtual void OnTriggerLeave([[maybe_unused]] SceneEntity* p_left) {};
		virtual void OnImGuiRender() {};

		void* Get(const std::string& property_name) {
			return static_cast<void*>(reinterpret_cast<std::byte*>(this) + properties[property_name]);
		}

		// DO NOT CALL - used by engine
		virtual void SetSI(SI* _si) {
			si = _si;
		}

	protected:
		SceneEntity* p_entity = nullptr;

		// DO NOT MODIFY VALUE, ONLY CALL METHODS
		inline static SI* si = nullptr;

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

	// ScriptInterface
	struct SI {
		std::function<SceneEntity& (const std::string&)> CreateEntity = nullptr;
		std::function<void(SceneEntity* p_entity)> DeleteEntity = nullptr;
		std::function<void(SceneEntity* p_entity)> DeleteEntityAtEndOfFrame = nullptr;
		std::function<SceneEntity& (SceneEntity& p_entity)> DuplicateEntity = nullptr;
		std::function<SceneEntity* (entt::entity entity_handle)> GetEntityByEnttHandle = nullptr;
		std::function<SceneEntity* (uint64_t)> GetEntityByUUID = nullptr;
		std::function<SceneEntity* (const std::string&)> GetEntityByName = nullptr;
		std::function<SceneEntity& (uint64_t)> InstantiatePrefab = nullptr;

		std::function<float()> GetSceneTimeElapsed = nullptr;
		std::function<CameraComponent* ()> GetActiveCamera = nullptr;
		std::function<OverlapQueryResults(physx::PxGeometry&, glm::vec3, unsigned)> OverlapQuery = nullptr;
		std::function<ORNG::RaycastResults(glm::vec3 origin, glm::vec3 unit_dir, float max_distance)> Raycast = nullptr;

		std::function<glm::ivec2()> GetScreenDimensions = nullptr;
		std::function<class Material* (uint64_t uuid)> GetMaterialByUUID = nullptr;
		std::function<class Material* ()> CreateMaterial = nullptr;
		std::function<class Shader* (const std::string& name)> CreateShader = nullptr;
		std::function<void(const std::string& name)> DeleteShader = nullptr;
		std::function<void(const struct Renderpass&)> AttachRenderpass = nullptr;
		std::function<void(const std::string& renderpass_name)> DetachRenderpass = nullptr;

	};

	typedef void(__cdecl* InputSetter)(void*);
	typedef void(__cdecl* EventInstanceSetter)(void*);
	typedef void(__cdecl* FrameTimingSetter)(void*);
	typedef void(__cdecl* ImGuiContextSetter)(void*, void*, void*);

	struct ScriptSymbols {
		// Even if script fails to load, name must be preserved
		ScriptSymbols(const std::string& _script_name) : script_name(_script_name) {};

		bool loaded = false;

		std::string script_name;

		InstanceCreator CreateInstance = [] {return new ScriptBase(); };
		InstanceDestroyer DestroyInstance = [](ScriptBase* p_base) { delete p_base; };
	};

}

#endif