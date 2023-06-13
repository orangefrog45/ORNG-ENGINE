#include "pch/pch.h"

#include "components/MeshComponent.h"
#include "rendering/MeshInstanceGroup.h"
#include "util/util.h"
#include "components/WorldTransform.h"
#include "rendering/Renderer.h"

namespace ORNG {

	MeshComponent::MeshComponent(unsigned long entity_id) : Component(entity_id) { mp_transform = new WorldTransform(); };

	MeshComponent::~MeshComponent() {
		delete mp_transform;
	}

	void MeshComponent::SetPosition(const float x, const float y, const float z) {
		mp_transform->SetPosition(x, y, z);
		RequestTransformSSBOUpdate();
	};

	void MeshComponent::SetMeshAsset(MeshAsset* p_asset) {
		mp_mesh_asset = p_asset;
		mp_instance_group->ResortMesh(this);
	};


	void MeshComponent::SetOrientation(const float x, const float y, const float z) {
		mp_transform->SetOrientation(x, y, z);
		RequestTransformSSBOUpdate();
	};

	void MeshComponent::SetScale(const float x, const float y, const float z) {
		mp_transform->SetScale(x, y, z);
		RequestTransformSSBOUpdate();
	};

	void MeshComponent::SetPosition(const glm::vec3 transform) {
		mp_transform->SetPosition(transform.x, transform.y, transform.z);
		RequestTransformSSBOUpdate();
	};

	void MeshComponent::SetOrientation(const glm::vec3 transform) {
		mp_transform->SetOrientation(transform.x, transform.y, transform.z);
		RequestTransformSSBOUpdate();
	};

	void MeshComponent::SetScale(const glm::vec3 transform) {
		mp_transform->SetScale(transform.x, transform.y, transform.z);
		RequestTransformSSBOUpdate();
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