#ifndef COMPONENT_H
#define COMPONENT_H
#include "entt/EnttSingleInclude.h"
namespace ORNG {
	class SceneEntity;

	class Component {
	public:
		friend class Scene;
		Component(SceneEntity* p_entity);


		uint64_t GetEntityUUID() const;
		entt::entity GetEnttHandle() const;
		uint64_t GetSceneUUID() const;
		SceneEntity* GetEntity() { return mp_entity; }
		std::string GetEntityName() const;

	private:
		SceneEntity* mp_entity = nullptr;
	};

	struct RelationshipComponent : public Component {
		RelationshipComponent(SceneEntity* p_entity) : Component(p_entity) {};
		size_t num_children = 0;
		entt::entity first{ entt::null };
		entt::entity prev{ entt::null };
		entt::entity next{ entt::null };
		entt::entity parent{ entt::null };
	};

	struct DataComponent : public Component {
		DataComponent(SceneEntity* p_entity) : Component(p_entity) { };

		template<typename T>
		T Get(const std::string& s) {
			if (!data.contains(s)) {
				throw std::runtime_error(std::format("DataComponent::Get failed, string key '{}' not found", s));
			}

			try {
				return std::any_cast<T>(data[s]);
			}
			catch (std::exception& e) {
				throw std::runtime_error(std::format("DataComponent::Get failed at string key '{}', type is invalid", s));
			}
		};

		template<typename T>
		void Push(const std::string& s, T val) {
			data[s] = static_cast<T>(val);
		}

		std::unordered_map<std::string, std::any> data;
	};
}

#endif