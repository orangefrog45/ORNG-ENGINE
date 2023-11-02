#include "../../ORNG-Core/headers/EngineAPI.h"

namespace ORNG {

	class GameLayer : public Layer {
		 void OnInit() override;
		 void Update() override;
		 void OnRender() override;
		 void OnShutdown() override;
		 void OnImGuiRender() override;
	};

}