#pragma once
#include "terrain/TerrainChunk.h"

namespace ORNG {

	class ChunkLoader {
	public:
		void RequestChunkLoad(TerrainChunk& chunk);
		void CancelChunkLoad(int key);
		void ProcessChunkLoads();
		void LoadChunk(TerrainChunk* chunk);
	private:
		[[nodiscard]] int CreateKey();
		int m_last_id = 0;
		std::map<int, TerrainChunk*> m_chunks_to_load;
		std::map<int, std::future<void>> m_futures;
		unsigned int m_active_processes = 0;
	};

}