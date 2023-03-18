#pragma once
#include "MainViewFramebuffer.h"
#include "ShadowMapFramebuffer.h"
#include "util/util.h"
#include "RendererData.h"


struct FramebufferLibrary {
	FramebufferLibrary() = default;
	void Init() { main_view_framebuffer.Init(); shadow_map_framebuffer.Init(); };

	MainViewFramebuffer main_view_framebuffer = MainViewFramebuffer(RendererData::WINDOW_WIDTH, RendererData::WINDOW_HEIGHT);
	ShadowMapFramebuffer shadow_map_framebuffer;

};