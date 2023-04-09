#pragma once
#include <vector>
#include "MeshData.h"

class WorldTransform;

class MeshInstanceGroup {
public:
	MeshInstanceGroup(MeshData* t_mesh_data, MeshData::MeshShaderMode shader_mode) : m_mesh_data(t_mesh_data), m_group_shader_type(shader_mode) {};
	void InitializeTransformBuffers();
	void AddTransformPtr(WorldTransform const* ptr) { m_transforms.push_back(ptr); m_instances++; }
	void UpdateMeshTransformBuffers() { m_mesh_data->UpdateTransformBuffers(m_transforms); }
	auto ClearTransforms() { m_transforms.clear(); }
	void ValidateTransformPtrs();

	unsigned int GetInstances() const { return m_instances; }
	auto GetMeshData() const { return m_mesh_data; }
	auto GetShaderType() const { return m_group_shader_type; }
private:
	// equal to transforms.size()
	unsigned int m_instances = 0;

	std::vector<WorldTransform const*> m_transforms;
	MeshData* m_mesh_data;
	MeshData::MeshShaderMode m_group_shader_type;
};
