#include "pch/pch.h"

#include "framebuffers/Framebuffer.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "util/Log.h"
#include "util/util.h"
#include "rendering/Textures.h"


namespace ORNG {
	Framebuffer::Framebuffer(unsigned int id, const char* name, bool scale_with_window) : m_name(name), m_framebuffer_id(id), m_scales_with_window(scale_with_window) {
	}



	void Framebuffer::Resize() {
		for (auto& texture_entry : m_textures) {
			if (texture_entry.second.is_shared)
				return;

			if (texture_entry.second.p_texture->m_texture_target == GL_TEXTURE_2D) {
				Texture2D* p_texture = static_cast<Texture2D*>(texture_entry.second.p_texture);
				Texture2DSpec new_spec = p_texture->GetSpec();
				new_spec.width = (uint32_t)Window::GetWidth() * texture_entry.second.screen_size_ratio.x;
				new_spec.height = (uint32_t)Window::GetHeight() * texture_entry.second.screen_size_ratio.y;
				p_texture->SetSpec(new_spec);
			}
		}

		if (m_rbo == 0)
			return;

		Bind();
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Window::GetWidth() * m_renderbuffer_screen_size_ratio.x, Window::GetHeight() * m_renderbuffer_screen_size_ratio.y);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
	}

	Framebuffer::~Framebuffer() {
		glDeleteFramebuffers(1, &m_fbo);
		for (auto& tex : m_textures) {
			if (tex.second.is_shared)
				continue;

			delete tex.second.p_texture;
		}

		ORNG_CORE_INFO("Framebuffer '{0}' deleted", m_name);
	}

	const Texture2D& Framebuffer::Add2DTexture(const std::string& name, unsigned int attachment_point, const Texture2DSpec& spec)
	{
		if (m_textures.contains(name)) {
			ORNG_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use", name);
			BREAKPOINT;
		}

		Texture2D* tex = new Texture2D(name.c_str());

		if (!tex->ValidateBaseSpec(static_cast<const TextureBaseSpec*>(&spec), true)) {
			ORNG_CORE_ERROR("Failed adding 2D texture to framebuffer '{0}', invalid spec", m_name);
			BREAKPOINT;
		}

		unsigned int tex_handle = tex->GetTextureHandle();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, tex_handle, GL_TEXTURE0, true);
		tex->SetSpec(spec);
		m_textures[name] = Framebuffer::FramebufferTexture{ static_cast<TextureBase*>(tex), false };
		m_textures[name].screen_size_ratio = glm::vec2((float)spec.width / (float)Window::GetWidth(), (float)spec.height / (float)Window::GetHeight());


		Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, tex_handle, 0);

		return *tex;
	}


	void Framebuffer::AddShared2DTexture(const std::string& name, Texture2D& tex, GLenum attachment_point) {
		if (m_textures.contains(name)) {
			ORNG_CORE_ERROR("Framebuffer texture creation failed, name '{0}' already in use", name);
			BREAKPOINT;
		}

		m_textures[name] = Framebuffer::FramebufferTexture{ static_cast<TextureBase*>(&tex), true };
		const Texture2DSpec& spec = tex.GetSpec();
		m_textures[name].screen_size_ratio = glm::vec2(spec.width / Window::GetWidth(), spec.height / Window::GetHeight());

		Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, tex.GetTextureHandle(), 0);
	}


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

		m_renderbuffer_screen_size_ratio = glm::vec2(width / Window::GetWidth(), height / Window::GetHeight());
	}

	void Framebuffer::SetRenderBufferDimensions(unsigned int width, unsigned int height) {
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	}


	void Framebuffer::BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer) {
		Bind();
		glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, tex_ref, 0, layer);
	}


	void Framebuffer::BindTexture2D(unsigned int tex_ref, unsigned int attachment, unsigned int target, unsigned int mip_layer) {
		Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target, tex_ref, mip_layer);
	}



	void Framebuffer::Init() {
		glGenFramebuffers(1, &m_fbo);
		/*Bind();
		if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
			ORNG_CORE_ERROR("Framebuffer '{0}' initialization failed", m_name);
			BREAKPOINT;
		}
		else {
			ORNG_CORE_INFO("Framebuffer '{0}' initialized", m_name);
		}*/
	}




	void Framebuffer::Bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	}
}