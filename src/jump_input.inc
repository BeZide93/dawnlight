// Included inside jump_hooks.cpp's private namespace.
using JumpBinding = JumpButton;
using JumpGamepadAxisFn = s16 (*)(SDL_Gamepad*, int);
JumpGamepadAxisFn s_jumpGamepadAxis = nullptr;

struct JumpInputState {
    bool held = false, pressed = false, suppressed = false;
    void sample(bool down, bool blocked) {
        if (blocked && down) suppressed = true;
        if (!down) suppressed = false;
        down = down && !blocked && !suppressed;
        pressed = down && !held;
        held = down;
    }
};
std::array<JumpInputState, 4> s_jumpInputs{};

JumpBinding active_jump_binding() {
    return jump_button();
}

bool jump_input_blocked() {
    bool visible = true;
    return !daAlink_getAlinkActorClass() || dComIfGp_isPauseFlag() ||
        !svc_ui || !svc_ui->is_any_document_visible ||
        svc_ui->is_any_document_visible(mod_ctx, &visible) != MOD_OK || visible;
}

bool jump_trigger_held(bool left) {
    const s32 index = PADGetIndexForPort(PAD_1);
    auto* gamepad = index < 0 ? nullptr : PADGetSDLGamepadForIndex(static_cast<u32>(index));
    // SDL3 trigger axes: left=4, right=5. Resolve the function through the host
    // instead of adding an SDL library dependency to every platform build.
    if (gamepad && s_jumpGamepadAxis)
        return s_jumpGamepadAxis(gamepad, left ? 4 : 5) >= 16384;
    const auto* pad = JUTGamePad::getGamePad(PAD_1);
    // JUT's clamped analog triggers range from 0 to 150.
    return pad && (left ? pad->getAnalogL() : pad->getAnalogR()) >= 75;
}

void after_jump_pad_read(ModContext*, void*, void*, void*) {
    const auto& pad = JUTGamePad::mPadStatus[PAD_1];
    const bool connected = pad.err == PAD_ERR_NONE;
    const bool blocked = jump_input_blocked();
    const std::array<bool, 4> down{{
        connected && jump_trigger_held(false),
        connected && jump_trigger_held(true),
        connected && (pad.extButton & PAD_BUTTON_RIGHT_STICK) != 0,
        connected && (pad.extButton & PAD_BUTTON_LEFT_STICK) != 0,
    }};
    for (size_t i = 0; i < down.size(); ++i) s_jumpInputs[i].sample(down[i], blocked);
}

bool jump_pressed(JumpBinding binding) {
    if (binding == JumpBinding::R) return mDoCPd_c::getTrigLockR(PAD_1) != 0;
    const auto index = static_cast<int>(binding) - 1;
    return index >= 0 && index < static_cast<int>(s_jumpInputs.size()) && s_jumpInputs[index].pressed;
}

bool jump_held(JumpBinding binding) {
    if (binding == JumpBinding::R) return mDoCPd_c::getHoldLockR(PAD_1) != 0;
    const auto index = static_cast<int>(binding) - 1;
    return index >= 0 && index < static_cast<int>(s_jumpInputs.size()) && s_jumpInputs[index].held;
}

void initialize_jump_input() {
    s_jumpInputs = {};
    s_jumpGamepadAxis = nullptr;
    void* address = nullptr;
    if (svc_hook && svc_hook->resolve &&
        svc_hook->resolve(mod_ctx, "SDL_GetGamepadAxis", &address, nullptr) == MOD_OK)
        s_jumpGamepadAxis = reinterpret_cast<JumpGamepadAxisFn>(address);
}
