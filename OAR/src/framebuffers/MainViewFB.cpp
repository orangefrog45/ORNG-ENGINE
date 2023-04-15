#include "MainViewFB.h"
#include "GLErrorHandling.h"
#include "util/util.h"
#include "RendererResources.h"
#include "Log.h"

bool MainViewFB::Init()
{
	GLCall(glGenFramebuffers(1, &salt_m_fbo));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo));

	GLCall(glGenTextures(1, &salt_m_render_texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, salt_m_render_texture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	GLCall(glGenTextures(1, &salt_m_depth_texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, salt_m_depth_texture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, salt_m_render_texture, 0));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, salt_m_depth_texture, 0));

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		return true;
	}
	else
	{
		return false;
	}
}

MainViewFB::~MainViewFB()
{
	GLCall(glDeleteFramebuffers(1, &salt_m_fbo));
}

void MainViewFB::Bind()
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo));
}
