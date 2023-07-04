#pragma once
#include "scene/Scene.h"
#include "util/UUID.h"
namespace ORNG {

	class SceneEntity {
		friend class EditorLayer;
	public:
		SceneEntity() = delete;
		SceneEntity(Scene* scene) : mp_scene(scene) {};
		SceneEntity(uint64_t t_id, Scene* scene) : m_uuid(t_id), mp_scene(scene) {};

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
			p_other->GetComponent<TransformComponent>()->SetParentTransform(GetComponent<TransformComponent>());
			m_children.push_back(p_other);
		}

		void RemoveChild(SceneEntity* p_entity) {
			p_entity->GetComponent<TransformComponent>()->RemoveParentTransform();
			m_children.erase(std::find(m_children.begin(), m_children.end(), p_entity));
		}

		SceneEntity* GetParent() {
			return mp_parent;
		}

		void SetParent(SceneEntity* p_parent) {
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