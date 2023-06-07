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

			if (tex.second.is_shared)
				return;


			switch (tex.second.p_texture->m_texture_target) {

			case GL_TEXTURE_2D:
				delete static_cast<Texture2D*>(tex.second.p_texture);
				break;
			case GL_TEXTURE_2D_ARRAY:
				delete static_cast<Texture2DArray*>(tex.second.p_texture);
				break;
			case GL_TEXTURE_CUBE_MAP:
				delete static_cast<TextureCubemap*>(tex.second.p_texture);
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



		Texture2D* tex = new Texture2D(name.c_str());

		if (!tex->ValidateSpec(spec)) {
			OAR_CORE_ERROR("Failed adding 2D texture to framebuffer '{0}', invalid spec", m_name);
			BREAKPOINT;
		}

		unsigned int tex_handle = tex->GetTextureHandle();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, tex_handle, GL_TEXTURE0, true);
		tex->SetSpec(spec, true);
		m_textures[name] = Framebuffer::FramebufferTexture{ static_cast<TextureBase*>(tex), false };


		Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, tex_handle, 0);

		return *tex;
	}


	void Framebuffer::AddShared2DTexture(const std::string& name, Texture2D& tex, GLenum attachment_point) {
		if (m_textures.contains(name)) {
			OAR_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use", name);
			BREAKPOINT;
		}

		m_textures[name] = Framebuffer::FramebufferTexture{ static_cast<TextureBase*>(&tex), true };
		Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, tex.GetTextureHandle(), 0);
	}


	const TextureCubemap& Framebuffer::AddCubemapTexture(const std::string& name, const TextureCubemapSpec& spec) {
		if (m_textures.contains(name)) {
			OAR_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use");
			BREAKPOINT;
		}


		TextureCubemap* tex = new TextureCubemap(name.c_str());

		if (!tex->ValidateSpec(spec)) {
			OAR_CORE_ERROR("Failed adding cubemap texture to framebuffer '{0}', invalid spec", m_name);
			BREAKPOINT;

		}
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, tex->GetTextureHandle(), GL_TEXTURE0, true);
		tex->SetSpec(spec);
		m_textures[name] = Framebuffer::FramebufferTexture{ static_cast<TextureBase*>(tex), false };


		return *tex;
	}

	const Texture2DArray& Framebuffer::Add2DTextureArray(const std::string& name, const Texture2DArraySpec& spec)
	{

		if (m_textures.contains(name)) {
			OAR_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use");
			BREAKPOINT;
		}

		Texture2DArray* tex = new Texture2DArray(name.c_str());

		if (!tex->ValidateSpec(spec)) {
			OAR_CORE_ERROR("Failed adding 2D texture array to framebuffer '{0}', invalid spec", m_name);
			BREAKPOINT;

		}
		GL_StateManager::BindTexture(GL_TEXTURE_2D_ARRAY, tex->GetTextureHandle(), GL_TEXTURE0, true);
		tex->SetSpec(spec);
		m_textures[name] = Framebuffer::FramebufferTexture{ static_cast<TextureBase*>(tex), false };


		return *tex;
	};

	void Framebuffer::EnableReadBuffer(unsigned int buffer) {
		glReadBuffer(buffer);
	}


	void Framebuffer::EnableDrawBuffers(unsigned int amount, unsigned int buffers[]) {
		glDrawBuffers(amount, buffers);
	}

	void Framebuffer::AddRenderbuffer(unsigned int width, unsigned int height) {
		Bind();
		glGenRenderbuffers(1, &m_rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

	}


	void Framebuffer::BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer) {
		Bind();
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