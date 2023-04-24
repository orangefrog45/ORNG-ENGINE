#include "pch/pch.h"

#include "framebuffers/Framebuffer.h"
#include "util/Log.h"
#include "util/util.h"
#include "rendering/Renderer.h"


Framebuffer::~Framebuffer() {
	glDeleteFramebuffers(1, &m_fbo);
	for (auto& it : m_textures) {
		glDeleteTextures(1, &it.second.tex_ref);
	}
	OAR_CORE_INFO("Framebuffer '{0}' deleted", m_name);
}

Framebuffer::FB_TextureObject& Framebuffer::Add2DTexture(const std::string& name, unsigned int width, unsigned int height, unsigned int attachment_point, unsigned int internal_format,
	unsigned int format, unsigned int gl_type)
{
	if (m_textures.contains(name)) {
		OAR_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use");
		BREAKPOINT;
	}
	Bind();


	m_textures[name] = FB_TextureObject(GL_TEXTURE_2D, 0, name.c_str());
	FB_TextureObject& tex = m_textures[name];

	GLCall(glGenTextures(1, &tex.tex_ref));
	GLCall(glBindTexture(GL_TEXTURE_2D, tex.tex_ref));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, gl_type, nullptr));

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, tex.tex_ref, 0));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return tex;
}



Framebuffer::FB_TextureObject& Framebuffer::Add2DTextureArray(const std::string& name, unsigned int width, unsigned int height, unsigned int layers, unsigned int internal_format,
	unsigned int format, unsigned int gl_type)
{

	if (m_textures.contains(name)) {
		OAR_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use");
		BREAKPOINT;
	}


	Bind();

	m_textures[name] = FB_TextureObject(GL_TEXTURE_2D_ARRAY, 0, name.c_str());
	FB_TextureObject& tex = m_textures[name];

	GLCall(glGenTextures(1, &tex.tex_ref));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, tex.tex_ref));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, width, height, layers, 0, format, gl_type, nullptr));

	return tex;
};

void Framebuffer::EnableReadBuffer(unsigned int buffer) {
	glReadBuffer(buffer);
}


void Framebuffer::EnableDrawBuffers(unsigned int amount, unsigned int buffers[]) {
	glDrawBuffers(amount, buffers);
}

void Framebuffer::AddRenderbuffer() {
	Bind();
	GLCall(glGenRenderbuffers(1, &m_rbo));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_rbo));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Renderer::GetWindowWidth(), Renderer::GetWindowHeight()));

	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));

}


void Framebuffer::BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer) {
	Bind();
	GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, tex_ref, 0, layer));
}

void Framebuffer::Init() {
	GLCall(glGenFramebuffers(1, &m_fbo));

	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		OAR_CORE_ERROR("Framebuffer '{0}' initialization failed", m_name);
		BREAKPOINT;
	}
	else {
		OAR_CORE_INFO("Framebuffer '{0}' initialized", m_name);
	}
}

void Framebuffer::Bind() const {
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
}

Framebuffer::FB_TextureObject& Framebuffer::GetTexture(const std::string& name) {
	if (!m_textures.contains(name)) {
		OAR_CORE_ERROR("No texture with name '{0}' found in framebuffer '{1}'", name, m_name);
		BREAKPOINT;
	}

	return m_textures[name];
}