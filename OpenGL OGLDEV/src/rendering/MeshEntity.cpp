#include "MeshEntity.h"
#include "util/util.h"

MeshEntity::MeshEntity(unsigned int t_instances, BasicMesh* t_mesh_data) : instances(t_instances), m_mesh_data(t_mesh_data) {
	for (unsigned int i = 0; i < instances; i++) {
		m_world_transforms.push_back(WorldTransform());
	}
};

void MeshEntity::CombineEntity(MeshEntity& entity) {

	//combine worldtransforms
	std::vector<WorldTransform> new_t_vector;
	new_t_vector.reserve(entity.m_world_transforms.size() + m_world_transforms.size());
	new_t_vector.insert(new_t_vector.end(), entity.m_world_transforms.begin(), entity.m_world_transforms.end());
	new_t_vector.insert(new_t_vector.end(), m_world_transforms.begin(), m_world_transforms.end());
	m_world_transforms.clear();
	m_world_transforms.insert(m_world_transforms.end(), new_t_vector.begin(), new_t_vector.end());

	//combine instances
	instances += entity.instances;
}

