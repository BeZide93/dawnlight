#pragma once

namespace dawnlight::shade {
// Simulation ticks, independent of rendering. Dialogue waits for the native
// message to close; bounded waits also release control if a message fails.
enum class Shot { None, Request, Arrival, Recover, Words1, Words2, Ready, BossName, Depart, Afterglow };
struct Cinema {
    Shot shot = Shot::None;
    int ticks = 0;
    bool victory = false;
    bool active() const { return shot != Shot::None; }
    void enter(Shot next) { shot=next; ticks=0; }
    void begin(bool won) { victory=won; enter(Shot::Request); }
    bool tick(bool event_accepted, bool message_done, bool pose_done) {
        if (!active()) return false;
        ++ticks;
        switch (shot) {
        case Shot::Request:
            if (event_accepted) enter(victory ? Shot::Recover : Shot::Arrival);
            else if (ticks>=180) enter(Shot::None);
            break;
        case Shot::Arrival: if (ticks>=54) enter(Shot::Words1); break;
        case Shot::Recover: if ((ticks>=45 && pose_done) || ticks>=120) enter(Shot::Words1); break;
        case Shot::Words1:
        case Shot::Words2:
            if (message_done || ticks>=300)
                enter(shot==Shot::Words1 ? Shot::Words2 : victory ? Shot::Depart : Shot::Ready);
            break;
        // Reveal the name as soon as the ready animation returns to its combat
        // idle, rather than adding a fixed pause after the stance is visible.
        case Shot::Ready: if (pose_done || ticks>=120) enter(Shot::BossName); break;
        case Shot::BossName: if (message_done || ticks>=300) enter(Shot::None); break;
        case Shot::Depart: if (ticks>=45) enter(Shot::Afterglow); break;
        case Shot::Afterglow: if (ticks>=30) enter(Shot::None); break;
        default: break;
        }
        return !active();
    }
};
}
