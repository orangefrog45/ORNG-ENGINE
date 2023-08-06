#pragma once
#include "layers/LayerStack.h"

namespace ORNG {

	class Application
	{
	public:
		Application();
		~Application() = default;

		// Initializes all core engine modules and starts propagating events
		void Init();

		LayerStack layer_stack;
	};
}