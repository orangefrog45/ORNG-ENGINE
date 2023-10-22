#pragma once
#include "layers/Layer.h"
#include "events/EventManager.h"
#include "layers/ImGuiLayer.h"


namespace ORNG {
	class LayerStack {
		friend class Application;
	public:
		void PushLayer(Layer* p_layer) { m_layers.push_back(p_layer); }
		void PopLayer(Layer* p_layer);
		void Init();
		void Shutdown() {
			for (auto* p_layer : m_layers) {
				p_layer->OnShutdown();
			}
		}

	private:
		void UpdateLayers();
		void OnRenderEvent();

		ImGuiLayer m_imgui_layer;
		Events::EventListener<Events::EngineCoreEvent> m_core_listener;
		std::vector<Layer*> m_layers;
	};
}