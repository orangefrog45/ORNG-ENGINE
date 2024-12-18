#pragma once
#include "scene/Skybox.h"
#include "scene/ScenePostProcessing.h"
#include "../extern/entt/EnttSingleInclude.h"
#include "scene/EntityNodeRef.h"
#include "events/EventManager.h"
#include "components/ComponentAPI.h"
#include "terrain/Terrain.h"


namespace ORNG {
	struct Prefab;
	class SceneEntity;
	class ComponentSystem;

	struct UUIDChangeEvent : public Events::Event {
		UUIDChangeEvent(uint64_t _old, uint64_t _new) : old_uuid(_old), new_uuid(_new) {};
		uint64_t old_uuid;
		uint64_t new_uuid;
	};

	class Scene {
	public:
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneEntity;
		friend class AssetManagerWindow;

		Scene();
		~Scene();

		// Adds a system to be managed by this scene, should be a heap-allocated ptr to the system
		// Memory for the system is freed when the scene is deleted
		template<typename SystemType>
		SystemType* AddSystem(SystemType* p_system) {
			ASSERT(!systems.contains(type_id<SystemType>));
			systems[type_id<SystemType>] = p_system;
			return p_system;
		}

		template<typename SystemType>
		SystemType& GetSystem() {
			ASSERT(systems.contains(type_id<SystemType>));
			return *dynamic_cast<SystemType*>(systems[type_id<SystemType>]);
		}

		template<typename SystemType>
		bool HasSystem() {
			return systems.contains(type_id<SystemType>);
		}

		// Allocates pool for component in main application instead of inside a script (needs to be called from main application)
		// This allocation prevents crashes when the scripts memory is released
		template<std::derived_from<Component> T>
		void RegisterComponent() {
			auto ent = m_registry.create();
			m_registry.emplace<T>(ent, nullptr);
			m_registry.destroy(ent);
		}

		void AddDefaultSystems();

		void OnStart();

		void Update(float ts);

		void OnImGuiRender();

		void LoadScene();

		void UnloadScene();

		CameraComponent* GetActiveCamera();

		// Specify uuid if deserializing
		SceneEntity& CreateEntity(const std::string& name, uint64_t uuid = 0);
		void DeleteEntity(SceneEntity* p_entity);
		void DeleteEntityAtEndOfFrame(SceneEntity* p_entity);
		SceneEntity* GetEntity(uint64_t uuid);
		SceneEntity* GetEntity(const std::string& name);
		SceneEntity* GetEntity(entt::entity handle);


		const SI& GetSI() {
			return m_si;
		}

		// Instantiates prefab without calling OnCreate, used for instantiation while the scene is paused e.g in the editor
		SceneEntity& InstantiatePrefab(const Prefab& prefab);

		// This instantiation method is what scripts will use, it calls the OnCreate method on the script component of the prefab if it has one
		SceneEntity& InstantiatePrefabCallScript(const Prefab& prefab);

		// This Duplicate method is what scripts will use, it calls the OnCreate method on the script component of the entity if it has one
		SceneEntity& DuplicateEntityCallScript(SceneEntity& original);

		// Duplicates entities as a group and calls OnCreate() on them
		SceneEntity& DuplicateEntityGroupCallScript(const std::vector<SceneEntity*> group);

		// Uses a noderef to attempt to find an entity, returns nullptr if none found
		// Begins searching last layer starting from child at index "child_start"
		SceneEntity* TryFindEntity(const EntityNodeRef& ref);

		// Searches only root entities (entities without a parent), returns nullptr if none found
		SceneEntity* TryFindRootEntityByName(const std::string& name);

		// Generates a node ref that can be used to locally reference an entity from src "p_src".
		EntityNodeRef GenEntityNodeRef(SceneEntity* p_src, SceneEntity* p_target);

		static void SortEntitiesNumParents(Scene* p_scene, std::vector<uint64_t>& entity_uuids, bool descending);

		static void SortEntitiesNumParents(std::vector<SceneEntity*>& entities, bool descending);

		void ClearAllEntities() {
			unsigned int max_iters = 10'000'000;
			unsigned int i = 0;
			while (!m_entities.empty()) {
				i++;

				if (i > max_iters) {
					ORNG_CORE_CRITICAL("Scene::ClearAllEntities exceeded max iteration count of 10,000,000, exiting");
					break;
				}

				DeleteEntity(m_entities[0]);
			}

			m_registry.clear();
			ASSERT(m_entities.empty());
			ASSERT(m_root_entities.empty());
		}


		entt::registry& GetRegistry() {
			return m_registry;
		}

		bool IsLoaded() const noexcept {
			return m_is_loaded;
		}

		double GetTimeElapsed() const noexcept {
			return m_time_elapsed;
		}

		UUID<uint64_t> uuid;

		Skybox skybox;
		PostProcessing post_processing;
		DirectionalLight directional_light;
		Terrain terrain;

		std::unordered_map<uint16_t, ComponentSystem*> systems;
		std::unordered_map<uint64_t, SceneEntity*> m_entity_uuid_lookup;

	private:
		void SetScriptState();
		bool m_is_loaded = false;

		// Delta time accumulated over each call to Update(), different from application time
		double m_time_elapsed = 0.0;

		SI m_si;

		// Favour using a prefab instead of duplicating entities for performance
		SceneEntity& DuplicateEntity(SceneEntity& original);

		SceneEntity& DuplicateEntityAsPartOfGroup(SceneEntity& original, std::unordered_map<uint64_t, uint64_t>& uuid_lookup);

		// Resolves any EntityNodeRefs locally in the group if possible, so things like joints stay connected properly
		// This only needs to be used over DuplicateEntity when duplicating sets of entities that reference entities that may share the same parent
		// For performance, it's better to store the duplicates as children of some other entity and just call DuplicateEntity() on that one entity, refs get resolved locally quickly this way
		std::vector<SceneEntity*> DuplicateEntityGroup(const std::vector<SceneEntity*> group);

		std::vector<SceneEntity*> m_entities;
		std::vector<SceneEntity*> m_entity_deletion_queue;

		// Entities without a parent, stored so they can be quickly found when a noderef path is being formed
		std::unordered_set<entt::entity> m_root_entities;
		Events::ECS_EventListener<RelationshipComponent> m_hierarchy_modification_listener;
		Events::EventListener<UUIDChangeEvent> m_uuid_change_listener;

		entt::registry m_registry;

		std::string m_name = "Untitled scene";
	};
}