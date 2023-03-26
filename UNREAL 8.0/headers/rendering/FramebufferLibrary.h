#pragma once
#include "MainViewFB.h"
#include "DirLightDepthFB.h"
#include "SpotLightDepthFB.h"
#include "PointLightDepthFB.h"
#include "util/util.h"
#include "RendererData.h"


struct FramebufferLibrary {
	FramebufferLibrary() = default;
	void Init() { main_view_framebuffer.Init(); dir_depth_fb.Init(); spotlight_depth_fb.Init(); pointlight_depth_fb.Init(); };
	void UnbindAllFramebuffers() { GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0)); };

	MainViewFB main_view_framebuffer = MainViewFB(RendererData::WINDOW_WIDTH, RendererData::WINDOW_HEIGHT);
	DirLightDepthFB dir_depth_fb;
	SpotLightDepthFB spotlight_depth_fb;
	PointLightDepthFB pointlight_depth_fb;

};