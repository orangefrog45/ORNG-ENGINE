#pragma once
#include "components/ComponentAPI.h"
#include "scene/MeshInstanceGroup.h"

namespace ORNG {

	class ComponentManager {
		virtual void OnUpdate() = 0;
		virtual void OnUnload() = 0;
	};

	class TransformComponentManager : public ComponentManager {
	public:
		void OnUpdate() final { /* Not needed currently for transforms */ };
		void OnUnload() final;

		TransformComponent* AddComponent(SceneEntity* p_entity);
		TransformComponent* GetComponent(unsigned long entity_id);

	private:
		std::vector<TransformComponent*> m_transform_components;
	};


	class SpotlightComponentManager : public ComponentManager {
	public:
		void OnLoad();
		void OnUpdate() final;
		void OnUnload() final;
		SpotLightComponent* AddComponent(SceneEntity* p_entity);
		SpotLightComponent* GetComponent(unsigned long entity_id);
		const auto& GetComponents() const { return m_spotlight_components; }
		void DeleteComponent(SceneEntity* p_entity);
	private:
		std::vector<SpotLightComponent*> m_spotlight_components;
		GLuint m_spotlight_ssbo_handle;
		bool m_light_buffer_update_flag = false;
	};



	class PointlightComponentManager : public ComponentManager {
	public:
		void OnLoad();
		void OnUpdate() final;
		void OnUnload() final;
		PointLightComponent* AddComponent(SceneEntity* p_entity);
		PointLightComponent* GetComponent(unsigned long entity_id);
		void DeleteComponent(SceneEntity* p_entity);

		const auto& GetComponents() { return m_pointlight_components; }
	private:
		std::vector<PointLightComponent*> m_pointlight_components;
		GLuint m_pointlight_ssbo_handle;
		bool m_light_buffer_update_flag = false;
	};


	class MeshComponentManager : public ComponentManager {
	public:
		void SortMeshIntoInstanceGroup(MeshComponent* comp, MeshAsset* asset);
		MeshComponent* AddComponent(SceneEntity* p_entity, MeshAsset* asset);
		MeshComponent* GetComponent(unsigned long entity_id);
		void DeleteComponent(SceneEntity* p_entity);
		void OnLoad();
		void OnUnload() final;
		void OnUpdate() final;
		void OnMeshAssetDeletion(MeshAsset* p_asset);

		const auto& GetInstanceGroups() { return m_instance_groups; }
		const auto& GetMeshComponents() { return m_mesh_components; }
	private:

		std::vector<MeshInstanceGroup*> m_instance_groups;
		std::vector<MeshComponent*> m_mesh_components;
	};
}