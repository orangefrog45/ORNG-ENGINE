#ifndef SCENE_ENTITY_H
#define SCENE_ENTITY_H

#define __cpp_lib_concepts
#include <concepts>

#include "Scene.h"
#include "util/UUID.h"
#include "util/util.h"
#include "scripting/SceneScriptInterface.h"

/* Using the horrible template type restriction method instead of std::derived_from to support intellisense in scripts, doesn't work with concepts */
namespace ORNG {

	class SceneEntity {
		friend class EditorLayer;
		friend class Scene;
	public:
		SceneEntity() = delete;
		SceneEntity(Scene* scene, entt::entity entt_handle) : mp_scene(scene), m_entt_handle(entt_handle), m_scene_uuid(scene->uuid()), mp_registry(&scene->m_registry) { AddComponent<TransformComponent>(); AddComponent<RelationshipComponent>(); };
		SceneEntity(uint64_t t_id, entt::entity entt_handle, Scene* scene) : m_uuid(t_id), m_entt_handle(entt_handle), mp_scene(scene), m_scene_uuid(scene->uuid()), mp_registry(&scene->m_registry) { AddComponent<TransformComponent>(); AddComponent<RelationshipComponent>(); };


		~SceneEntity() {
			RemoveParent();
			mp_registry->destroy(m_entt_handle);
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>, typename... Args>
		T* AddComponent(Args&&... args) {
			if (HasComponent<T>()) return GetComponent<T>(); return &mp_registry->emplace<T>(m_entt_handle, this, std::forward<Args>(args)...);
		};

		// Returns ptr to component or nullptr if no component was found
		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
		T* GetComponent() {
			return mp_registry->try_get<T>(m_entt_handle);
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
		bool HasComponent() {
			return mp_registry->all_of<T>(m_entt_handle);
		}

		// Deletes component if found, else does nothing.
		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
		void DeleteComponent() {
			if (!HasComponent<T>())
				return;

			mp_registry->remove<T>(m_entt_handle);
		}

		SceneEntity* GetParent() {
			return mp_scene->GetEntity(GetComponent<RelationshipComponent>()->parent);
		}

		Scene* GetScene() {
			return mp_scene;
		}

		void SetParent(SceneEntity& parent_entity);
		void RemoveParent();

		// Returns a new duplicate entity in the scene
		SceneEntity& Duplicate() {
#ifdef ORNG_SCRIPT_ENV
			return ScriptInterface::Scene::DuplicateEntity(*this);
#else
			return mp_scene->DuplicateEntity(*this);
#endif
		}


		uint64_t GetUUID() const { return static_cast<uint64_t>(m_uuid); };
		uint64_t GetSceneUUID() const { return m_scene_uuid; };
		entt::entity GetEnttHandle() const { return m_entt_handle; };

		std::string name = "Entity";
	private:
		entt::entity m_entt_handle;
		UUID m_uuid;
		Scene* mp_scene = nullptr;
		entt::registry* mp_registry = nullptr;
		uint64_t m_scene_uuid; // Stored seperately for faster access
	};


}

#endif