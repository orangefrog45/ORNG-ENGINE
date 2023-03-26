#include "SpotLightDepthFB.h"
#include "GLErrorHandling.h"
#include "util/util.h"
#include <glew.h>
#include <glfw/glfw3.h>
#include "RendererData.h"

void SpotLightDepthFB::Init() {
	GLCall(glGenFramebuffers(1, &m_fbo));

	constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	/*Gen spotlight tex array*/
	GLCall(glGenTextures(1, &m_spot_depth_array));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, m_spot_depth_array));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, m_spot_light_shadow_width, m_spot_light_shadow_height, RendererData::max_spot_lights, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);


	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glReadBuffer(GL_NONE));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));


	if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		PrintUtils::PrintSuccess("SPOTLIGHT SHADOW FRAMEBUFFER GENERATED");
	}
	else {
		PrintUtils::PrintError("SPOTLIGHT SHADOW FRAMEBUFFER GENERATION FAILED");
		ASSERT(false);
	}
}

void SpotLightDepthFB::BindForDraw() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
	glViewport(0, 0, m_spot_light_shadow_width, m_spot_light_shadow_height);
}

void SpotLightDepthFB::SetDepthTexLayer(unsigned int layer) {
	GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_spot_depth_array, 0, layer));
}