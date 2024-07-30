#pragma once

namespace ORNG {
	struct Layer {
		virtual void OnInit() = 0;
		virtual void Update() = 0;
		virtual void OnRender() = 0;
		virtual void OnShutdown() = 0;
		virtual void OnImGuiRender()= 0;

		virtual void PostImGuiRender() {};
	};
}