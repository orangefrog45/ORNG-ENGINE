#pragma once
#include <BasicMesh.h>
#include <memory>
#include "WorldTransform.h"
class MeshEntity {
public:
	//TODO: entities need to unload too
	//TODO: add worldtransform modification functions (tie in with transformbufferupdate optimization)
	MeshEntity(unsigned int t_instances, BasicMesh* t_mesh_data);
	void CombineEntity(MeshEntity& entity);

	BasicMesh* m_mesh_data = nullptr;
	std::vector<WorldTransform> m_world_transforms;
	unsigned int instances;
private:
};