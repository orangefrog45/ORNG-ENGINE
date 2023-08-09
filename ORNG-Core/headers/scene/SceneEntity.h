#pragma once
#include "scene/Scene.h"
#include "util/UUID.h"
#include "util/util.h"
namespace ORNG {



	struct RelationshipComponent : public Component {
		RelationshipComponent(SceneEntity* p_entity) : Component(p_entity) {};
		size_t num_children = 0;
		entt::entity first{ entt::null };
		entt::entity prev{ entt::null };
		entt::entity next{ entt::null };
		entt::entity parent{ entt::null };
	};

	class SceneEntity {
		friend class EditorLayer;
		friend class Scene;
	public:
		SceneEntity() = delete;
		SceneEntity(Scene* scene, entt::entity entt_handle) : mp_scene(scene), m_entt_handle(entt_handle), m_scene_uuid(scene->uuid()) { AddComponent<TransformComponent>(); AddComponent<RelationshipComponent>(); };
		SceneEntity(uint64_t t_id, entt::entity entt_handle, Scene* scene) : m_uuid(t_id), m_entt_handle(entt_handle), mp_scene(scene), m_scene_uuid(scene->uuid()) { AddComponent<TransformComponent>(); AddComponent<RelationshipComponent>(); };

		~SceneEntity() {

			entt::entity current_child = GetComponent<RelationshipComponent>()->first;
			while (current_child != entt::null) {
				auto next = mp_scene->m_registry.get<RelationshipComponent>(current_child).next;
				mp_scene->GetEntity(current_child)->RemoveParent();
				current_child = next;
			}

			RemoveParent();

			mp_scene->m_registry.destroy(m_entt_handle);
		}

		template<std::derived_from<Component> T, typename... Args>
		T* AddComponent(Args&&... args) { if (HasComponent<T>()) return GetComponent<T>(); return &mp_scene->m_registry.emplace<T>(m_entt_handle, this, std::forward<Args>(args)...); };

		// Returns ptr to component or nullptr if no component was found
		template<std::derived_from<Component> T>
		T* GetComponent() {
			return HasComponent<T>() ? &mp_scene->m_registry.get<T>(m_entt_handle) : nullptr;
		}

		template<typename T>
		bool HasComponent() {
			return mp_scene->m_registry.all_of<T>(m_entt_handle);
		}

		// Deletes component if found, else does nothing.
		template<std::derived_from<Component> T>
		void DeleteComponent() {
			if (!HasComponent<T>())
				return;

			mp_scene->m_registry.remove<T>(m_entt_handle);
		}

		SceneEntity* GetParent() {
			return mp_scene->GetEntity(GetComponent<RelationshipComponent>()->parent);
		}

		void SetParent(SceneEntity& parent_entity) {
			RemoveParent();
			auto* p_comp = AddComponent<RelationshipComponent>();
			auto* p_parent_comp = parent_entity.GetComponent<RelationshipComponent>();

			entt::entity current_parent_child = p_parent_comp->first;

			// Make sure parent is not a child of this entity, if it is return - the parent will not be set
			entt::entity current_parent = p_parent_comp->parent;
			while (current_parent != entt::null) {
				if (current_parent == m_entt_handle) // Check if you're setting this as a parent of one of its children (not allowed)
					return;
				current_parent = mp_scene->m_registry.get<RelationshipComponent>(current_parent).parent;
			}
			p_comp->parent = entt::entity{ parent_entity.GetEnttHandle() };


			if (current_parent_child == entt::null) {
				p_parent_comp->first = m_entt_handle;
				return;
			}

			while (mp_scene->m_registry.get<RelationshipComponent>(current_parent_child).next != entt::null) {
				current_parent_child = mp_scene->m_registry.get<RelationshipComponent>(current_parent_child).next;
			}

			// Link this entity to last entity found in parents linked list of children
			auto& prev_child_of_parent = mp_scene->m_registry.get<RelationshipComponent>(current_parent_child);
			prev_child_of_parent.next = m_entt_handle;
			p_comp->prev = entt::entity{ prev_child_of_parent.GetEnttHandle() };
			p_parent_comp->num_children++;
		}

		void RemoveParent() {
			auto* p_comp = AddComponent<RelationshipComponent>();

			if (p_comp->parent == entt::null)
				return;

			auto& parent_comp = mp_scene->m_registry.get<RelationshipComponent>(p_comp->parent);
			parent_comp.num_children--;

			if (parent_comp.first == entt::entity{m_entt_handle})
				parent_comp.first = entt::null;


			// Patch hole in linked list
			if (auto* prev = mp_scene->m_registry.try_get<RelationshipComponent>(p_comp->prev))
				prev->next = p_comp->next;

			if (auto* next = mp_scene->m_registry.try_get<RelationshipComponent>(p_comp->next))
				next->prev = p_comp->prev;

			p_comp->next = entt::null;
			p_comp->prev = entt::null;
			p_comp->parent = entt::null;

		}


		uint64_t GetUUID() const { return static_cast<uint64_t>(m_uuid); };
		uint64_t GetSceneUUID() const { return m_scene_uuid; };
		uint32_t GetEnttHandle() const { return static_cast<uint32_t>(m_entt_handle); };

		std::string name = "Entity";
	private:
		entt::entity m_entt_handle;
		UUID m_uuid;
		Scene* mp_scene = nullptr;
		uint64_t m_scene_uuid; // Stored seperately for faster access
	};


}