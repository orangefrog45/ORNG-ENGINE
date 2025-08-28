#include "pch/pch.h"

#include "framebuffers/Framebuffer.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "util/Log.h"
#include "util/util.h"
#include "rendering/Textures.h"


namespace ORNG {
	Framebuffer::Framebuffer(unsigned int id, const char* name, bool scale_with_window) : m_framebuffer_id(id), m_name(name), m_scales_with_window(scale_with_window) {
	}

	void Framebuffer::Resize() {
		if (m_rbo == 0)
			return;

		Bind();
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast<int>(static_cast<float>(Window::GetWidth()) * m_renderbuffer_screen_size_ratio.x),
			static_cast<int>(static_cast<float>(Window::GetHeight()) * m_renderbuffer_screen_size_ratio.y));

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
	}

	Framebuffer::~Framebuffer() {
		glDeleteFramebuffers(1, &m_fbo);
	}


	void Framebuffer::EnableReadBuffer(unsigned int buffer) {
		glReadBuffer(buffer);
	}


	void Framebuffer::EnableDrawBuffers(unsigned int amount, unsigned int buffers[]) {
		glDrawBuffers(amount, buffers);
	}

	void Framebuffer::AddRenderbuffer(int width, int height) {
		Bind();
		glGenRenderbuffers(1, &m_rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

		m_renderbuffer_screen_size_ratio = glm::vec2(width / Window::GetWidth(), height / Window::GetHeight());
	}

	void Framebuffer::SetRenderBufferDimensions(int width, int height) {
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	}

	void Framebuffer::BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer) {
		Bind();
		glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, tex_ref, 0, layer);
	}

	void Framebuffer::BindTexture2D(unsigned int tex_ref, unsigned int attachment, unsigned int target, unsigned int mip_layer) {
		Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target, tex_ref, mip_layer);
	}


	void Framebuffer::Init() {
		glGenFramebuffers(1, &m_fbo);
		/*Bind();
		if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
			ORNG_CORE_ERROR("Framebuffer '{0}' initialization failed", m_name);
			BREAKPOINT;
		}
		else {
			ORNG_CORE_INFO("Framebuffer '{0}' initialized", m_name);
		}*/
	}

	void Framebuffer::Bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	}
}
