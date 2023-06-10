#include "pch/pch.h"

#include "framebuffers/FramebufferLibrary.h"
#include "util/Log.h"
#include "core/Window.h"

namespace ORNG {

	void FramebufferLibrary::Init() {

	}

	Framebuffer& FramebufferLibrary::GetFramebuffer(const char* name) {
		if (!m_framebuffers.contains(name)) {
			OAR_CORE_CRITICAL("Framebuffer '{0}' not found", name);
			BREAKPOINT;
		}

		return m_framebuffers[name];
	}

	Framebuffer& FramebufferLibrary::CreateFramebuffer(const char* name, bool scale_with_window) {
		if (m_framebuffers.contains(name)) {
			OAR_CORE_CRITICAL("Framebuffer '{0}' cannot be created, name already in use by another framebuffer", name);
			BREAKPOINT;
		}
		else {
			m_framebuffers.try_emplace(name, CreateFramebufferID(), name, scale_with_window);
			m_framebuffers[name].Init();
			return m_framebuffers[name];
		}
	}
}