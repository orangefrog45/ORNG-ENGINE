#pragma once
namespace ScriptInterface {
namespace Scene {
namespace Entities {
constexpr uint64_t ShipBase = 16143535647637499458;
constexpr uint64_t ShipTop = 13972840778305969908;
constexpr uint64_t New_entity = 13519866667247840905;
constexpr uint64_t New_entity_1 = 10715185434853614626;
constexpr uint64_t New_entity_1_1 = 615399000224840637;
constexpr uint64_t New_entity_1_1_1 = 7005335994085481975;
constexpr uint64_t New_entity_1_1_1_1 = 1487024142417982154;
constexpr uint64_t New_entity_1_1_1_1_1 = 2747256969409050269;
};namespace Prefabs {
inline static const std::string OrangeDebris = R"(Entity: 16332162998874037088
Name: OrangeDebris
ParentID: 0
TransformComp:
  Pos: [-2.19347548, 0, 0]
  Scale: [0.156127512, 0.183673441, 0.206035957]
  Orientation: [0, 0, 0]
  Absolute: false
MeshComp:
  MeshAssetID: 0
  Materials: [1610759369916266989]
PhysicsComp:
  RigidBodyType: 1
  GeometryType: 0
ScriptComp:
  ScriptPath: res\scripts\4593265525889349944.cpp)"; 
inline static const std::string GreenLaser = R"(Entity: 13658198146195192145
Name: GreenLaser
ParentID: 0
TransformComp:
  Pos: [0, 0, 0]
  Scale: [0.156127512, 0.183673441, 1.85257745]
  Orientation: [0, 0, 0]
  Absolute: false
MeshComp:
  MeshAssetID: 0
  Materials: [10857381234196093435]
PhysicsComp:
  RigidBodyType: 1
  GeometryType: 0
ScriptComp:
  ScriptPath: res\scripts\14766806766574730398.cpp)"; 
};namespace Sounds {
inline static const uint64_t mouse_click = 1; 
inline static const uint64_t blaster_2_81267_mp3 = 8425462682793348983; 
};};};