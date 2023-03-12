#include "ShadowMapFramebuffer.h"
#include "GLErrorHandling.h"
#include "util/util.h"
#include <glew.h>
#include <glfw/glfw3.h>

void ShadowMapFramebuffer::Init() {
	GLCall(glGenFramebuffers(1, &depth_map_fbo));

	GLCall(glGenTextures(1, &depth_map_texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, depth_map_texture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, m_shadow_width, m_shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map_texture, 0));
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		PrintUtils::PrintSuccess("FRAMEBUFFER GENERATED");
	}
	else {
		PrintUtils::PrintError("FRAMEBUFFER FAILED TO BE CREATED");
		ASSERT(false);
	}
}

unsigned int ShadowMapFramebuffer::GetDepthMapTexture() { return depth_map_texture; }

void ShadowMapFramebuffer::BindForDraw() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_CW);
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo));
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m_shadow_width, m_shadow_height); // shadow width/height
}

void ShadowMapFramebuffer::Unbind() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}