#include "survivors_title_logo.hpp"
#include "d/actor/d_a_title.h"

namespace dawnlight {
void finish_survivors_title_logo(void* actor) {
    auto* title = static_cast<daTitle_c*>(actor);
    if (!title) return;
    // Use the native final frames and PRESS START setup, without its extra
    // 30-update delay. keyWait runs on the next Execute, not on this input.
    title->fastLogoDispInit();
    title->mWaitTimer = 0;
    title->fastLogoDisp();
}
}
