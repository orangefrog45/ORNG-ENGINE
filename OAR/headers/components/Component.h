#pragma once
namespace ORNG {

	class SceneEntity;

	class Component {
	public:
		friend class Scene;
		Component(SceneEntity* p_entity);
		uint64_t GetEntityHandle() const;
		SceneEntity* GetEntity() { return mp_entity; }

	private:
		SceneEntity* mp_entity = nullptr;
	};
}