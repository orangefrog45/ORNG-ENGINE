#pragma once
#include <vector>
#include "WorldTransform.h"
#include "BasicMesh.h"
#include "MeshEntity.h"
#include "util/util.h"



class EntityInstanceGroup {
public:
	EntityInstanceGroup(BasicMesh* t_mesh_data) : m_mesh_data(t_mesh_data) {};
	~EntityInstanceGroup();
	void InitializeTransformBuffers();
	void AddInstance(MeshEntity* entity);
	//void ModifyInstance(unsigned int index, glm::fmat4& transform);
	//void DeleteInstance(unsigned int index);

	unsigned int m_instances;
	std::vector<MeshEntity*> m_mesh_entities;
	std::vector< WorldTransform const*>* m_transforms = new std::vector< WorldTransform const*>();
	BasicMesh* m_mesh_data;

};
