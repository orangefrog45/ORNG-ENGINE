#include "pch/pch.h"

#include "components/MeshComponent.h"
#include "scene/MeshInstanceGroup.h"
#include "util/util.h"
#include "components/TransformComponent.h"
#include "rendering/Renderer.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	MeshComponent::MeshComponent(SceneEntity* p_entity) : Component(p_entity) {
		transform.OnTransformUpdate = [this] {
			RequestTransformSSBOUpdate();
		};
	};



	void MeshComponent::SetMeshAsset(MeshAsset* p_asset) {
		mp_mesh_asset = p_asset;
		mp_instance_group->ResortMesh(this);
	};


	void MeshComponent::SetShaderID(unsigned int id) {
		m_shader_id = id;
		if (mp_mesh_asset)
			mp_instance_group->ResortMesh(this);
	}

	void MeshComponent::SetMaterialID(unsigned int index, const Material* p_material) {
		m_materials[index] = p_material;
		if (mp_mesh_asset)
			mp_instance_group->ResortMesh(this);
	}

	void MeshComponent::RequestTransformSSBOUpdate() {
		m_transform_update_flag = true;
		if (mp_mesh_asset && mp_mesh_asset->GetLoadStatus() == true) mp_instance_group->ActivateFlagSubUpdateWorldMatBuffer();
	}


}