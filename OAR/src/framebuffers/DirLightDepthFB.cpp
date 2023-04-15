#include "DirLightDepthFB.h"
#include "GLErrorHandling.h"
#include "util/util.h"
#include <glew.h>
#include <glfw/glfw3.h>
#include "Log.h"

bool DirLightDepthFB::Init()
{
	GLCall(glGenFramebuffers(1, &salt_m_fbo));

	constexpr float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

	/*Gen dir light tex*/
	GLCall(glGenTextures(1, &salt_dir_depth_tex));
	GLCall(glBindTexture(GL_TEXTURE_2D, salt_dir_depth_tex));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, salt_m_dir_light_shadow_width, salt_m_dir_light_shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, salt_dir_depth_tex, 0));
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

unsigned int DirLightDepthFB::GetDepthMap() { return salt_dir_depth_tex; }

void DirLightDepthFB::BindForDraw()
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, salt_m_fbo));
	glViewport(0, 0, salt_m_dir_light_shadow_width, salt_m_dir_light_shadow_height);
}
