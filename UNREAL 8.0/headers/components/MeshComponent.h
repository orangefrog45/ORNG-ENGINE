#pragma once
#include "MeshData.h"
#include "WorldTransform.h"
#include "MeshInstanceGroup.h"
#include "SceneEntity.h"

class MeshComponent : public SceneEntity {
public:
	explicit MeshComponent(MeshData* t_mesh_data, MeshInstanceGroup* instance_group, unsigned int entity_id, MeshData::MeshShaderMode t_shader_mode = MeshData::MeshShaderMode::LIGHTING) :
		SceneEntity(entity_id), m_mesh_data(t_mesh_data), m_instance_group(instance_group), m_shader_mode(t_shader_mode) {};
	MeshComponent(const MeshComponent& other);
	~MeshComponent() { delete m_transform; }

	void UpdateInstanceTransformBuffers();

	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);
	inline void SetColor(const glm::fvec3& t_color) { color = t_color; }
	inline void SetInstanceGroup(MeshInstanceGroup* group) { m_instance_group = group; };

	inline MeshData::MeshShaderMode GetShaderMode() const { return m_shader_mode; }
	inline auto& GetMeshData() const { return m_mesh_data; }
	inline auto& GetInstanceGroup() const { return m_instance_group; };
	inline glm::fvec3 GetColor() const { return color; };
	inline WorldTransform const* GetWorldTransform() const { return m_transform; };

private:
	MeshInstanceGroup* m_instance_group = nullptr;
	MeshData::MeshShaderMode m_shader_mode = MeshData::MeshShaderMode::LIGHTING;
	glm::fvec3 color = glm::fvec3(0.5f, 0.5f, 0.5f);
	MeshData* m_mesh_data = nullptr;
	/* Memory at pointer freed upon deletion of meshcomponent via Scene class. */
	WorldTransform* m_transform = new WorldTransform();
};