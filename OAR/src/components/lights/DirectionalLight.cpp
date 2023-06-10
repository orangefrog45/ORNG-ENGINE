#include "pch/pch.h"
#include "components/lights/DirectionalLight.h"

namespace ORNG {

	DirectionalLight::DirectionalLight() : BaseLight(0) {
		diffuse_intensity = 1.f;
		color = glm::vec3(0.922f, 0.985f, 0.875f) * 5.f;

	}
}