#include "ShadowMapFramebuffer.h"
#include "GLErrorHandling.h"
#include "util/util.h"
#include <glew.h>
#include <glfw/glfw3.h>

void ShadowMapFramebuffer::Init() {
	GLCall(glGenFramebuffers(1, &depth_map_fbo));

	//gen directional light texture
	GLCall(glGenTextures(1, &depth_map_texture));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, depth_map_texture));
	//2D TEXTURE ARRAY, 8 DEEP
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, m_shadow_width, m_shadow_height, 8, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);


	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo));
	GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_map_texture, 0, 0));
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));


	if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		PrintUtils::PrintSuccess("FRAMEBUFFER GENERATED");
	}
	else {
		PrintUtils::PrintError("SHADOW FRAMEBUFFER FAILED TO BE CREATED");
		ASSERT(false);
	}
}

void ShadowMapFramebuffer::SetTextureLayer(unsigned int layer) {
	GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_map_texture, 0, layer));
};


unsigned int ShadowMapFramebuffer::GetDepthMapTexture() { return depth_map_texture; }

void ShadowMapFramebuffer::BindForDraw() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo));
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m_shadow_width, m_shadow_height); // shadow width/height
}

void ShadowMapFramebuffer::Unbind() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}