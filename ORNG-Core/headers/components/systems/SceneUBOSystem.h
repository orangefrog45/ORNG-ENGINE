#pragma once
#include "components/systems/ComponentSystem.h"
#include "rendering/VAO.h"

namespace ORNG {
	class SceneUBOSystem : public ComponentSystem {
	public:
		explicit SceneUBOSystem(Scene* p_scene) : ComponentSystem(p_scene) {};
		virtual ~SceneUBOSystem() = default;
		void OnLoad() override;
		void OnUpdate() override;
		void OnUnload() override;

		void UpdateVoxelAlignedPositions(const std::array<glm::vec3, 2>& positions);
		inline static constexpr uint64_t GetSystemUUID() { return 8289371920347454239; }

		void UpdateMatrixUBO(glm::mat4* p_proj = nullptr, glm::mat4* p_view = nullptr);

	private:
		void UpdateCommonUBO();
		void UpdateGlobalLightingUBO();

		UBO m_matrix_ubo{ true, 0 };
		inline const static unsigned int m_matrix_ubo_size = sizeof(glm::mat4) * 6;

		UBO m_global_lighting_ubo{ true, 0 };
		inline const static unsigned int m_global_lighting_ubo_size = 16 * sizeof(float);

		UBO m_common_ubo{ true, 0 };
		inline const static unsigned int m_common_ubo_size = sizeof(glm::vec4) * 8 + sizeof(float) * 6;
	};
}