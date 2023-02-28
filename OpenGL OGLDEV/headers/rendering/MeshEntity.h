#pragma once
#include <BasicMesh.h>
#include "WorldTransform.h"
class MeshEntity {
public:
	//TODO: entities need to unload too
	//TODO: add worldtransform modification functions (tie in with transformbufferupdate optimization)
	MeshEntity(BasicMesh* t_mesh_data);
	~MeshEntity();

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetTransform(WorldTransform& t_transform);
	void SetInstanceTransforms(std::vector<WorldTransform const*>* vec) { m_instance_transforms = vec; };
	void UpdateInstanceTransformBuffers();
	WorldTransform const* GetWorldTransform() { return m_transform; };
private:
	std::vector<WorldTransform const*>* m_instance_transforms = nullptr; // FOR UPDATING TRANSFORM BUFFERS ONLY WHEN NEEDED
	BasicMesh* m_mesh_data = nullptr;
	WorldTransform* m_transform = new WorldTransform();
};