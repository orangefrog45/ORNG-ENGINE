#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include "SceneEntity.h"
#include "WorldTransform.h"

class MeshComponent;


class BaseLight : public SceneEntity {
public:
	virtual ~BaseLight() = default;
	explicit BaseLight(unsigned int entity_id) : SceneEntity(entity_id) {};

	void SetColor(const float r, const float g, const float b) { color = glm::vec3(r, g, b); }
	void SetAmbientIntensity(const float intensity) { ambient_intensity = intensity; }
	void SetDiffuseIntensity(const float intensity) { diffuse_intensity = intensity; }

	float GetAmbientIntensity() const { return ambient_intensity; }
	float GetDiffuseIntensity() const { return diffuse_intensity; }
	glm::vec3 GetColor() const { return color; }

protected:
	bool shadows_enabled = true;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	float ambient_intensity = 0.2f;
	float diffuse_intensity = 1.0f;
};


class DirectionalLightComponent : public BaseLight {
public:
	DirectionalLightComponent(unsigned int entity_id);
	auto GetLightDirection() const { return light_direction; };
	void SetLightDirection(const glm::vec3& dir);
	const auto& GetTransformMatrix() const { return mp_light_transform_matrix; }
private:
	glm::mat4 mp_light_transform_matrix;
	glm::vec3 light_direction = glm::vec3(0.0f, 0.0f, -1.f);
};

struct LightAttenuation {
	float constant = 1.0f;
	float linear = 0.05f;
	float exp = 0.01f;
};


class PointLightComponent : public BaseLight {
public:
	PointLightComponent(unsigned int entity_id);
	virtual ~PointLightComponent() = default;

	const LightAttenuation& GetAttentuation() const { return attenuation; }
	const WorldTransform& GetWorldTransform() const { return transform; };
	const MeshComponent* GetMeshVisual() const { return m_mesh_visual; };
	float GetMaxDistance() const { return max_distance; }
	const std::vector<glm::mat4>& GetLightTransforms() const { return m_light_transforms; }

	void SetPosition(const float x, const float y, const float z);
	void SetMeshVisual(MeshComponent* p) { m_mesh_visual = p; }
	void SetAttenuation(const float constant, const float lin, const float exp) { attenuation.constant = constant; attenuation.linear = lin; attenuation.exp = exp; }
	void SetMaxDistance(const float d) { max_distance = d; };
protected:
	std::vector<glm::mat4> m_light_transforms;
	float max_distance = 48.0f;
	MeshComponent* m_mesh_visual = nullptr;
	LightAttenuation attenuation;
	WorldTransform transform;
private:
	void UpdateLightTransforms();
};

class SpotLightComponent : public PointLightComponent {
public:
	SpotLightComponent(unsigned int ID);
	void SetLightDirection(float i, float j, float k);
	void SetAperture(float angle) { aperture = cosf(glm::radians(angle)); }
	void SetPosition(const float x, const float y, const float z);

	auto GetLightDirection() const { return m_light_direction_vec; }
	auto GetAperture() const { return aperture; }
	const auto& GetTransformMatrix() const { return m_light_transforms[0]; }
private:
	void UpdateLightTransform();
	glm::vec3 m_light_direction_vec = glm::vec3(1, 0, 0);
	float aperture = 0.9396f;
};
