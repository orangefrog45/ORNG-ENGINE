#include "PointLightDepthFB.h"
#include "GLErrorHandling.h"
#include "util/util.h"
#include <glew.h>
#include <glfw/glfw3.h>
#include "RendererData.h"


void PointLightDepthFB::Init() {
	GLCall(glGenFramebuffers(1, &m_fbo));


	GLCall(glGenTextures(1, &m_depth_map_texture_array));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_depth_map_texture_array));
	GLCall(glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_R32F, m_depth_tex_width, m_depth_tex_height, RendererData::max_point_lights * 6));
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);


	GLCall(glGenRenderbuffers(1, &m_rbo));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_rbo));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_depth_tex_width, m_depth_tex_height));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo));

	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	if (GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		PrintUtils::PrintSuccess("POINTLIGHT SHADOW FRAMEBUFFER GENERATED");
	}
	else {
		PrintUtils::PrintError("POINTLIGHT SHADOW FRAMEBUFFER GENERATION FAILED");
		ASSERT(false);
	}
}

unsigned int PointLightDepthFB::GetDepthMap() { return m_depth_map_texture_array; }

void PointLightDepthFB::SetCubemapFace(int layer, int face) {
	GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_depth_map_texture_array, 0, (layer * 6) + face));
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}


void PointLightDepthFB::BindForDraw() {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
	glViewport(0, 0, m_depth_tex_width, m_depth_tex_height);
}