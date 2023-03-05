#pragma once
#include <BasicMesh.h>
#include "util/util.h"
#include "WorldTransform.h"

class MeshEntity {
public:
	explicit MeshEntity(BasicMesh* t_mesh_data, MeshShaderMode t_shader_mode = MeshShaderMode::LIGHTING) : m_mesh_data(t_mesh_data), m_shader_mode(t_shader_mode) {};
	~MeshEntity();

	void UpdateInstanceTransformBuffers();

	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);
	void SetColor(const glm::fvec3& t_color) { color = t_color; }
	void SetInstanceTransforms(std::vector<WorldTransform const*>* vec) { m_instance_transforms = vec; };

	MeshShaderMode GetShaderMode() const { return m_shader_mode; }
	auto GetMeshData() const { return m_mesh_data; }
	glm::fvec3 GetColor() const { return color; };
	WorldTransform* const GetWorldTransform() const { return m_transform; };

private:
	MeshShaderMode m_shader_mode = MeshShaderMode::LIGHTING;
	glm::fvec3 color = glm::fvec3(0.5f, 0.5f, 0.5f);
	std::vector<WorldTransform const*>* m_instance_transforms = nullptr; // FOR UPDATING TRANSFORM BUFFERS ONLY WHEN NEEDED
	BasicMesh* m_mesh_data = nullptr;
	WorldTransform* m_transform = new WorldTransform();
};