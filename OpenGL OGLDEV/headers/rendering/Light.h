#pragma once
#include "WorldTransform.h"

class BaseLight {
public:
	glm::fvec3 color = glm::fvec3(1.0f, 1.0f, 1.0f);
	float ambient_intensity = 0.5f;
	BaseLight() = default;
	explicit BaseLight(const glm::fvec3& t_color, const float t_ambient_intensity) : color(t_color), ambient_intensity(t_ambient_intensity) {};
};

class PointLight : public BaseLight {
public:
	PointLight() = default;
	PointLight(const glm::fvec3& position, const glm::fvec3& t_color) : color(t_color) { transform.SetPosition(position.x, position.y, position.z); };
	glm::fvec3 color = glm::fvec3(1.0f, 1.0f, 1.0f);
	WorldTransform transform;
};


