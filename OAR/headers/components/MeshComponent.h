#pragma once
#include "rendering/MeshAsset.h"
#include "components/Component.h"

class MeshInstanceGroup;
class WorldTransform;

class MeshComponent : public Component {
public:
	friend class Scene;
	friend class Renderer;
	friend class EditorLayer;
	explicit MeshComponent(unsigned long entity_id);
	~MeshComponent();
	MeshComponent(const MeshComponent& other) = delete;

	void UpdateInstanceTransformBuffers();

	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);

	void SetPosition(const glm::vec3 transform);
	void SetRotation(const glm::vec3 transform);
	void SetScale(const glm::vec3 transform);

	void SetShaderID(unsigned int id);
	inline void SetColor(const glm::vec3 t_color) { m_color = t_color; }

	inline int GetShaderMode() const { return m_shader_id; }
	inline const MeshAsset* GetMeshData() const { return mp_mesh_asset; }
	inline glm::vec3 GetColor() const { return m_color; };
	inline const WorldTransform* GetWorldTransform() const { return mp_transform; };

private:
	unsigned int m_shader_id = 0; // 0 = lighting (default shader)
	glm::vec3 m_color = glm::vec3(1.f, 1.f, 1.f);
	MeshInstanceGroup* mp_instance_group = nullptr;
	MeshAsset* mp_mesh_asset = nullptr;
	WorldTransform* mp_transform = nullptr;
};