#pragma once
#include "MeshData.h"
#include "WorldTransform.h"
#include "RendererData.h"
#include "MeshInstanceGroup.h"
#include "SceneEntity.h"

class MeshComponent : public SceneEntity {
public:
	explicit MeshComponent(MeshData* t_mesh_data, MeshInstanceGroup* instance_group, unsigned int entity_id, MeshShaderMode t_shader_mode = MeshShaderMode::LIGHTING) :
		SceneEntity(entity_id), m_mesh_data(t_mesh_data), m_instance_group(instance_group), m_shader_mode(t_shader_mode) {};
	MeshComponent(const MeshComponent& other);

	void UpdateInstanceTransformBuffers();

	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);
	void SetColor(const glm::fvec3& t_color) { color = t_color; }
	void SetInstanceGroup(MeshInstanceGroup& group) { m_instance_group = &group; };

	MeshShaderMode GetShaderMode() const { return m_shader_mode; }
	auto& GetMeshData() const { return m_mesh_data; }
	auto& GetInstanceGroup() const { return m_instance_group; };
	glm::fvec3 GetColor() const { return color; };
	WorldTransform const* GetWorldTransform() const { return m_transform; };

private:
	MeshInstanceGroup* m_instance_group = nullptr;
	MeshShaderMode m_shader_mode = MeshShaderMode::LIGHTING;
	glm::fvec3 color = glm::fvec3(0.5f, 0.5f, 0.5f);
	MeshData* m_mesh_data = nullptr;
	/* Memory at pointer freed upon deletion of meshcomponent via Scene class. */
	WorldTransform* m_transform = new WorldTransform();
};