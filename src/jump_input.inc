// Included inside jump_hooks.cpp's private namespace.
using JumpBinding = JumpButton;
using JumpGamepadButtonFn = bool (*)(SDL_Gamepad*, int);
JumpGamepadButtonFn s_jumpGamepadButton = nullptr;

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
std::array<JumpInputState, 3> s_jumpInputs{};

JumpBinding active_jump_binding() {
    return jump_button();
}

bool jump_input_blocked() {
    bool visible = true;
    return !daAlink_getAlinkActorClass() || dComIfGp_isPauseFlag() ||
        !svc_ui || !svc_ui->is_any_document_visible ||
        svc_ui->is_any_document_visible(mod_ctx, &visible) != MOD_OK || visible;
}

bool jump_left_bumper_held() {
    constexpr int leftShoulder = 9; // SDL3 SDL_GAMEPAD_BUTTON_LEFT_SHOULDER (LB/L1).
    const s32 index = PADGetIndexForPort(PAD_1);
    auto* gamepad = index < 0 ? nullptr : PADGetSDLGamepadForIndex(static_cast<u32>(index));
    if (gamepad && s_jumpGamepadButton) return s_jumpGamepadButton(gamepad, leftShoulder);
    // Also permits Dawnlight's virtual LB when no physical controller is attached.
    return PADGetNativeButtonPressed(PAD_1) == leftShoulder;
}

void after_jump_pad_read(ModContext*, void*, void*, void*) {
    const auto& pad = JUTGamePad::mPadStatus[PAD_1];
    const bool connected = pad.err == PAD_ERR_NONE;
    const bool blocked = jump_input_blocked();
    const std::array<bool, 3> down{{
        connected && (pad.extButton & PAD_BUTTON_RIGHT_STICK) != 0,
        connected && (pad.extButton & PAD_BUTTON_LEFT_STICK) != 0,
        jump_left_bumper_held(),
    }};
    for (size_t i = 0; i < down.size(); ++i) s_jumpInputs[i].sample(down[i], blocked);
}

bool jump_pressed(JumpBinding binding) {
    if (touch_jump_pressed()) return true;
    if (binding == JumpBinding::R) return mDoCPd_c::getTrigLockR(PAD_1) != 0;
    const auto index = static_cast<int>(binding) - 3;
    return index >= 0 && index < static_cast<int>(s_jumpInputs.size()) && s_jumpInputs[index].pressed;
}

bool jump_held(JumpBinding binding) {
    if (touch_jump_held()) return true;
    if (binding == JumpBinding::R) return mDoCPd_c::getHoldLockR(PAD_1) != 0;
    const auto index = static_cast<int>(binding) - 3;
    return index >= 0 && index < static_cast<int>(s_jumpInputs.size()) && s_jumpInputs[index].held;
}

void initialize_jump_input() {
    s_jumpInputs = {};
    s_jumpGamepadButton = nullptr;
    void* address = nullptr;
    if (svc_hook && svc_hook->resolve &&
        svc_hook->resolve(mod_ctx, "SDL_GetGamepadButton", &address, nullptr) == MOD_OK)
        s_jumpGamepadButton = reinterpret_cast<JumpGamepadButtonFn>(address);
}
