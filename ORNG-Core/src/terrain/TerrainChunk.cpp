#include "pch/pch.h"

#include "terrain/TerrainChunk.h"

namespace ORNG {

	void TerrainChunk::LoadGLData() {
		m_vao.FillBuffers();
		m_is_initialized = true;
	}
}