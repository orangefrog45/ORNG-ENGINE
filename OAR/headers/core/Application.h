#pragma once
#include "layers/EditorLayer.h"

namespace ORNG {

	class Application
	{
	public:
		Application();
		~Application() = default;
		void Init();
		void RenderScene();

	private:
		EditorLayer editor_layer = EditorLayer();
	};
}