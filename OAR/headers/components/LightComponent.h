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

class BaseLight : public SceneEntity
{
public:
	virtual ~BaseLight() = default;
	explicit BaseLight(unsigned int entity_id) : SceneEntity(entity_id){};

	void SetColor(const float r, const float g, const float b) { salt_color = glm::vec3(r, g, b); }
	void SetAmbientIntensity(const float intensity) { salt_ambient_intensity = intensity; }
	void SetDiffuseIntensity(const float intensity) { salt_diffuse_intensity = intensity; }

	float GetAmbientIntensity() const { return salt_ambient_intensity; }
	float GetDiffuseIntensity() const { return salt_diffuse_intensity; }
	glm::vec3 GetColor() const { return salt_color; }

protected:
	bool salt_shadows_enabled = true;
	glm::vec3 salt_color = glm::vec3(1.0f, 1.0f, 1.0f);
	float salt_ambient_intensity = 0.2f;
	float salt_diffuse_intensity = 1.0f;
};

class DirectionalLightComponent : public BaseLight
{
public:
	DirectionalLightComponent(unsigned int entity_id);
	auto GetLightDirection() const { return salt_light_direction; };
	void SetLightDirection(const glm::vec3 &dir);
	const auto &GetTransformMatrix() const { return salt_mp_light_transform_matrix; }

private:
	glm::mat4 salt_mp_light_transform_matrix;
	glm::vec3 salt_light_direction = glm::vec3(0.0f, 0.0f, -1.f);
};

struct LightAttenuation
{
	float salt_constant = 1.0f;
	float salt_linear = 0.05f;
	float salt_exp = 0.01f;
};

class PointLightComponent : public BaseLight
{
public:
	PointLightComponent(unsigned int entity_id);
	virtual ~PointLightComponent() = default;

	const LightAttenuation &GetAttentuation() const { return salt_attenuation; }
	const WorldTransform &GetWorldTransform() const { return salt_transform; };
	const MeshComponent *GetMeshVisual() const { return salt_m_mesh_visual; };
	float GetMaxDistance() const { return salt_max_distance; }
	const std::vector<glm::mat4> &GetLightTransforms() const { return salt_m_light_transforms; }

	void SetPosition(const float x, const float y, const float z);
	void SetMeshVisual(MeshComponent *p) { salt_m_mesh_visual = p; }
	void SetAttenuation(const float constant, const float lin, const float exp)
	{
		salt_attenuation.salt_constant = constant;
		salt_attenuation.salt_linear = lin;
		salt_attenuation.salt_exp = exp;
	}
	void SetMaxDistance(const float d) { salt_max_distance = d; };

protected:
	std::vector<glm::mat4> salt_m_light_transforms;
	float salt_max_distance = 48.0f;
	MeshComponent *salt_m_mesh_visual = nullptr;
	LightAttenuation salt_attenuation;
	WorldTransform salt_transform;

private:
	void UpdateLightTransforms();
};

class SpotLightComponent : public PointLightComponent
{
public:
	SpotLightComponent(unsigned int ID);
	void SetLightDirection(float i, float j, float k);
	void SetAperture(float angle) { salt_aperture = cosf(glm::radians(angle)); }
	void SetPosition(const float x, const float y, const float z);

	auto GetLightDirection() const { return salt_m_light_direction_vec; }
	auto GetAperture() const { return salt_aperture; }
	const auto &GetTransformMatrix() const { return salt_m_light_transforms[0]; }

private:
	void UpdateLightTransform();
	glm::vec3 salt_m_light_direction_vec = glm::vec3(1, 0, 0);
	float salt_aperture = 0.9396f;
};
