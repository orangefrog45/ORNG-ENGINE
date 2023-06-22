#pragma once
#include "rendering/MeshAsset.h"
#include "components/Component.h"
#include "components/TransformComponent.h"

namespace ORNG {

	class MeshInstanceGroup;
	class TransformComponent;

	class MeshComponent : public Component {
	public:
		friend class MeshComponentManager;
		friend class Scene;
		friend class EditorLayer;
		friend class MeshInstanceGroup;
		friend class SceneRenderer;
		explicit MeshComponent(SceneEntity* p_entity);
		MeshComponent(const MeshComponent& other) = delete;

		void SetShaderID(unsigned int id);
		void SetMaterialID(unsigned int index, const Material* p_material);
		void SetMeshAsset(MeshAsset* p_asset);
		inline void SetColor(const glm::vec3 t_color) { m_color = t_color; }

		inline int GetShaderMode() const { return m_shader_id; }
		inline const MeshAsset* GetMeshData() const { return mp_mesh_asset; }
		inline glm::vec3 GetColor() const { return m_color; };

		TransformComponent transform;
	private:
		void RequestTransformSSBOUpdate();

		std::vector<const Material*> m_materials;

		unsigned int m_shader_id = 1; // 1 = lighting (default shader)
		bool m_transform_update_flag = false;

		glm::vec3 m_color = glm::vec3(1.f, 1.f, 1.f);
		MeshInstanceGroup* mp_instance_group = nullptr;
		MeshAsset* mp_mesh_asset = nullptr;
	};

}