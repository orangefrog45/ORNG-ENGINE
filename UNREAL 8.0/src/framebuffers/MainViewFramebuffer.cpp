#include "MainViewFramebuffer.h"
#include "GLErrorHandling.h"
#include "util/util.h"

void MainViewFramebuffer::Init() {
	GLCall(glGenFramebuffers(1, &m_fbo));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));

	GLCall(glGenTextures(1, &m_render_texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, m_render_texture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_window_width, m_window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	GLCall(glGenRenderbuffers(1, &m_rbo));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_rbo));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_window_width, m_window_height));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));

	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_render_texture, 0));
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo));

	if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		PrintUtils::PrintSuccess("FRAMEBUFFER CREATED");
	}
	else {
		PrintUtils::PrintError("FRAMEBUFFER FAILED TO BE CREATED");
		ASSERT(false); // cause breakpoint here
	}

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

MainViewFramebuffer::~MainViewFramebuffer() {
	GLCall(glDeleteFramebuffers(1, &m_fbo));
}

void MainViewFramebuffer::Bind() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
}

void MainViewFramebuffer::Unbind() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

