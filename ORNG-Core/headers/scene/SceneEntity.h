#ifndef SCENE_ENTITY_H
#define SCENE_ENTITY_H

#include "Scene.h"
#include "util/UUID.h"
#include "util/util.h"
#include "components/Component.h"
#include "components/TransformComponent.h"

namespace ORNG {

	class SceneEntity {
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

			mp_registry->erase<T>(m_entt_handle);
		}

		// Do not delete children or rearrange them in the scene graph in the callback function
		void ForEachChildRecursive(std::function<void(entt::entity)> func_ptr);

		// Do not delete children or rearrange them in the scene graph in the callback function
		void ForEachLevelOneChild(std::function<void(entt::entity)> func_ptr);

		entt::entity GetParent() {
			return GetComponent<RelationshipComponent>()->parent;
		}

		SceneEntity* GetChild(const std::string& _name) {
			auto& rel_comp = mp_registry->get<RelationshipComponent>(m_entt_handle);
			entt::entity current_entity = rel_comp.first;

			for (int i = 0; i < rel_comp.num_children; i++) {
				auto* p_ent = mp_registry->get<TransformComponent>(current_entity).GetEntity();

				if (p_ent->name == _name)
					return p_ent;

				current_entity = mp_registry->get<RelationshipComponent>(current_entity).next;
			}

			return nullptr;
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
		SceneEntity& Duplicate();

		uint64_t GetUUID() const { return static_cast<uint64_t>(m_uuid); };

		void SetUUID(uint64_t new_uuid);

		entt::entity GetEnttHandle() const { return m_entt_handle; };

		std::string name = "Entity";
	private:
		UUID<uint64_t> m_uuid;

		void ForEachChildRecursiveInternal(std::function<void(entt::entity)> func_ptr, entt::entity search_entity);

		entt::entity m_entt_handle;

		Scene* mp_scene = nullptr;
		entt::registry* mp_registry = nullptr;
		uint64_t m_scene_uuid; // Stored seperately for faster access
};


}

#endif