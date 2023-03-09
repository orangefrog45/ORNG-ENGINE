#pragma once
#include "WorldTransform.h"
#include "MeshEntity.h"


class BaseLight {
public:
	BaseLight() = default;
	explicit BaseLight(const glm::fvec3& t_color, const float t_ambient_intensity) : color(t_color), ambient_intensity(t_ambient_intensity) {};

	void SetColor(const float r, const float g, const float b) { color = glm::fvec3(r, g, b); }
	void SetAmbientIntensity(const float intensity) { ambient_intensity = intensity; }
	void SetDiffuseIntensity(const float intensity) { diffuse_intensity = intensity; }

	float GetAmbientIntensity() const { return ambient_intensity; }
	float GetDiffuseIntensity() const { return diffuse_intensity; }
	glm::fvec3 GetColor() const { return color; }

private:
	glm::fvec3 color = glm::fvec3(1.0f, 1.0f, 1.0f);
	float ambient_intensity = 0.2f;
	float diffuse_intensity = 1.0f;
};


struct LightAttenuation {
	float constant = 1.0f;
	float linear = 0.05f;
	float exp = 0.01f;
};


class PointLight : public BaseLight {
public:
	PointLight() = default;
	PointLight(const glm::fvec3& position, const glm::fvec3& t_color) { transform.SetPosition(position.x, position.y, position.z); SetColor(t_color.x, t_color.y, t_color.z); };

	const LightAttenuation& GetAttentuation() const { return attenuation; }
	const WorldTransform& GetWorldTransform() const { return transform; };
	const MeshEntity* GetMeshVisual() const { return mesh_visual; };
	float GetMaxDistance() { return max_distance; }

	void SetMeshVisual(MeshEntity* p) { mesh_visual = p; }
	void SetAttenuation(const float constant, const float lin, const float exp) { attenuation.constant = constant; attenuation.linear = lin; attenuation.exp = exp; }
	void SetPosition(const float x, const float y, const float z) { if (mesh_visual != nullptr) { transform.SetPosition(x, y, z); mesh_visual->SetPosition(x, y, z); } };
	void SetMaxDistance(const float d) { max_distance = d; };
protected:
	float max_distance = 48.0f;
	MeshEntity* mesh_visual = nullptr;
	LightAttenuation attenuation;
	WorldTransform transform;
};

class SpotLight : public PointLight {
public:
	SpotLight() = default;
	SpotLight(const glm::fvec3& dir_vec, const float t_aperture) : light_direction_vec(dir_vec), aperture(cosf(glm::radians(t_aperture))) { SetAttenuation(0.1f, 0.0f, 0.0f); };
	void SetLightDirection(float i, float j, float k) { light_direction_vec = glm::normalize(glm::fvec3(i, j, k)); mesh_visual->SetRotation(glm::degrees(sinf(j)) - 90.0f, -glm::degrees(atan2f(k, i)) - 90.0f, 0.0f); }
	void SetAperture(float angle) { aperture = cosf(glm::radians(angle)); }

	auto GetLightDirection() const { return light_direction_vec; }
	auto GetAperture() const { return aperture; }
private:
	glm::fvec3 light_direction_vec = glm::fvec3(1, 0, 0);
	float aperture = 0.9396f;
};


