#pragma once
#include "layers/LayerStack.h"

namespace ORNG {

	class Application
	{
	public:
		Application() = default;

		// Initializes all core engine modules and starts propagating events
		void Init();
		void Shutdown();

		LayerStack layer_stack;
	};
}