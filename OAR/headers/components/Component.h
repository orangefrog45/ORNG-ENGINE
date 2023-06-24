#pragma once
namespace ORNG {

	class SceneEntity;

	class Component {
	public:
		friend class Scene;
		Component(SceneEntity* p_entity);
		unsigned long GetEntityHandle() const;
		SceneEntity* GetEntity() { return mp_entity; }

	private:
		SceneEntity* mp_entity = nullptr;
	};
}