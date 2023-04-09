#pragma once
#include <string>
#include "TextureBase.h"


class Texture2D : public TextureBase {
public:
	explicit Texture2D(const std::string& filename);

	void Load() override;

};