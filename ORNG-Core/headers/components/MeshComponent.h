#ifndef MESHCOMPONENT_H
#define MESHCOMPONENT_H
#include "components/Component.h"

namespace ORNG {

	class MeshInstanceGroup;
	class MeshAsset;
	class Material;

	class MeshComponent : public Component {
	public:
		friend class MeshInstancingSystem;
		friend class Scene;
		friend class EditorLayer;
		friend class MeshInstanceGroup;
		friend class SceneRenderer;
		friend class SceneSerializer;

		explicit MeshComponent(SceneEntity* p_entity);
		MeshComponent(SceneEntity* p_entity, MeshAsset* p_asset);
		MeshComponent(SceneEntity* p_entity, MeshAsset* p_asset, std::vector<const Material*>&& materials);
		MeshComponent(const MeshComponent& other) = delete;

		void SetMaterialID(unsigned int index, const Material* p_material);
		void SetMeshAsset(MeshAsset* p_asset);

		inline  MeshAsset* GetMeshData() { return mp_mesh_asset; }
		auto& GetMaterials() { return m_materials; }


	private:
		void DispatchUpdateEvent();
		std::vector<const Material*> m_materials;

		MeshInstanceGroup* mp_instance_group = nullptr;
		MeshAsset* mp_mesh_asset = nullptr;
	};

}
#endif