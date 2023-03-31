#pragma once
#include "SceneEntity.h"
#include "WorldTransform.h"
#include "MeshComponent.h"


class BaseLight : public SceneEntity {
public:
	virtual ~BaseLight() = default;
	explicit BaseLight(unsigned int entity_id) : SceneEntity(entity_id) {};

	void SetColor(const float r, const float g, const float b) { color = glm::fvec3(r, g, b); }
	void SetAmbientIntensity(const float intensity) { ambient_intensity = intensity; }
	void SetDiffuseIntensity(const float intensity) { diffuse_intensity = intensity; }

	float GetAmbientIntensity() const { return ambient_intensity; }
	float GetDiffuseIntensity() const { return diffuse_intensity; }
	glm::fvec3 GetColor() const { return color; }

protected:
	bool shadows_enabled = true;
	glm::fvec3 color = glm::fvec3(1.0f, 1.0f, 1.0f);
	float ambient_intensity = 0.2f;
	float diffuse_intensity = 1.0f;
};


class DirectionalLightComponent : public BaseLight {
public:
	DirectionalLightComponent(unsigned int entity_id);
	auto GetLightDirection() const { return light_direction; };
	void SetLightDirection(const glm::fvec3& dir, const glm::fvec3& camera_pos);
	const auto& GetTransformMatrix() const { return mp_light_transform_matrix; }
private:
	glm::fmat4 mp_light_transform_matrix;
	glm::fvec3 light_direction = glm::fvec3(0.0f, 0.0f, -1.f);
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
	const std::vector<glm::fmat4>& GetLightTransforms() const { return m_light_transforms; }

	void SetPosition(const float x, const float y, const float z);
	void SetMeshVisual(MeshComponent* p) { m_mesh_visual = p; }
	void SetAttenuation(const float constant, const float lin, const float exp) { attenuation.constant = constant; attenuation.linear = lin; attenuation.exp = exp; }
	void SetMaxDistance(const float d) { max_distance = d; };
protected:
	std::vector<glm::fmat4> m_light_transforms;
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
	glm::fvec3 m_light_direction_vec = glm::fvec3(1, 0, 0);
	float aperture = 0.9396f;
};
