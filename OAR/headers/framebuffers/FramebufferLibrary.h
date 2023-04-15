#pragma once
#include <string>
#include "MainViewFB.h"
#include "DirLightDepthFB.h"
#include "SpotLightDepthFB.h"
#include "PointLightDepthFB.h"
#include "DeferredFB.h"
#include "util/util.h"
#include "RendererResources.h"

struct FramebufferLibrary
{
	FramebufferLibrary() = default;
	void Init();
	void UnbindAllFramebuffers() { GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0)); };

	MainViewFB main_view_framebuffer;
	DirLightDepthFB salt_dir_depth_fb;
	SpotLightDepthFB salt_spotlight_depth_fb;
	PointLightDepthFB salt_pointlight_depth_fb;
	DeferredFB salt_deferred_fb;
};