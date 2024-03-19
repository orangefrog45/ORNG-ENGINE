#pragma once
#include "scene/Skybox.h"
#include "terrain/Terrain.h"
#include "components/ComponentSystems.h"
#include "scene/ScenePostProcessing.h"
#include "../extern/entt/EnttSingleInclude.h"
#include "EntityNodeRef.h"


namespace ORNG {
	struct Prefab;
	class SceneEntity;

	class Scene {
	public:
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneEntity;
		friend class AssetManagerWindow;
		Scene() = default;
		~Scene();

		void Update(float ts);

		// Specify uuid if deserializing
		SceneEntity& CreateEntity(const std::string& name, uint64_t uuid = 0);
		void DeleteEntity(SceneEntity* p_entity);
		SceneEntity* GetEntity(uint64_t uuid);
		SceneEntity* GetEntity(const std::string& name);
		SceneEntity* GetEntity(entt::entity handle);

		void OnStart();

		CameraComponent* GetActiveCamera() {
			return m_camera_system.GetActiveCamera();
		}

		// Instantiates prefab without calling OnCreate, used for instantiation while the scene is paused e.g in the editor
		SceneEntity& InstantiatePrefab(const std::string& serialized_data);

		// This instantiation method is what scripts will use, it calls the OnCreate method on the script component of the prefab if it has one
		SceneEntity& InstantiatePrefabCallScript(const std::string& serialized_data);

		// This Duplicate method is what scripts will use, it calls the OnCreate method on the script component of the entity if it has one
		SceneEntity& DuplicateEntityCallScript(SceneEntity& original);

		// Duplicates entities as a group and calls OnCreate() on them
		SceneEntity& DuplicateEntityGroupCallScript(const std::vector<SceneEntity*> group);

		// Uses a noderef to attempt to find an entity, returns nullptr if none found
		// Begins searching last layer starting from child at index "child_start"
		SceneEntity* TryFindEntity(const EntityNodeRef& ref);

		// Searches only root entities (entities without a parent), returns nullptr if none found
		SceneEntity* TryFindRootEntityByName(const std::string& name);

		// Searches only root entities (entities without a parent), returns nullptr if none found
		SceneEntity* TryFindRootEntityByNodeID(uint32_t node_id);

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

		void LoadScene(const std::string& filepath);

		void UnloadScene();

		entt::registry& GetRegistry() {
			return m_registry;
		}

		bool IsLoaded() {
			return m_is_loaded;
		}

		UUID<uint64_t> uuid;

		Skybox skybox;
		Terrain terrain;
		PostProcessing post_processing;
		DirectionalLight directional_light;

		PhysicsSystem physics_system{ &m_registry, uuid(), this };
	private:
		void SetScriptState();
		bool m_is_loaded = false;
		bool active = false;

		SceneEntity& DuplicateEntity(SceneEntity& original, bool duplicating_as_part_of_group);

		// Resolves any EntityNodeRefs locally in the group if possible, so things like joints stay connected properly
		// This only needs to be used over DuplicateEntity when duplicating sets of entities that reference entities that may share the same parent
		// For performance, it's better to store the duplicates as children of some other entity and just call DuplicateEntity() on that one entity, refs get resolved locally quickly this way
		std::vector<SceneEntity*> DuplicateEntityGroup(const std::vector<SceneEntity*> group);

		std::vector<SceneEntity*> m_entities;
		std::vector<SceneEntity*> m_entity_deletion_queue;

		// Entities without a parent, stored so they can be quickly found when a noderef path is being formed
		std::unordered_set<entt::entity> m_root_entities;
		Events::ECS_EventListener<RelationshipComponent> m_hierarchy_modification_listener;

		MeshInstancingSystem m_mesh_component_manager{ &m_registry, uuid() };
		CameraSystem m_camera_system{ &m_registry, uuid() };
		TransformHierarchySystem m_transform_system{ &m_registry, uuid() };
		AudioSystem m_audio_system{ &m_registry, uuid(), &m_camera_system.m_active_cam_entity_handle };
		ParticleSystem m_particle_system{ &m_registry, uuid() };

		entt::registry m_registry;

		std::string m_name = "Untitled scene";
	};
}