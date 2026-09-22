// Native Darknut cues (d_a_b_tn.cpp); audio stays in the player's game archives.
constexpr u32 kShadeIntroMusic = 0x2000037;
constexpr u32 kNoMusic = 0xffffffff;
struct ShadeMusic {
    bool active = false;
    u32 roomMain = kNoMusic;
    u32 roomStream = kNoMusic;
    bool introPending = false;
    bool battleStarted = false;
} sShadeMusic;

void start_shade_music(bool intro) {
    auto* audio=Z2GetAudioMgr();
    if (!audio) return;
    if (!sShadeMusic.active) {
        sShadeMusic={true,audio->getMainBgmID(),audio->getStreamBgmID()};
    }
    if (intro) {
        sShadeMusic.introPending=true;
        sShadeMusic.battleStarted=false;
        audio->bgmStreamPrepare(kShadeIntroMusic);
        audio->bgmStreamPlay();
    } else {
        sShadeMusic.introPending=false;
        if (sShadeMusic.battleStarted) return;
        sShadeMusic.battleStarted=true;
        // Native TN_MBOSS performs the stream stop and starts its sub-sequence
        // in one handoff. Do not release the stream with a separate fade first.
        audio->subBgmStart(Z2BGM_TN_MBOSS);
    }
}

// Called before the audio framework automatically unmutes the room after a
// stream finishes. Actor/cutscene polling can run too late for that audio frame.
void advance_shade_music() {
    if (!sShadeMusic.active || !sShadeMusic.introPending) return;
    auto* audio=Z2GetAudioMgr();
    auto* player=daAlink_getAlinkActorClass();
    if (!audio || !player) return;
    if (!arena() || dComIfGp_isEnableNextStage() || player->checkDeadHP() ||
        audio->getMainBgmID()!=sShadeMusic.roomMain) {
        sShadeMusic.introPending=false;
        return;
    }
    const auto stream=audio->getStreamBgmID();
    if (stream==kShadeIntroMusic) return; // includes asynchronous preparation
    if (stream!=kNoMusic || audio->getSubBgmID()!=kNoMusic) {
        sShadeMusic.introPending=false; // another cue now owns the audio
        return;
    }
    start_shade_music(false);
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
