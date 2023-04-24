#include "pch/pch.h"

#include "framebuffers/FramebufferLibrary.h"
#include "util/Log.h"
#include "rendering/Renderer.h"

void FramebufferLibrary::Init() {

	/* DIRECTIONAL LIGHT DEPTH FB */
	Framebuffer& dir_depth = CreateFramebuffer("dir_depth");
	dir_depth.Init();
	dir_depth.Add2DTexture("depth", Renderer::GetWindowWidth(), Renderer::GetWindowHeight(), GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
	constexpr float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

	/* SPOTLIGHT DEPTH FB */
	Framebuffer& spot_depth = CreateFramebuffer("spotlight_depth");
	spot_depth.Init();
	spot_depth.Add2DTextureArray("depth", 1024, 1024, Renderer::max_spot_lights, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_color);
	UnbindAllFramebuffers();

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
		return m_framebuffers[name];
	}
}