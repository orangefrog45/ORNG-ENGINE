#pragma once
#include "VAO.h"

namespace ORNG {
	class Quad {
	public:
		friend class Renderer;
		void Load();
		// Vertices must be given in range -1, 1
		void SetVertices(glm::vec2 min, glm::vec2 max);

	private:
		std::unique_ptr<VAO> m_vao;
	};
}
