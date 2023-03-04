#pragma once
#include "WorldTransform.h"
#include "MeshEntity.h"


class BaseLight {
public:
	BaseLight() = default;
	explicit BaseLight(const glm::fvec3& t_color, const float t_ambient_intensity) : color(t_color), ambient_intensity(t_ambient_intensity) {};
	glm::fvec3 color = glm::fvec3(1.0f, 1.0f, 1.0f);
	float ambient_intensity = 0.6f;
	float diffuse_intensity = 2.0f;
private:
};


struct LightAttenuation {
	float constant = 1.0f;
	float linear = 0.01f;
	float exp = 0.05f;
};

class PointLight : public BaseLight {
public:
	PointLight() = default;
	PointLight(const glm::fvec3& position, const glm::fvec3& t_color) { transform.SetPosition(position.x, position.y, position.z); color = t_color; };
	const LightAttenuation& GetAttentuation() const { return attenuation; }
	void SetAttenuation(float constant, float lin, float exp) { attenuation.constant = constant; attenuation.linear = lin; attenuation.exp = exp; }
	const WorldTransform& GetWorldTransform() const { return transform; };
	MeshEntity* cube_visual = nullptr;
	void SetPosition(float x, float y, float z) { if (cube_visual) { transform.SetPosition(x, y, z); cube_visual->SetPosition(x, y, z); } };
	void SetColor(float r, float g, float b) { color = glm::fvec3(r, g, b); }
	float ambient_intensity = 1.0f;
private:
	LightAttenuation attenuation;
	WorldTransform transform;
};



