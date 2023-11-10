#include "pch/pch.h"
#include "layers/LayerStack.h"
#include "util/Log.h"
#include "layers/ImGuiLayer.h"

namespace ORNG {
	void LayerStack::PopLayer(Layer* p_layer) {
		auto it = std::ranges::find(m_layers, p_layer);

		if (it == m_layers.end()) {
			ORNG_CORE_ERROR("Failed to pop layer, not found in stack");
			return;
		}

		m_layers.erase(it);
	}


	void LayerStack::UpdateLayers() {
		for (auto* p_layer : m_layers) {
			p_layer->Update();
		}
		m_imgui_layer.Update();
	}

	void LayerStack::OnRenderEvent() {
		for (auto* p_layer : m_layers) {
			p_layer->OnRender();
		}


		m_imgui_layer.BeginFrame();
		for (auto* p_layer : m_layers) {
			p_layer->OnImGuiRender();
		}
		m_imgui_layer.OnImGuiRender();
	}

	void LayerStack::Init() {
		m_core_listener.OnEvent = [this](const Events::EngineCoreEvent t_event) {
			if (t_event.event_type == Events::Event::ENGINE_UPDATE)
				UpdateLayers();
			else if (t_event.event_type == Events::Event::ENGINE_RENDER)
				OnRenderEvent();
			};

		std::ranges::for_each(m_layers, [](Layer* p_layer) {p_layer->OnInit(); });

		Events::EventManager::RegisterListener(m_core_listener);
	}
}