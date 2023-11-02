#include "./includes/ScriptAPI.h"
#include <iostream>

/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/
using namespace ORNG;

const std::array<glm::vec3, 6> vecs = {
	glm::vec3{2, 0, 0},
	glm::vec3{-2, 0, 0},
	glm::vec3{0, 2, 0},
	glm::vec3{0, -2, 0},
	glm::vec3{0, 0, 2},
	glm::vec3{0, 0, -2}
};

glm::vec3 CSize = glm::vec3(1.0, 1.0, 1.3);

float map(glm::vec3 p1) {

	auto p2 = glm::vec3(p1.x, p1.z, p1.y);
	float scale = 1.f;

	// Move point towards limit set
	for (int i = 0; i < 12; i++)
	{
		p2 = 2.f * glm::clamp(p2, -CSize, CSize) - p2;
		float r2 = glm::dot(p2, p2);
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
		float k = glm::max((2.f) / (r2), 0.027f);
		p2 *= k;
		scale *= k;
	}

	float l = glm::length(glm::vec2(p2.x, p2.y));
	float rxy = l - 4.0;
	float n = l * p2.z;
	rxy = glm::max(rxy, -(n) / 4.f);
	return (rxy) / abs(scale);
};


void CalcMovement(ORNG::SceneEntity* p_entity, TransformComponent* p_transform, PhysicsComponent* p_phys_comp, DataComponent* p_data) {
	float delta_time = ScriptInterface::FrameTiming::GetDeltaTime();

	static glm::vec3 velocity_offset{ 0, 0,0 };
	velocity_offset -= velocity_offset * 20.f * 0.001f * delta_time;

	int count = 0;
	auto pos = p_transform->GetAbsoluteTransforms()[0];
	for (int i = 0; i < 6; i++) {
		if (map(pos + vecs[i]) < 0.01) {
			//p_transform->SetAbsolutePosition(pos - vecs[i] * 5.f);
			p_data->data["engineOn"] = false;
			p_phys_comp->AddForce(-vecs[i] * 1000.f);
			p_phys_comp->SetAngularVelocity(-vecs[i] * 1.f);
			velocity_offset -= vecs[i];
			count++;
		}
	}

	glm::vec3 ang = p_data->Get<glm::vec3>("angularVel");
	p_data->data["lookDir"] = glm::normalize(p_data->Get<glm::vec3>("lookDir") + ang * delta_time * 0.001f);

	p_data->data["angularVel"] = ang - ang * 0.001f * delta_time;

	glm::vec3 target = p_data->Get<glm::vec3>("lookDir");

	if (p_data->Get<bool>("engineOn"))
		p_phys_comp->SetVelocity((target + velocity_offset) * 30.f);
	else {
		return;
	}

	glm::vec3 angular = p_data->Get<glm::vec3>("angularVel");
	glm::vec3 up = glm::normalize(glm::vec3(0, 1, 0) + angular);


	p_transform->LookAt(pos + target, up);

	if (ScriptInterface::Input::IsKeyDown('b')) {
		p_transform->SetAbsolutePosition(pos + p_transform->up);
	}
	if (ScriptInterface::Input::IsKeyDown('w')) {
		angular += glm::vec3(0, 0.001, 0) * delta_time;
	}
	if (ScriptInterface::Input::IsKeyDown('s')) {
		angular -= glm::vec3(0, 0.001, 0) * delta_time;
	}
	if (ScriptInterface::Input::IsKeyDown('a')) {
		angular -= glm::normalize(glm::cross(p_transform->forward, glm::vec3(0, 1, 0))) * 0.002f * delta_time;
	}
	if (ScriptInterface::Input::IsKeyDown('d')) {
		angular += glm::normalize(glm::cross(p_transform->forward, glm::vec3(0, 1, 0))) * 0.002f * delta_time;
	}

	p_data->data["angularVel"] = angular;
}

void HandleGun(ORNG::SceneEntity* p_entity, TransformComponent* p_transform, PhysicsComponent* p_phys_comp, DataComponent* p_data) {
	p_data->data["gunCooldown"] = p_data->Get<float>("gunCooldown") - glm::min(p_data->Get<float>("gunCooldown"), ScriptInterface::FrameTiming::GetDeltaTime());
	if (ScriptInterface::Input::IsKeyDown('f') && p_data->Get<float>("gunCooldown") < 1) {
		p_data->data["gunCooldown"] = p_data->Get<float>("gunCooldown") + 100.f;
		auto& ent = ScriptInterface::Scene::InstantiatePrefab(ScriptInterface::Scene::Prefabs::GreenLaser);
		glm::vec3 new_pos = p_transform->GetAbsoluteTransforms()[0] + p_transform->forward * 5.f;
		p_entity->GetComponent<AudioComponent>()->Play();
		ent.GetComponent<TransformComponent>()->SetPosition(new_pos);
		ent.GetComponent<TransformComponent>()->LookAt(new_pos + p_transform->forward);
		p_data->data["startPos"] = new_pos;
	}
}



extern "C" {

	__declspec(dllexport) void OnCreate(ORNG::SceneEntity* p_entity) {
		//p_data->data["lookDir"] = glm::vec3(0, 0, -1.0);
		//p_data->data["angularVel"] = glm::vec3(0, 0, 0);
		//p_data->data["gunCooldown"] = 100.f;
		auto* p_phys_comp = p_entity->GetComponent<PhysicsComponent>();
		auto* p_data = p_entity->AddComponent<DataComponent>();
		glm::vec3 v{ 0,0,-1 };
		glm::vec3 v2{ 0,0,0 };
		float gc = 0;
		p_data->Push("lookDir", v);
		p_data->Push("angularVel", v2);
		p_data->Push("gunCooldown", gc);
		p_data->Push("engineOn", false);
		p_phys_comp->ToggleGravity(false);
	}





	__declspec(dllexport) void OnUpdate(ORNG::SceneEntity* p_entity) {
		auto* p_phys_comp = p_entity->GetComponent<PhysicsComponent>();
		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		auto* p_data = p_entity->GetComponent<DataComponent>();

		float dt = ScriptInterface::FrameTiming::GetElapsedTime();
		std::cout << dt << "\n";
		CSize = glm::vec3(0.9 + sinf(dt * 0.0001) * 0.1, 0.9 + cosf(dt * 0.0001) * 0.1, 1.3);

		CalcMovement(p_entity, p_transform, p_phys_comp, p_data);

		bool eo = p_data->Get<bool>("engineOn");
		if (ScriptInterface::Input::IsKeyPressed('b')) {
			p_data->data["engineOn"] = !eo;
		}


		HandleGun(p_entity, p_transform, p_phys_comp, p_data);
		
	}

	__declspec(dllexport) void OnDestroy(ORNG::SceneEntity* p_entity) {
	}

	__declspec(dllexport) void OnCollision(ORNG::SceneEntity* p_this, ORNG::SceneEntity* p_other) {
	}
}