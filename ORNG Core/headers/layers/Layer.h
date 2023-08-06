#pragma once

namespace ORNG {
	class Layer {
	public:
		virtual void OnInit() = 0;
		virtual void Update() = 0;
		virtual void OnRender() = 0;
	};
}