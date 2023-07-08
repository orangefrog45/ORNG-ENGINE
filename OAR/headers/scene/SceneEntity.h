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
		SceneEntity(Scene* scene) : mp_scene(scene) {};
		SceneEntity(uint64_t t_id, Scene* scene) : m_uuid(t_id), mp_scene(scene) {};

		~SceneEntity() {
			SetParent(nullptr);
			DeleteComponent<PhysicsComponent>();
			DeleteComponent<MeshComponent>();
			DeleteComponent<PointLightComponent>();
			DeleteComponent<SpotLightComponent>();
			DeleteComponent<CameraComponent>();
			DeleteComponent<TransformComponent>();


		}

		template<std::derived_from<Component> T, typename... Args>
		T* AddComponent(Args... args) { return mp_scene->AddComponent<T>(this, args...); };

		template<std::derived_from<Component> T>
		T* GetComponent() {
			T* comp = mp_scene->GetComponent<T>(m_uuid());
			return comp;
		}

		template<std::derived_from<Component> T>
		void DeleteComponent() {
			mp_scene->DeleteComponent<T>(this);
		}


		void AddChild(SceneEntity* p_other) {
			if (VectorContains(m_children, p_other) || p_other == mp_parent)
				return;

			p_other->GetComponent<TransformComponent>()->SetParentTransform(GetComponent<TransformComponent>());
			m_children.push_back(p_other);
		}

		void RemoveChild(SceneEntity* p_entity) {
			if (m_children.empty())
				return;

			auto it = std::ranges::find(m_children, p_entity);
			if (it == m_children.end()) {
				OAR_CORE_ERROR("Failure to remove entity child, entity '{0}' is not a child of entity {1}", p_entity->m_uuid(), m_uuid());
				return;
			}

			(*it)->mp_parent = nullptr;
			m_children.erase(it);
			auto* p_transform = p_entity->GetComponent<TransformComponent>();
			if (!p_transform)
				return;


			p_transform->RemoveParentTransform();
		}

		SceneEntity* GetParent() {
			return mp_parent;
		}

		void SetParent(SceneEntity* p_parent) {
			auto* p_current_parent = mp_parent;
			while (p_current_parent) {
				if (p_current_parent == this || p_parent == p_current_parent)
					return;
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

		uint64_t GetID() const { return (uint64_t)m_uuid; };

		std::string name = "Entity";
	private:

		UUID m_uuid;
		std::vector<SceneEntity*> m_children;
		SceneEntity* mp_parent = nullptr;
		Scene* mp_scene = nullptr;
	};

}