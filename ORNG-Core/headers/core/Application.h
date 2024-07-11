#pragma once
#include "layers/LayerStack.h"

namespace ORNG {

	struct ApplicationData {
		// Only needed when building engine for runtime
		std::string shader_package_file;
	};

	class Application
	{
	public:
		Application() = default;

		// Initializes all core engine modules and starts propagating events
		void Init(const ApplicationData& data);
		void Shutdown();

		LayerStack layer_stack;
	};
}