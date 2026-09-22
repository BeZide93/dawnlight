// Each effect owns its level-sound handle and stable position, just like native
// Beamos. Stopping one eye must not stop the other eye or unrelated game sounds.
struct TrialLevelSound {
    cXyz position{0,0,0};
    Z2SoundObjSimple sound;
    bool running=false;
    void play(u32 id,const cXyz& at) {
        position=at;
        if (!sound.isAlive()) sound.init(&position,1);
        sound.framework(0,dComIfGp_getReverb(51));
        sound.startLevelSound(id,0,-1);
        running=true;
    }
    void stop() {
        if (running) sound.stopAllSounds(0);
        running=false;
    }
};
struct ShadeTrialAudio {
    TrialLevelSound shield;
    std::array<TrialLevelSound,2> beams,sparks;
    bool shieldBroken=false;
    void stop() {
        shield.stop();
        for (unsigned i=0;i<2;++i) { beams[i].stop(); sparks[i].stop(); }
    }
    void begin(shade::Trial kind,const cXyz& shieldPos,const std::array<cXyz,2>& eyes) {
        stop();shieldBroken=false;
        if (kind==shade::Trial::Shield)
            mDoAud_seStart(Z2SE_OBJ_GANON_BARRIER_APPR,&shieldPos,0,dComIfGp_getReverb(51));
        if (kind==shade::Trial::Eyes) for (const auto& eye:eyes) {
            mDoAud_seStart(Z2SE_EN_BM_FIND,&eye,0,dComIfGp_getReverb(51));
            mDoAud_seStart(Z2SE_EN_BM_HEAT,&eye,0,dComIfGp_getReverb(51));
        }
    }
    void update_shield(bool broken,const cXyz& at) {
        if (!broken) shield.play(Z2SE_OBJ_GANON_BARRIER,at);
        else {
            shield.stop();
            if (!shieldBroken)
                mDoAud_seStart(Z2SE_EN_PZ_BALL_BURST,&at,0,dComIfGp_getReverb(51));
        }
        shieldBroken=broken;
    }
    void update_beam(unsigned index,bool live,const cXyz& start,const cXyz& end,
                     float reach,const cXyz& listener) {
        if (!live) { beams[index].stop(); sparks[index].stop(); return; }
        const cXyz ray=(end-start)*reach;
        const float length2=ray.inprod(ray);
        const float t=length2>0 ? std::clamp((listener-start).inprod(ray)/length2,0.0f,1.0f) : 0;
        beams[index].play(Z2SE_EN_BM_BEAM2,start+ray*t);
        if (reach>=1) sparks[index].play(Z2SE_EN_BM_SPARK,end);
        else sparks[index].stop();
    }
};
