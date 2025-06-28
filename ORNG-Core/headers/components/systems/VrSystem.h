#pragma once

#include "components/systems/ComponentSystem.h"
#include "VR.h"

namespace ORNG {
    // Should be added to the scene before the scripting system if VR rendering is enabled

    class VrSystem : public ComponentSystem {
    public:
        explicit VrSystem(Scene* scene, vrlib::VR& vr) : ComponentSystem(scene), m_vr(vr) {
            SetTrackedActions = [this](std::string_view profile_path, std::vector<vrlib::VrInput::RequestedTrackedAction>& actions) {
                return m_vr.input.SetTrackedActions(profile_path, actions);
            };
        };

        virtual ~VrSystem() = default;

        // Resets use_matrices flag
        void OnUpdate() final { m_use_matrices = false;  };

        // Sets matrices that will be used in the common scene UBO, should be updated each frame if rendering in VR
        // If none are provided, the normal camera matrices will be used for both eyes, which will look very wrong
        void SetMatrices(unsigned eye_idx, const glm::mat4& view, const glm::mat4& proj) {
            m_matrices[eye_idx].first = view;
            m_matrices[eye_idx].second = proj;
            m_use_matrices = true;
        }

        [[nodiscard]] bool ShouldUseMatrices() const noexcept {
            return m_use_matrices;
        }

        [[nodiscard]] vrlib::VR& GetVrInstance() const noexcept {
            return m_vr;
        }

        [[nodiscard]] const std::pair<glm::mat4, glm::mat4>& GetEyeMatrices(unsigned eye_idx) const noexcept {
            ASSERT(eye_idx < 2);
            return m_matrices[eye_idx];
        }

        // A function pointer has to be used here to prevent OpenXR loader bugs across DLL boundaries
        std::function<bool(std::string_view, std::vector<vrlib::VrInput::RequestedTrackedAction>&)> SetTrackedActions = nullptr;

        inline static constexpr uint64_t GetSystemUUID() { return 2528367418286; }
    private:
        vrlib::VR& m_vr;

        bool m_use_matrices = false;
        // pair.first = view matrix
        // pair.second = projection matrix
        // [0] = left eye
        // [1] = right eye
        std::array<std::pair<glm::mat4, glm::mat4>, 2> m_matrices;
    };
}