#pragma once
#include "framebuffers/Framebuffer.h"
#include "util/util.h"

#if false
namespace ORNG {

	class FramebufferLibrary {
	public:
		FramebufferLibrary() = default;

		void Init();

		Framebuffer& CreateFramebuffer(const char* name, bool scale_with_window);

		Framebuffer& GetFramebuffer(const char* name);

		void DeleteFramebuffer(Framebuffer* p_framebuffer);

		void UnbindAllFramebuffers() { glBindFramebuffer(GL_FRAMEBUFFER, 0); };
	private:
		[[nodiscard]] unsigned int CreateFramebufferID() { return m_last_id++; };
		Events::EventListener<Events::WindowEvent> m_window_event_listener;
		unsigned int m_last_id = 0;
		std::unordered_map<std::string, Framebuffer> m_framebuffers;

	};
}
#endif
