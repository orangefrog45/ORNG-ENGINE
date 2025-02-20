#pragma once

#include "events/Events.h"
#include "components/systems/ComponentSystem.h"
#include "scene/Scene.h"
#include "components/MeshComponent.h"
#include "components/BillboardComponent.h"

namespace ORNG {
	class MeshInstancingSystem : public ComponentSystem {
	public:
		MeshInstancingSystem(Scene* p_scene);
		virtual ~MeshInstancingSystem() = default;
		void SortMeshIntoInstanceGroup(MeshComponent* comp);
		void OnLoad() override;
		void OnUnload() override;
		void OnUpdate() override;
		void OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		const auto& GetInstanceGroups() const { return m_instance_groups; }
		const auto& GetBillboardInstanceGroups() const { return m_billboard_instance_groups; }

		inline static constexpr uint64_t GetSystemUUID() { return 7828929347847; }

	private:
		void OnMeshAssetDeletion(MeshAsset* p_asset);
		void OnMaterialDeletion(Material* p_material);

		void SortBillboardIntoInstanceGroup(BillboardComponent* p_comp);
		void OnBillboardAdd(BillboardComponent* p_comp);
		void OnBillboardRemove(BillboardComponent* p_comp);

		// Listener for asset deletion
		Events::EventListener<Events::AssetEvent> m_asset_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::ECS_EventListener<MeshComponent> m_mesh_listener;
		std::vector<MeshInstanceGroup*> m_instance_groups;

		Events::ECS_EventListener<BillboardComponent> m_billboard_listener;
		std::vector<MeshInstanceGroup*> m_billboard_instance_groups;

		entt::connection m_mesh_add_connection;
		entt::connection m_mesh_remove_connection;
		entt::connection m_billboard_add_connection;
		entt::connection m_billboard_remove_connection;

		unsigned m_default_group_end_index = 0;
	};
}