// Native Darknut cues (d_a_b_tn.cpp); audio stays in the player's game archives.
constexpr u32 kShadeIntroMusic = 0x2000037;
constexpr u32 kNoMusic = 0xffffffff;
struct ShadeMusic {
    bool active = false;
    u32 roomMain = kNoMusic;
    u32 roomStream = kNoMusic;
} sShadeMusic;

void start_shade_music(bool intro) {
    auto* audio=Z2GetAudioMgr();
    if (!audio) return;
    if (!sShadeMusic.active) {
        sShadeMusic={true,audio->getMainBgmID(),audio->getStreamBgmID()};
    }
    if (intro) {
        audio->bgmStreamPrepare(kShadeIntroMusic);
        audio->bgmStreamPlay();
    } else {
        if (audio->getStreamBgmID()==kShadeIntroMusic) audio->bgmStreamStop(30);
        // Uses the native sub-sequence channel, preserving the room sequence.
        audio->subBgmStart(Z2BGM_TN_MBOSS);
    }
}

void stop_shade_music() {
    if (!sShadeMusic.active) return;
    const auto saved=sShadeMusic;
    sShadeMusic={};
    auto* audio=Z2GetAudioMgr();
    if (!audio) return;
    if (audio->getStreamBgmID()==kShadeIntroMusic) audio->bgmStreamStop(30);
    if (audio->getSubBgmID()==Z2BGM_TN_MBOSS) {
        audio->subBgmStop();
        // Darknut normally delays room music by 510 ticks after victory.
        // Finish that handoff now so the room music returns during our outro.
        audio->subBgmStopInner();
    }
    auto* player=daAlink_getAlinkActorClass();
    if (saved.roomStream!=kNoMusic && saved.roomStream!=kShadeIntroMusic &&
        arena() && !dComIfGp_isEnableNextStage() && player && !player->checkDeadHP() &&
        audio->getMainBgmID()==saved.roomMain && audio->getStreamBgmID()==kNoMusic &&
        audio->getSubBgmID()==kNoMusic) {
        audio->bgmStreamPrepare(saved.roomStream);
        audio->bgmStreamPlay();
    }
    // A sequence room BGM was only ducked, never stopped. Native sub/stream
    // teardown restores its volume. Never restart old room audio over game-over,
    // a replacement cue or the destination scene during departure.
}
