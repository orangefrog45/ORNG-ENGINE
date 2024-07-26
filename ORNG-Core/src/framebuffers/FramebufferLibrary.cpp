#include "pch/pch.h"

#include "framebuffers/FramebufferLibrary.h"
#include "util/Log.h"
#include "events/EventManager.h"

namespace ORNG {

	void FramebufferLibrary::Init() {
		m_window_event_listener.OnEvent = [this](const Events::WindowEvent& t_event) {
			if (t_event.event_type == Events::Event::WINDOW_RESIZE) {
				for (auto& k_v : m_framebuffers) {
					if (k_v.second.GetIsScalingWithWindow())
						k_v.second.Resize();
				}
			}
		};

		Events::EventManager::RegisterListener(m_window_event_listener);

	}

	Framebuffer& FramebufferLibrary::GetFramebuffer(const char* name) {
		if (!m_framebuffers.contains(name)) {
			ORNG_CORE_CRITICAL("Framebuffer '{0}' not found", name);
			BREAKPOINT;
		}

		return m_framebuffers[name];
	}

	Framebuffer& FramebufferLibrary::CreateFramebuffer(const char* name, bool scale_with_window) {
		if (m_framebuffers.contains(name)) {
			ORNG_CORE_CRITICAL("Framebuffer '{0}' cannot be created, name already in use by another framebuffer", name);
			BREAKPOINT;
		}
		else {
			m_framebuffers.try_emplace(name, CreateFramebufferID(), name, scale_with_window);
			m_framebuffers[name].Init();
			return m_framebuffers[name];
		}
	}

	void FramebufferLibrary::DeleteFramebuffer(Framebuffer* p_framebuffer) {
		if (m_framebuffers.contains(p_framebuffer->m_name))
			m_framebuffers.erase(p_framebuffer->m_name);
		else
			ORNG_CORE_ERROR("Failed to delete framebuffer '{0}', not found in FramebufferLibrary", p_framebuffer->m_name);
	}
}