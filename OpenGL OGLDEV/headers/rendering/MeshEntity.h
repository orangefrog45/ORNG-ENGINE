#pragma once
#include <BasicMesh.h>
#include "WorldTransform.h"
class MeshEntity {
public:
	MeshEntity(BasicMesh* t_mesh_data);
	~MeshEntity();

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetTransform(WorldTransform& t_transform);
	void SetInstanceTransforms(std::vector<WorldTransform const*>* vec) { m_instance_transforms = vec; };
	auto GetMeshData() { return m_mesh_data; }
	void UpdateInstanceTransformBuffers();
	WorldTransform const* GetWorldTransform() { return m_transform; };
	glm::fvec3 color = glm::fvec3(0.5f, 0.5f, 0.5f);
private:
	std::vector<WorldTransform const*>* m_instance_transforms = nullptr; // FOR UPDATING TRANSFORM BUFFERS ONLY WHEN NEEDED
	BasicMesh* m_mesh_data = nullptr;
	WorldTransform* m_transform = new WorldTransform();
};