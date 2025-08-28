#pragma once
#include "rendering/Textures.h"
#include "util/Log.h"
#include "events/Events.h"

namespace ORNG {

	class Framebuffer {
	public:
		friend class FramebufferLibrary;
		Framebuffer() = default;
		Framebuffer(unsigned int id, const char* name, bool scale_with_window);
		virtual ~Framebuffer();
		void Init();
		void Bind() const;

		void AddRenderbuffer(int width, int height);

		void BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer);

		void BindTexture2D(unsigned int tex_ref, unsigned int attachment, unsigned int target, unsigned int mip_layer = 0);

		void EnableReadBuffer(unsigned int buffer);

		void EnableDrawBuffers(unsigned int amount, unsigned int buffers[]);

		void SetRenderBufferDimensions(int width, int height);

		bool GetIsScalingWithWindow() const {
			return m_scales_with_window;
		}

		void Resize();
	private:
		GLuint m_framebuffer_id;
		const char* m_name = "Unnamed framebuffer";
		bool m_scales_with_window = false;

		GLuint m_fbo = 0;
		GLuint m_rbo = 0;
		glm::vec2 m_renderbuffer_screen_size_ratio = glm::vec2(0);
	};
}
