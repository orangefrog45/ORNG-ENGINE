#include "DeferredFB.h"
#include "glew.h"
#include "glfw/glfw3.h"
#include "util/util.h"
#include "Log.h"
#include "RendererResources.h"

bool DeferredFB::Init()
{
	GLCall(glGenFramebuffers(1, &salt_m_fbo));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo));

	GLCall(glGenTextures(1, &salt_m_geometry_texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, salt_m_geometry_texture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight(), 0, GL_RGBA, GL_FLOAT, NULL));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, salt_m_geometry_texture, 0);

	GLCall(glGenRenderbuffers(1, &salt_m_rbo));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, salt_m_rbo));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight()));

	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, salt_m_rbo));

	if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		return true;
	}
	else
	{
		return false;
	}

	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredFB::BindForDraw()
{
	glViewport(0, 0, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight());
	glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void DeferredFB::BindGeometryTexture()
{
	glViewport(0, 0, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight());
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, salt_m_geometry_texture, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}
