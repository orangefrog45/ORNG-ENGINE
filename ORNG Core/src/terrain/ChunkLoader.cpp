#include "pch/pch.h"

#include "terrain/ChunkLoader.h"
#include "util/Log.h"
#include "util/util.h"

namespace ORNG {


	int ChunkLoader::CreateKey() {
		m_last_id++;
		return m_last_id;
	}

	void ChunkLoader::RequestChunkLoad(TerrainChunk& chunk) {
		int key = CreateKey();
		m_chunks_to_load[key] = &chunk;
		chunk.m_chunk_key = key;
	}

	void ChunkLoader::CancelChunkLoad(int key) {
		m_chunks_to_load.erase(key);
		m_futures.erase(key);
	}

	void ChunkLoader::LoadChunk(TerrainChunk* chunk) {
		chunk->is_loading = true;
		TerrainGenerator::GenNoiseChunk(chunk->m_seed, chunk->m_width, chunk->m_resolution, chunk->m_height_scale, chunk->m_bot_left_coord, chunk->m_vao.vertex_data, chunk->m_bounding_box);
		chunk->m_data_is_loaded = true;
		chunk->is_loading = false;
		this->m_active_processes--;
	}

	void ChunkLoader::ProcessChunkLoads() {
		for (auto it = m_chunks_to_load.begin(); it != m_chunks_to_load.end();) {
			TerrainChunk* chunk = it->second;

			if (m_active_processes < 2 && !chunk->m_data_is_loaded && !chunk->is_loading) {
				this->m_active_processes++;
				m_futures[chunk->m_chunk_key] = (std::async(std::launch::async, [this, chunk] {LoadChunk(chunk); }));
			}

			if (chunk->m_data_is_loaded && !chunk->is_loading) {
				chunk->LoadGLData();
				m_futures.erase(chunk->m_chunk_key);
				it = m_chunks_to_load.erase(it);
			}
			else {
				it++;
			}
		}

	}




}