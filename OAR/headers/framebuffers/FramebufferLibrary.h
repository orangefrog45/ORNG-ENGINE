#pragma once
#include "framebuffers/Framebuffer.h"
#include "util/util.h"


class FramebufferLibrary {
public:
	FramebufferLibrary() = default;
	void Init();
	Framebuffer& CreateFramebuffer(const char* name);
	Framebuffer& GetFramebuffer(const char* name);
	void UnbindAllFramebuffers() { GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0)); };

private:
	[[nodiscard]] unsigned int CreateFramebufferID() { return m_last_id++; };
	unsigned int m_last_id = 0;
	std::unordered_map<std::string, Framebuffer> m_framebuffers;

};