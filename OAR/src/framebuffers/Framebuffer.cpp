#include "pch/pch.h"

#include "framebuffers/Framebuffer.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "util/Log.h"
#include "util/util.h"
#include "rendering/Textures.h"

namespace ORNG {

	Framebuffer::~Framebuffer() {
		glDeleteFramebuffers(1, &m_fbo);
		for (auto& tex : m_textures) {
			switch (tex.second->m_texture_target) {

			case GL_TEXTURE_2D:
				delete static_cast<Texture2D*>(tex.second);
				break;
			case GL_TEXTURE_2D_ARRAY:
				delete static_cast<Texture2DArray*>(tex.second);
				break;
			}
		}

		OAR_CORE_INFO("Framebuffer '{0}' deleted", m_name);
	}

	const Texture2D& Framebuffer::Add2DTexture(const std::string& name, unsigned int attachment_point, const Texture2DSpec& spec)
	{
		if (m_textures.contains(name)) {
			OAR_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use", name);
			BREAKPOINT;
		}

		if (!Texture2D::ValidateSpec(spec)) {
			OAR_CORE_ERROR("Failed adding 2D texture to framebuffer '{0}', invalid spec", m_name);
			BREAKPOINT;
		}


		Texture2D* tex = new Texture2D();
		unsigned int tex_handle = tex->GetTextureHandle();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, tex_handle, GL_TEXTURE0, true);
		tex->SetSpec(spec, true);
		m_textures[name] = static_cast<TextureBase*>(tex);


		Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, tex_handle, 0);

		return *tex;
	}



	const Texture2DArray& Framebuffer::Add2DTextureArray(const std::string& name, const Texture2DArraySpec& spec)
	{

		if (m_textures.contains(name)) {
			OAR_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use");
			BREAKPOINT;
		}

		if (!Texture2DArray::ValidateSpec(spec)) {
			OAR_CORE_ERROR("Failed adding 2D texture array to framebuffer '{0}', invalid spec", m_name);
			BREAKPOINT;
		}
		OAR_CORE_ERROR("ADD DESTRUCTORS TO FRAMEBUFFER TEXTURES");

		Texture2DArray* tex = new Texture2DArray();
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, tex->GetTextureHandle(), GL_TEXTURE0, true);
		tex->SetSpec(spec);
		m_textures[name] = static_cast<TextureBase*>(tex);


		return *tex;
	};

	void Framebuffer::EnableReadBuffer(unsigned int buffer) {
		glReadBuffer(buffer);
	}


	void Framebuffer::EnableDrawBuffers(unsigned int amount, unsigned int buffers[]) {
		glDrawBuffers(amount, buffers);
	}

	void Framebuffer::AddRenderbuffer() {
		Bind();
		glGenRenderbuffers(1, &m_rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Window::GetWidth(), Window::GetHeight());

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

	}


	void Framebuffer::BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer) {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, tex_ref, 0, layer);
	}



	void Framebuffer::Init() {
		glGenFramebuffers(1, &m_fbo);

		if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
			OAR_CORE_ERROR("Framebuffer '{0}' initialization failed", m_name);
			BREAKPOINT;
		}
		else {
			OAR_CORE_INFO("Framebuffer '{0}' initialized", m_name);
		}
	}




	void Framebuffer::Bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	}




}