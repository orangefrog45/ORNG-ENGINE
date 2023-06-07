#pragma once
namespace ORNG {

	class SceneEntity;

	class Component {
	public:
		friend class Renderer;
		friend class Scene;
		Component(unsigned long entity_handle) : m_entity_handle(entity_handle) {};
		inline unsigned long GetEntityHandle() const { return m_entity_handle; }
	private:
		unsigned long m_entity_handle;
	};
}