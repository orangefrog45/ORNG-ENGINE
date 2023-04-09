#pragma once
#include "MeshData.h"
#include "SceneEntity.h"

class MeshData;
class MeshInstanceGroup;
class WorldTransform;

class MeshComponent : public SceneEntity {
public:
	MeshComponent(MeshData* t_mesh_data, MeshInstanceGroup* instance_group, unsigned int entity_id, MeshData::MeshShaderMode t_shader_mode = MeshData::MeshShaderMode::LIGHTING);
	~MeshComponent();
	MeshComponent(const MeshComponent& other);

	void UpdateInstanceTransformBuffers();

	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);
	inline void SetColor(const glm::vec3& t_color) { color = t_color; }
	inline void SetInstanceGroup(MeshInstanceGroup* group) { m_instance_group = group; };

	inline MeshData::MeshShaderMode GetShaderMode() const { return m_shader_mode; }
	inline auto& GetMeshData() const { return m_mesh_data; }
	inline auto& GetInstanceGroup() const { return m_instance_group; };
	inline glm::vec3 GetColor() const { return color; };
	inline WorldTransform const* GetWorldTransform() const { return m_transform; };

private:
	MeshInstanceGroup* m_instance_group = nullptr;
	MeshData::MeshShaderMode m_shader_mode = MeshData::MeshShaderMode::LIGHTING;
	glm::vec3 color = glm::vec3(0.5f, 0.5f, 0.5f);
	MeshData* m_mesh_data = nullptr;
	WorldTransform* m_transform = nullptr;
};