#pragma once
#include <BasicMesh.h>
#include "WorldTransform.h"
class MeshEntity {
public:
	explicit MeshEntity(BasicMesh* t_mesh_data);
	~MeshEntity();

	void UpdateInstanceTransformBuffers();

	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);
	void SetColor(const glm::fvec3& t_color) { color = t_color; }
	void SetInstanceTransforms(std::vector<WorldTransform const*>* vec) { m_instance_transforms = vec; };

	auto GetMeshData() const { return m_mesh_data; }
	glm::fvec3 GetColor() { return color; };
	WorldTransform* const GetWorldTransform() const { return m_transform; };

private:
	glm::fvec3 color = glm::fvec3(0.5f, 0.5f, 0.5f);
	std::vector<WorldTransform const*>* m_instance_transforms = nullptr; // FOR UPDATING TRANSFORM BUFFERS ONLY WHEN NEEDED
	BasicMesh* m_mesh_data = nullptr;
	WorldTransform* m_transform = new WorldTransform();
};