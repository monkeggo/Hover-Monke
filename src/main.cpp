#include <thread>
#include "modloader/shared/modloader.hpp"
#include "GorillaLocomotion/Player.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils-functions.h"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils-methods.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "GlobalNamespace/OVRInput.hpp"
#include "GlobalNamespace/OVRInput_Button.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/ForceMode.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Rigidbody.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/CapsuleCollider.hpp"
#include "UnityEngine/SphereCollider.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RaycastHit.hpp"
#include "UnityEngine/Physics.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/XR/InputDevice.hpp"
#include "monkecomputer/shared/GorillaUI.hpp"
#include "monkecomputer/shared/Register.hpp"
#include "custom-types/shared/register.hpp"
#include "config.hpp"
#include "HoverMonkeWatchView.hpp"

ModInfo modInfo;

#define INFO(value...) getLogger().info(value)
#define ERROR(value...) getLogger().error(value)

using namespace UnityEngine;
using namespace UnityEngine::XR;
using namespace GorillaLocomotion;

Logger& getLogger()
{
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

bool isRoom = false;
bool fist = false;
bool isFist = false;
float thrust = 0;

void powerCheck() {
    if(config.power == 0) {
        thrust = 0.4;
    }
    if(config.power == 1) {
        thrust = 0.7;
    }
    if(config.power == 2) {
        thrust = 0.9;
    }
    if(config.power == 3) {
        thrust = 1.2;
    }
    if(config.power == 4) {
        thrust = 1.4;
    }
}

MAKE_HOOK_OFFSETLESS(PhotonNetworkController_OnJoinedRoom, void, Il2CppObject* self) {
    INFO("Checking if private BUZZ");
    
    PhotonNetworkController_OnJoinedRoom(self);

    Il2CppObject* currentRoom = CRASH_UNLESS(il2cpp_utils::RunMethod("Photon.Pun", "PhotonNetwork", "get_CurrentRoom"));

    if (currentRoom)
    {
        isRoom = !CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(currentRoom, "get_IsVisible"));
    }
    else isRoom = true;

}

bool LStick = false;

void updateButton() {
    bool leftThumbstick = false;
    leftThumbstick = GlobalNamespace::OVRInput::Get(GlobalNamespace::OVRInput::Button::PrimaryThumbstick, GlobalNamespace::OVRInput::Controller::LTouch);

    if(leftThumbstick) {
        LStick = true;
    } else {
        LStick = false;
    }
}

#include "GlobalNamespace/GorillaTagManager.hpp"
#include "GlobalNamespace/OVRInput_Axis2D.hpp"
#include "GlobalNamespace/OVRInput_RawAxis2D.hpp"

MAKE_HOOK_OFFSETLESS(GorillaTagManager_Update, void, GlobalNamespace::GorillaTagManager* self) {

    using namespace GlobalNamespace;
    using namespace GorillaLocomotion;
    GorillaTagManager_Update(self);
    INFO("Running GTManager hook BUZZ");

    Player* playerInstance = Player::get_Instance();
    if(playerInstance == nullptr) return;
    GameObject* go = playerInstance->get_gameObject();
    auto* player = go->GetComponent<GorillaLocomotion::Player*>();

    Rigidbody* playerPhysics = playerInstance->playerRigidBody;
    if(playerPhysics == nullptr) return;

    GameObject* playerGameObject = playerPhysics->get_gameObject();
    if(playerGameObject == nullptr) return;

    Transform* turnParent = playerGameObject->get_transform()->GetChild(0);

    Transform* mainCamera = turnParent->GetChild(0);

    if(isRoom && config.enabled) {

        if(LStick) {
            playerPhysics->set_velocity(Vector3::get_zero());
            LStick = false;
            INFO("Brakes used");
        } 

        RaycastHit hit;

        UnityEngine::Transform* transform = playerGameObject->get_transform();

        float distance = 2.3f;

        Vector3 targetLocation;

        int layermask = 0b1 << 9;

        if(Physics::Raycast(transform->get_position() + Vector3::get_down().get_normalized() * 0.1f, Vector3::get_down(), hit, distance, layermask)) {
            Vector3 targetLocation = hit.get_point();

            float rayDistance = hit.get_distance();
                
            if(rayDistance < distance) {
                playerPhysics->AddForce(Vector3::get_up() * 500);
            }
            if(rayDistance > distance) {
                playerPhysics->AddForce(Vector3::get_down() * 2000);
            }
            if(rayDistance >= 7.5f) {
                playerPhysics->AddForce(Vector3::get_down() * 10000);
            }
        }

        powerCheck();

        Vector2 inputDir = OVRInput::Get(OVRInput::RawAxis2D::_get_LThumbstick(), OVRInput::Controller::LTouch);

        INFO("BUZZ inputDir x: " + std::to_string(inputDir.x));
        INFO("BUZZ inputDir y: " + std::to_string(inputDir.y));
       
        Vector3 velocityForward = mainCamera->get_forward() * (inputDir.y / 10);
        Vector3 velocitySideways = mainCamera->get_right() * (inputDir.x / 10);

        Vector3 newVelocityDir = (velocityForward + velocitySideways) * thrust;

        playerPhysics->set_velocity(playerPhysics->get_velocity() + newVelocityDir);

        INFO("In private room BUZZ");
    }
}

MAKE_HOOK_OFFSETLESS(Player_Update, void, Il2CppObject* self)
{
    using namespace UnityEngine;
    using namespace GlobalNamespace;
    INFO("player update was called");
    Player_Update(self);
}

extern "C" void setup(ModInfo& info)
{
    info.id = ID;
    info.version = VERSION;

    modInfo = info;
}

extern "C" void load()
{
    getLogger().info("Loading Phrog monke BUZZ...");

    GorillaUI::Init();

    INSTALL_HOOK_OFFSETLESS(getLogger(), PhotonNetworkController_OnJoinedRoom, il2cpp_utils::FindMethodUnsafe("", "PhotonNetworkController", "OnJoinedRoom", 0));
	INSTALL_HOOK_OFFSETLESS(getLogger(), Player_Update, il2cpp_utils::FindMethodUnsafe("GorillaLocomotion", "Player", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), GorillaTagManager_Update, il2cpp_utils::FindMethodUnsafe("", "GorillaTagManager", "Update", 0));

    custom_types::Register::RegisterType<HoverMonke::HoverMonkeWatchView>(); 
    GorillaUI::Register::RegisterWatchView<HoverMonke::HoverMonkeWatchView*>("<b><i><color=#FF0000>Ho</color><color=#FF8700>v</color><color=#FFFB00>er</color><color=#0FFF00>M</color><color=#0036FF>on</color><color=#B600FF>k</color><color=#FF00B6>e</color></i></b>", VERSION);

    LoadConfig();
    getLogger().info("Phrog monke loaded BUZZ!");
}