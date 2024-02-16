#include "Component.h"
#include "physics/vehicles/DirectDrive.h"


namespace ORNG {
	class MeshAsset;
	class Material;

	class VehicleComponent : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class SceneSerializer;
	public:
		VehicleComponent(SceneEntity* p_entity) : Component(p_entity) { };
		void SetThrottle(float t) { m_vehicle.mCommandState.throttle = t; };
		void SetSteer(float t) { m_vehicle.mCommandState.steer = t; };
		void SetHandBrake(float t) { m_vehicle.mCommandState.nbBrakes = 2;  m_vehicle.mCommandState.brakes[1] = t; };
		void SetBrake(float t) { m_vehicle.mCommandState.nbBrakes = 2;  m_vehicle.mCommandState.brakes[0] = t; };

		glm::vec3 wheel_scale{ 1,1,1, };
		glm::vec3 body_scale{ 1,1,1 };
		DirectDriveVehicle m_vehicle{};
	private:

		// TODO: Once skeletal meshes are supported, use the mesh component in the entity
		MeshAsset* p_body_mesh = nullptr;
		MeshAsset* p_wheel_mesh = nullptr;

		std::vector<Material*> m_body_materials;
		std::vector<Material*> m_wheel_materials;
	};
}