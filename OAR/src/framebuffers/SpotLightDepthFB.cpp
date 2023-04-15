#include "SpotLightDepthFB.h"
#include "GLErrorHandling.h"
#include "util/util.h"
#include <glew.h>
#include <glfw/glfw3.h>
#include "RendererResources.h"

bool SpotLightDepthFB::Init()
{
	GLCall(glGenFramebuffers(1, &salt_m_fbo));

	constexpr float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

	/*Gen spotlight tex array*/
	GLCall(glGenTextures(1, &salt_m_spot_depth_array));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, salt_m_spot_depth_array));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, salt_m_spot_light_shadow_width, salt_m_spot_light_shadow_height, RendererResources::max_spot_lights, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo));
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glReadBuffer(GL_NONE));
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

void SpotLightDepthFB::BindForDraw()
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo));
	glViewport(0, 0, salt_m_spot_light_shadow_width, salt_m_spot_light_shadow_height);
}

void SpotLightDepthFB::SetDepthTexLayer(unsigned int layer)
{
	GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, salt_m_spot_depth_array, 0, layer));
}