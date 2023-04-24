#pragma once
#include "rendering/MeshAsset.h"
#include "components/MeshComponent.h"

class WorldTransform;
class Scene;

class MeshInstanceGroup {
public:
	friend class EditorLayer;
	friend class Renderer;
	friend class MeshComponent;
	friend class Scene;
	MeshInstanceGroup(MeshAsset* t_mesh_data, unsigned int shader_id, Scene* scene) : m_mesh_asset(t_mesh_data), m_group_shader_id(shader_id), m_scene(scene) {};
	void AddMeshPtr(MeshComponent* ptr);
	void DeleteMeshPtr(const MeshComponent* ptr);
	unsigned int FindMeshPtrIndex(const MeshComponent* ptr);

	auto GetMeshData() const { return m_mesh_asset; }
	auto GetShaderType() const { return m_group_shader_id; }
private:
	/* Update world matrix for the mesh component belonging to entity_id */
	void SubUpdateWorldMatBuffer(const MeshComponent* ptr);
	/* Update entire transform buffer with current mesh transforms */
	void UpdateWorldMatBuffer(); // needs to be called upon m_meshes having entities placed/removed

	std::vector< MeshComponent*> m_meshes; // kept in ascending order (entity id)
	Scene* m_scene = nullptr;
	MeshAsset* m_mesh_asset;
	unsigned int m_group_shader_id;
};
