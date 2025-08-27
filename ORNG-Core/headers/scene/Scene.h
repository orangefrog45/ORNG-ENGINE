#pragma once
#include "scene/ScenePostProcessing.h"
#include "../extern/entt/EnttSingleInclude.h"
#include "scene/EntityNodeRef.h"
#include "events/EventManager.h"
#include "components/Component.h"
#include "components/Lights.h"
#include "util/UUID.h"


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
		friend class RuntimeLayer;

		Scene() = default;
		~Scene();

		// Call this just before this scene starts getting updated but after it's fully loaded
		void Start();
		void End();

		// Adds a system to be managed by this scene, should be a heap-allocated ptr to the system
		// Memory for the system is freed when the scene is deleted or the system is removed
		// Systems are loaded, updated, and unloaded in ascending order of priority
		template<typename SystemType>
		SystemType* AddSystem(SystemType* p_system, int priority) {
			ASSERT(!systems.contains(SystemType::GetSystemUUID()));
			systems[SystemType::GetSystemUUID()] = p_system;

			auto it = m_systems_with_priority.begin();
			while (it != m_systems_with_priority.end()) {
				if (it->second > priority) break;
				++it;
			}

			m_systems_with_priority.insert(it, std::make_pair(p_system, priority));

			return p_system;
		}

		template<typename SystemType> 
		bool HasSystem() {
			return systems.contains(SystemType::GetSystemUUID());
		}

		template<typename SystemType>
		void RemoveSystem() {
			ASSERT(systems.contains(SystemType::GetSystemUUID()));
			auto* p_sys = systems[SystemType::GetSystemUUID()];

			auto it = std::ranges::find_if(m_systems_with_priority, [p_sys](const auto& pair) {return pair.first == p_sys;});
			m_systems_with_priority.erase(it);

			delete systems[SystemType::GetSystemUUID()];
			systems.erase(SystemType::GetSystemUUID());
		}

		template<typename SystemType>
		SystemType& GetSystem() {
			ASSERT(HasSystem<SystemType>());
			return *dynamic_cast<SystemType*>(systems[SystemType::GetSystemUUID()]);
		}

		// Allocates pool for component in main application instead of inside a script (needs to be called from main application)
		// This allocation prevents crashes when the scripts memory is released, really only something that can be an issue in the editor as scripts unload/reload.
		template<std::derived_from<Component> T, typename... Args>
		void RegisterComponent(Args&&... args) {
			auto ent = m_registry.create();
			m_registry.emplace<T>(ent, nullptr, std::forward<Args>(args)...);
			m_registry.destroy(ent);

			Events::ECS_EventListener<T> e;
			e.OnEvent = [](auto) {};
			Events::EventManager::RegisterListener(e);
			Events::EventManager::DeregisterListener(e.GetRegisterID());
		}

		void AddDefaultSystems();

		void Update(float ts);

		void OnRender();

		void OnImGuiRender();

		void LoadScene();

		void UnloadScene();

		// This method will clear the scene and deserialize from the scene asset provided
		// This must be the deserialization method used in scripts to avoid crashes or UB
		// scene_asset must be valid at the end of the frame
		void DeserializeAtEndOfFrame(class SceneAsset& scene_asset);

		// Specify uuid if deserializing
		SceneEntity& CreateEntity(const std::string& name, uint64_t uuid = 0);
		void DeleteEntity(SceneEntity* p_entity);
		void DeleteEntityAtEndOfFrame(SceneEntity* p_entity);
		SceneEntity* GetEntity(uint64_t uuid);
		SceneEntity* GetEntity(const std::string& name);
		SceneEntity* GetEntity(entt::entity handle);

		std::vector<SceneEntity*>& GetEntities() {
			return m_entities;
		}

		// Instantiates prefab
		// call_on_create will call the OnCreate function of any script components attached to the prefab or its child entities if true
		SceneEntity& InstantiatePrefab(const Prefab& prefab, bool call_on_create = true);

		SceneEntity* InstantiatePrefab(uint64_t prefab_uuid, bool call_on_create = true);

		// This Duplicate method is what scripts will use, it calls the OnCreate method on the script component of the entity if it has one
		SceneEntity& DuplicateEntityCallScript(SceneEntity& original);

		// Uses a noderef to attempt to find an entity, returns nullptr if none found
		// Begins searching last layer starting from child at index "child_start"
		SceneEntity* TryFindEntity(const EntityNodeRef& ref);

		// Searches only root entities (entities without a parent), returns nullptr if none found
		SceneEntity* TryFindRootEntityByName(const std::string& name);

		// Generates a node ref that can be used to locally reference an entity from src "p_src".
		EntityNodeRef GenEntityNodeRef(SceneEntity* p_src, SceneEntity* p_target);

		static void SortEntitiesNumParents(Scene* p_scene, std::vector<uint64_t>& entity_uuids, bool descending);

		static void SortEntitiesNumParents(std::vector<SceneEntity*>& entities, bool descending);

		void ClearAllEntities(bool clear_reg = true) {
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

			if (clear_reg) m_registry.clear();

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

		PostProcessingSettings post_processing;
		DirectionalLight directional_light;

		std::unordered_map<uint64_t, ComponentSystem*> systems;
		std::unordered_map<uint64_t, SceneEntity*> m_entity_uuid_lookup;
		
		// This points to the default render graph in-engine being used to render the scene
		// This can be freely modified by scripts as long as renderpass dependencies aren't broken
		inline class RenderGraph* GetRenderGraph() noexcept {
			return mp_render_graph;
		}

		// The UUID of the SceneAsset deserialized into this scene
		// 0 if no SceneAsset has been deserialized into the scene yet
		[[nodiscard]] uint64_t GetSceneAssetUUID() const {
			return m_asset_uuid();
		}

		[[nodiscard]] uint64_t GetStaticUUID() const {
			return m_static_uuid();
		}

	private:
		// The UUID of the SceneAsset deserialized into this scene
		// 0 if no SceneAsset has been deserialized into the scene yet
		UUID<uint64_t> m_asset_uuid{0};

		// A static UUID that remains the same on this scene for its lifetime
		UUID<uint64_t> m_static_uuid{};

		std::vector<std::pair<ComponentSystem*, int>> m_systems_with_priority;

		// Set externally by either editor or runtime layer
		RenderGraph* mp_render_graph = nullptr;

		SceneAsset* mp_pending_deserialization_asset = nullptr;

		bool m_is_loaded = false;
		bool m_started = false;

		// Delta time accumulated over each call to Update(), different from application time
		double m_time_elapsed = 0.0;

		// Favour using a prefab instead of duplicating entities for performance
		SceneEntity& DuplicateEntity(SceneEntity& original);

		SceneEntity& DuplicateEntityAsPartOfGroup(SceneEntity& original, std::unordered_map<uint64_t, uint64_t>& uuid_lookup);

		// Resolves any EntityNodeRefs locally in the group if possible, so things like joints stay connected properly
		// This only needs to be used over DuplicateEntity when duplicating sets of entities that reference entities that may share the same parent
		// For performance, it's better to store the duplicates as children of some other entity and just call DuplicateEntity() on that one entity, refs get resolved locally quickly this way
		std::vector<SceneEntity*> DuplicateEntityGroup(const std::vector<SceneEntity*>& group);

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