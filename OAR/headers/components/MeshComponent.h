#pragma once
#include "rendering/MeshAsset.h"
#include "components/Component.h"

namespace ORNG {

	class MeshInstanceGroup;
	class WorldTransform;

	class MeshComponent : public Component {
	public:
		friend class Scene;
		friend class Renderer;
		friend class EditorLayer;
		friend class MeshInstanceGroup;
		friend class SceneRenderer;
		explicit MeshComponent(unsigned long entity_id);
		~MeshComponent();
		MeshComponent(const MeshComponent& other) = delete;


		void SetPosition(const float x, const float y, const float z);
		void SetOrientation(const float x, const float y, const float z);
		void SetScale(const float x, const float y, const float z);

		void SetPosition(const glm::vec3 transform);
		void SetOrientation(const glm::vec3 transform);
		void SetScale(const glm::vec3 transform);

		void SetShaderID(unsigned int id);
		void SetMaterialID(unsigned int index, unsigned int id);
		void SetMeshAsset(MeshAsset* p_asset);
		inline void SetColor(const glm::vec3 t_color) { m_color = t_color; }

		inline int GetShaderMode() const { return m_shader_id; }
		inline const MeshAsset* GetMeshData() const { return mp_mesh_asset; }
		inline glm::vec3 GetColor() const { return m_color; };
		inline const WorldTransform* GetWorldTransform() const { return mp_transform; };

	private:
		void RequestTransformSSBOUpdate();

		//ID's of materials associated with each submesh of the mesh asset
		std::vector<unsigned int> m_material_ids;

		unsigned int m_shader_id = 1; // 1 = lighting (default shader)
		bool m_transform_update_flag = false;

		glm::vec3 m_color = glm::vec3(1.f, 1.f, 1.f);
		MeshInstanceGroup* mp_instance_group = nullptr;
		MeshAsset* mp_mesh_asset = nullptr;
		WorldTransform* mp_transform = nullptr;
	};

}