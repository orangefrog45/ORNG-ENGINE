#pragma once
#include "scene/Scene.h"

namespace ORNG {

	class SceneEntity {
	public:
		SceneEntity(unsigned long t_id, Scene* scene) : m_id(t_id), mp_scene(scene) {};

		template<std::derived_from<Component> T, typename... Args>
		T* AddComponent(Args... args) { return mp_scene->AddComponent<T>(m_id, args...); };

		template<std::derived_from<Component> T>
		T* GetComponent() {
			T* comp = mp_scene->GetComponent<T>(m_id);
			if (comp == nullptr) {
				return nullptr;
			}
			else {
				return comp;
			}
		}

		template<std::derived_from<Component> T>
		void DeleteComponent() {
			if (!GetComponent<T>()) {
				OAR_CORE_ERROR("Error deleting component for entity '{0}', does not have specified component", name);
				return;
			}
			mp_scene->DeleteComponent<T>(m_id);
		}

		int GetID() const { return m_id; };

		const char* name = "Entity";
	private:
		unsigned long m_id;
		Scene* mp_scene = nullptr;
	};

}