#include "pch/pch.h"

#include "components/MeshComponent.h"
#include "scene/MeshInstanceGroup.h"
#include "util/util.h"
#include "components/TransformComponent.h"
#include "rendering/Renderer.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	MeshComponent::MeshComponent(SceneEntity* p_entity, TransformComponent* t_transform) : Component(p_entity), p_transform(t_transform) {

	};



	void MeshComponent::SetMeshAsset(MeshAsset* p_asset) {
		mp_mesh_asset = p_asset;
		m_materials.clear();

		for (auto* p_material : p_asset->GetSceneMaterials()) {
			m_materials.push_back(p_material);
		}

		mp_instance_group->ResortMesh(this);
	};


	void MeshComponent::SetShaderID(unsigned int id) {
		m_shader_id = id;
		if (mp_mesh_asset)
			mp_instance_group->ResortMesh(this);
	}

	void MeshComponent::SetMaterialID(unsigned int index, const Material* p_material) {
		if (index >= m_materials.size()) {
			OAR_CORE_ERROR("Material ID not set for mesh component of entity '{0}', index out of range", GetEntity()->name);
			BREAKPOINT;
		}

		m_materials[index] = p_material;
		if (mp_mesh_asset)
			mp_instance_group->ResortMesh(this);
	}

	void MeshComponent::RequestTransformSSBOUpdate() {
		m_transform_update_flag = true;
		if (mp_mesh_asset && mp_mesh_asset->GetLoadStatus() == true) mp_instance_group->ActivateFlagSubUpdateWorldMatBuffer();
	}


}