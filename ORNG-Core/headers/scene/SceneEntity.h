#ifndef SCENE_ENTITY_H
#define SCENE_ENTITY_H

#ifndef ORNG_SCRIPT_ENV
#include "Scene.h"
#endif
#include "util/UUID.h"
#include "util/util.h"
#include "scripting/SceneScriptInterface.h"

/* Using the horrible template type restriction method instead of std::derived_from to support intellisense in scripts, doesn't work with concepts */
namespace ORNG {

	class SceneEntity {
		friend class EditorLayer;
		friend class Scene;
		friend class SceneSerializer;
	public:
		SceneEntity() = delete;
		SceneEntity(Scene* scene, entt::entity entt_handle, entt::registry* p_reg, uint64_t scene_uuid) : mp_scene(scene), m_entt_handle(entt_handle), mp_registry(p_reg), m_scene_uuid(scene_uuid) { AddComponent<TransformComponent>(); AddComponent<RelationshipComponent>(); };
		SceneEntity(uint64_t t_id, entt::entity entt_handle, Scene* scene, entt::registry* p_reg, uint64_t scene_uuid) : m_uuid(t_id), m_entt_handle(entt_handle), mp_scene(scene), m_scene_uuid(scene_uuid), mp_registry(p_reg) {
			AddComponent<TransformComponent>(); AddComponent<RelationshipComponent>();
		};


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
#ifdef ORNG_SCRIPT_ENV
			auto* p_comp = mp_registry->try_get<T>(m_entt_handle);
			if (!p_comp) {
				throw std::runtime_error("GetComponent call failed, entity does not have specified component. Validate with HasComponent first!");
			}
			else {
				return p_comp;
			}
#else
			return mp_registry->try_get<T>(m_entt_handle);
#endif
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

			mp_registry->erase<T>(m_entt_handle);
		}

		void ForEachChildRecursive(std::function<void(entt::entity)> func_ptr);

		void ForEachLevelOneChild(std::function<void(entt::entity)> func_ptr);

		entt::entity GetParent() {
			return GetComponent<RelationshipComponent>()->parent;
		}

		entt::registry* GetRegistry() {
			return mp_registry;
		}

		Scene* GetScene() {
			return mp_scene;
		}

		void SetParent(SceneEntity& parent_entity);
		void RemoveParent();


		// Returns a new duplicate entity in the scene
		SceneEntity& Duplicate() {
#ifdef ORNG_SCRIPT_ENV
			return ScriptInterface::World::DuplicateEntity(*this);
#else
			return mp_scene->DuplicateEntity(*this);
#endif
	}


		uint64_t GetUUID() const { return static_cast<uint64_t>(m_uuid); };
		//uint64_t GetSceneUUID() const { return m_scene_uuid; };
		entt::entity GetEnttHandle() const { return m_entt_handle; };

		std::string name = "Entity";
	private:

		void ForEachChildRecursiveInternal(std::function<void(entt::entity)> func_ptr, entt::entity search_entity);

		void RecursiveChildSearch();
		entt::entity m_entt_handle;
		UUID m_uuid;
		Scene* mp_scene = nullptr;
		entt::registry* mp_registry = nullptr;
		uint64_t m_scene_uuid; // Stored seperately for faster access
};


}

#endif