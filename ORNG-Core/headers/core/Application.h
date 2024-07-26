#pragma once
#include "layers/LayerStack.h"

namespace ORNG {

	enum ApplicationModulesFlags : uint32_t {
		PHYSICS = 1 << 0,
		SCENE_RENDERER = 1 << 1,
		AUDIO = 1 << 2,
		INPUT = 1 << 3,
		ASSET_MANAGER = 1 << 4,
	};

	struct ApplicationData {
		// Only needed when building engine for runtime
		std::string shader_package_file;

		// Modules specified here will not be initialized with the application, this will make them unusable
		ApplicationModulesFlags disabled_modules;

		// Leave as -1 to prevent fullscreening
		int initial_window_display_monitor_idx = -1;
		glm::ivec2 initial_window_dimensions = { 2560, 1440 };
		const char* window_name = "ORNG ENGINE";
		bool window_decorated = true;
		bool window_iconified = true;
	};

	class Application
	{
	public:
		Application() = default;

		// Initializes all core engine modules and starts propagating events
		void Init(const ApplicationData& data);
		void Shutdown();

		LayerStack layer_stack;

	private:
		ApplicationData m_settings;
	};
}