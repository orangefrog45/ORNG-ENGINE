#include "FramebufferLibrary.h"
#include "Log.h"

namespace FramebufferRef {
	const std::string dir = "DIRECTIONAL LIGHT DEPTH";
	const std::string spot = "SPOT LIGHT DEPTH";
	const std::string point = "POINT LIGHT DEPTH";
	const std::string deffered = "DEFERRED";
	const std::string main = "MAIN VIEW";
};


void FramebufferLibrary::Init() {
	std::string failed_framebuffer = "";
	using namespace FramebufferRef;

	if (!main_view_framebuffer.Init()) failed_framebuffer = main;
	if (!dir_depth_fb.Init()) failed_framebuffer = dir;
	if (!spotlight_depth_fb.Init()) failed_framebuffer = spot;
	if (!pointlight_depth_fb.Init()) failed_framebuffer = point;
	if (!deferred_fb.Init()) failed_framebuffer = deffered;

	if (failed_framebuffer != "") {
		OAR_CORE_CRITICAL("FRAMEBUFFER {0} FAILED TO BE GENERATED", failed_framebuffer);
		BREAKPOINT;
	}

}