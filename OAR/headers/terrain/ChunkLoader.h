#pragma once
#include "terrain/TerrainChunk.h"

namespace ORNG {

	class ChunkLoader {
	public:
		void SetChunkSettings(unsigned int seed, float height_scale, float sampling_density, unsigned int terrain_master_width);
		void RequestChunkLoad(TerrainChunk& chunk);
		void CancelChunkLoad(int key);
		void ProcessChunkLoads();
		void LoadChunk(TerrainChunk* chunk);
	private:
		[[nodiscard]] int CreateKey();
		int m_last_id = 0;
		std::map<int, TerrainChunk*> m_chunks_to_load;
		std::map<int, std::future<void>> m_futures;
		unsigned int m_seed;
		unsigned int m_active_processes = 0;
		unsigned int m_terrain_master_width = -1;
		float m_height_scale;
		float m_sampling_density;
	};

}