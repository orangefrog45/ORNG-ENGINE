#pragma once
#include "components/systems/ComponentSystem.h"
#include "rendering/VAO.h"

namespace ORNG {
	class PointlightSystem : public ComponentSystem {
	public:
		PointlightSystem(Scene* p_scene) : ComponentSystem(p_scene) {};
		~PointlightSystem() = default;
		void OnLoad() override;
		void OnUpdate() override;
		void OnUnload() override;
		void WriteLightToVector(std::vector<float>& output_vec, PointLightComponent& light, int& index);

		// Checks if the depth map array needs to grow/shrink
		void OnDepthMapUpdate();

		static constexpr unsigned POINTLIGHT_SHADOW_MAP_RES = 2048;

		inline static constexpr uint64_t GetSystemUUID() { return 920384564563898; }

		TextureCubemapArray& GetDepthTex() {
			return m_pointlight_depth_tex;
		}

	private:
		TextureCubemapArray m_pointlight_depth_tex{ "Pointlight depth" }; // Used for shadow maps
		SSBO<float> m_pointlight_ssbo{ true, 0 };
	};
}