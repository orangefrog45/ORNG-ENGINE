#pragma once
namespace ORNG {

	class SceneEntity;

	class Component {
	public:
		friend class Scene;
		Component(SceneEntity* p_entity);


		uint64_t GetEntityUUID() const;
		uint32_t GetEnttHandle() const;
		uint64_t GetSceneUUID() const;
		SceneEntity* GetEntity() { return mp_entity; }

	private:
		SceneEntity* mp_entity = nullptr;
	};

}