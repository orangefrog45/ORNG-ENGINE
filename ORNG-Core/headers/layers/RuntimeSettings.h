#pragma once

struct RuntimeSettings {
    bool use_vr = false;
    uint64_t start_scene_uuid = 0;

    template<typename S>
    void serialize(S& s) {
        s.value1b(use_vr);
        s.value8b(start_scene_uuid);
    }

    template<typename S>
    void deserialize(S& s) {
        s.value1b(use_vr);
        s.value8b(start_scene_uuid);
    }
};