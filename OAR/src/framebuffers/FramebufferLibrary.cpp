#include "pch/pch.h"

#include "framebuffers/FramebufferLibrary.h"
#include "util/Log.h"
#include "core/Window.h"

namespace ORNG {

	void FramebufferLibrary::Init() {

		/* LIGHTING FB */
		Texture2DSpec lighting_spec;
		lighting_spec.format = GL_RGB;
		lighting_spec.internal_format = GL_RGB16;
		lighting_spec.storage_type = GL_UNSIGNED_BYTE;
		lighting_spec.width = Window::GetWidth();
		lighting_spec.height = Window::GetHeight();

		Framebuffer& lighting_fb = CreateFramebuffer("lighting");
		lighting_fb.AddRenderbuffer();
		lighting_fb.Add2DTexture("render_texture", GL_COLOR_ATTACHMENT0, lighting_spec);

	}

	Framebuffer& FramebufferLibrary::GetFramebuffer(const char* name) {
		if (!m_framebuffers.contains(name)) {
			OAR_CORE_CRITICAL("Framebuffer '{0}' not found", name);
			BREAKPOINT;
		}

		return m_framebuffers[name];
	}

	Framebuffer& FramebufferLibrary::CreateFramebuffer(const char* name) {
		if (m_framebuffers.contains(name)) {
			OAR_CORE_CRITICAL("Framebuffer '{0}' cannot be created, name already in use by another framebuffer", name);
			BREAKPOINT;
		}
		else {
			m_framebuffers.try_emplace(name, CreateFramebufferID(), name);
			m_framebuffers[name].Init();
			return m_framebuffers[name];
		}
	}
}