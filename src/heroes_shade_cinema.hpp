#pragma once

namespace dawnlight::shade {
// All pacing uses simulation ticks (30 Hz), never render calls.
inline constexpr int wolf_pan_ticks = 60;
inline constexpr int flash_rise_ticks = 12;
inline constexpr int flash_hold_ticks = 6;
inline constexpr int flash_fall_ticks = 18;
inline constexpr int flash_total_ticks = flash_rise_ticks + flash_hold_ticks + flash_fall_ticks;
inline constexpr int wolf_hold_ticks = 30;
enum class Shot { None, Request, Arrival, Recover, Words1, Words2, Ready, BossName, Depart, Afterglow };
struct Cinema {
    Shot shot = Shot::None;
    int ticks = 0;
    bool victory = false;
    bool active() const { return shot != Shot::None; }
    void enter(Shot next) { shot=next; ticks=0; }
    void begin(bool won) { victory=won; enter(Shot::Request); }
    bool tick(bool event_accepted, bool message_done, bool pose_done, bool visual_done=false) {
        if (!active()) return false;
        ++ticks;
        switch (shot) {
        case Shot::Request:
            if (event_accepted) enter(victory ? Shot::Recover : Shot::Arrival);
            else if (ticks>=180) enter(Shot::None);
            break;
        case Shot::Arrival: if (visual_done || ticks>=600) enter(Shot::Words1); break;
        case Shot::Recover: if ((ticks>=45 && pose_done) || ticks>=120) enter(Shot::Words1); break;
        case Shot::Words1:
        case Shot::Words2:
            if (message_done || ticks>=300)
                enter(shot==Shot::Words1 ? Shot::Words2 : victory ? Shot::Depart : Shot::Ready);
            break;
        case Shot::Ready: if (pose_done || ticks>=120) enter(Shot::BossName); break;
        case Shot::BossName: if (message_done || ticks>=300) enter(Shot::None); break;
        case Shot::Depart: if (visual_done || ticks>=600) enter(Shot::Afterglow); break;
        case Shot::Afterglow: if (ticks>=30) enter(Shot::None); break;
        default: break;
        }
        return !active();
    }
};
}
