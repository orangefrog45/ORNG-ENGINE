#pragma once
#include "scene/Scene.h"
#include "util/UUID.h"
#include "util/util.h"
namespace ORNG {

	class SceneEntity {
		friend class EditorLayer;
		friend class Scene;
	public:
		SceneEntity() = delete;
		SceneEntity(Scene* scene, entt::entity entt_handle) : mp_scene(scene), m_entt_handle(entt_handle), m_scene_uuid(scene->uuid()) {};
		SceneEntity(uint64_t t_id, entt::entity entt_handle, Scene* scene) : m_uuid(t_id), m_entt_handle(entt_handle), mp_scene(scene), m_scene_uuid(scene->uuid()) {};

		~SceneEntity() {
			SetParent(nullptr);
			DeleteComponent<PhysicsComponent>();
			DeleteComponent<MeshComponent>();
			DeleteComponent<PointLightComponent>();
			DeleteComponent<SpotLightComponent>();
			DeleteComponent<CameraComponent>();
			DeleteComponent<TransformComponent>();

			for (auto* p_child : m_children) {
				p_child->SetParent(nullptr);
			}


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


		void AddChild(SceneEntity* p_other) {
			if (VectorContains(m_children, p_other) || p_other == mp_parent)
				return;

			m_children.push_back(p_other);
		}

		void RemoveChild(SceneEntity* p_entity) {
			if (m_children.empty())
				return;

			auto it = std::ranges::find(m_children, p_entity);
			if (it == m_children.end()) {
				ORNG_CORE_ERROR("Failure to remove entity child, entity '{0}' is not a child of entity {1}", p_entity->m_uuid(), m_uuid());
				return;
			}

			(*it)->mp_parent = nullptr;
			m_children.erase(it);
			auto* p_transform = p_entity->GetComponent<TransformComponent>();
			if (!p_transform)
				return;


		}

		SceneEntity* GetParent() {
			return mp_parent;
		}

		void SetParent(SceneEntity* p_parent) {
			auto* p_current_parent = mp_parent;
			while (p_current_parent && p_parent) {
				if (p_current_parent == this || p_parent == p_current_parent) {
					return;
				}
				p_current_parent = p_current_parent->mp_parent;
			}
			if (VectorContains(m_children, p_parent))
				return;

			if (mp_parent)
				mp_parent->RemoveChild(this);

			mp_parent = p_parent;

			if (p_parent)
				p_parent->AddChild(this);
		}

		void RemoveParent() {
			SetParent(nullptr);
		}

		uint64_t GetUUID() const { return static_cast<uint64_t>(m_uuid); };
		uint64_t GetSceneUUID() const { return m_scene_uuid; };
		uint32_t GetEnttHandle() const { return static_cast<uint32_t>(m_entt_handle); };

		std::string name = "Entity";
	private:
		entt::entity m_entt_handle;
		UUID m_uuid;
		std::vector<SceneEntity*> m_children;
		SceneEntity* mp_parent = nullptr;
		Scene* mp_scene = nullptr;
		uint64_t m_scene_uuid; // Stored seperately for faster access
	};

}