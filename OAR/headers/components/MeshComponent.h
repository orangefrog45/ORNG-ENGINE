#pragma once
#include "MeshData.h"
#include "SceneEntity.h"

class MeshData;
class MeshInstanceGroup;
class WorldTransform;

class MeshComponent : public SceneEntity
{
public:
	MeshComponent(MeshData *t_mesh_data, MeshInstanceGroup *instance_group, unsigned int entity_id, MeshData::MeshShaderMode t_shader_mode = MeshData::MeshShaderMode::LIGHTING);
	~MeshComponent();
	MeshComponent(const MeshComponent &other);

	void UpdateInstanceTransformBuffers();

	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);
	inline void SetColor(const glm::vec3 &t_color) { salt_color = t_color; }
	inline void SetInstanceGroup(MeshInstanceGroup *group) { salt_m_instance_group = group; };

	inline MeshData::MeshShaderMode GetShaderMode() const { return salt_m_shader_mode; }
	inline auto &GetMeshData() const { return salt_m_mesh_data; }
	inline auto &GetInstanceGroup() const { return salt_m_instance_group; };
	inline glm::vec3 GetColor() const { return salt_color; };
	inline WorldTransform const *GetWorldTransform() const { return salt_m_transform; };

private:
	MeshInstanceGroup *salt_m_instance_group = nullptr;
	MeshData::MeshShaderMode salt_m_shader_mode = MeshData::MeshShaderMode::LIGHTING;
	glm::vec3 salt_color = glm::vec3(0.5f, 0.5f, 0.5f);
	MeshData *salt_m_mesh_data = nullptr;
	WorldTransform *salt_m_transform = nullptr;
};