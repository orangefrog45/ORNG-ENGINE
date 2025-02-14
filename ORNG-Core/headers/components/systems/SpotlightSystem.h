#pragma once
#include "components/systems/ComponentSystem.h"

namespace ORNG {
	class SpotlightSystem : public ComponentSystem {
		friend class SceneRenderer;
	public:
		SpotlightSystem(Scene* p_scene) : ComponentSystem(p_scene) {};
		virtual ~SpotlightSystem() = default;

		void OnLoad() override;
		void OnUpdate() override;
		void OnUnload() override;

		inline static constexpr uint64_t GetSystemUUID() { return 2834836378357356; }

		inline static constexpr unsigned SPOTLIGHT_SHADOW_MAP_RES = 1024;

		Texture2DArray& GetDepthTex() {
			return m_spotlight_depth_tex;
		}
	private:
		void WriteLightToVector(std::vector<float>& output_vec, SpotLightComponent& light, int& index);
		Texture2DArray m_spotlight_depth_tex{"Spotlight depth"}; // Used for shadow maps
		SSBO<float> m_spotlight_ssbo{ true, 0 };
	};
}