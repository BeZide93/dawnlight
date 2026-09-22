// Cinematic fire only: native campfire particles, no fire actor or damage volume.
// Both appearances use the normal Shade model; no private model/material copies.
struct ShadeFlame {
    std::array<u32,3> emitters{};
    bool active=false,arrival=false,extinguishing=false;
    int ticks=0;

    void stop() {
        for (auto& id:emitters) {
            if (auto* effect=dComIfGp_particle_getEmitter(id)) {
                effect->becomeInvalidEmitter();
                effect->deleteAllParticle();
            }
            id=0;
        }
        active=false;
    }
    bool hides_model() const {
        return active && (arrival ? ticks<shade::flame_switch_ticks : ticks>=shade::flame_switch_ticks);
    }
    void begin(daNpc_Kn_c* actor,bool arriving) {
        stop();active=true;arrival=arriving;extinguishing=false;ticks=0;
        actor->field_0x16f4.set(1,1,1);
        actor->field_0x170c=0;actor->field_0x170d=0;
        actor->mNoDraw=hides_model();
        mDoAud_seStart(Z2SE_OBJ_FIRE_IGNITION,&actor->current.pos,0,dComIfGp_getReverb(51));
    }
    void tick(daNpc_Kn_c* actor,int elapsed) {
        if (!active) return;
        ticks=elapsed;
        actor->mNoDraw=hides_model();
        const float grow=std::clamp(ticks/static_cast<float>(shade::flame_build_ticks),0.0f,1.0f);
        const float fade=std::clamp((ticks-shade::flame_fade_ticks)/
            static_cast<float>(shade::flame_total_ticks-shade::flame_fade_ticks),0.0f,1.0f);
        const float strength=(0.2f+0.8f*grow*grow*(3-2*grow))*(1-fade*fade*(3-2*fade));
        // A broad, tall campfire hides the body swap at peak intensity. Taper
        // particle size and opacity together so the fire visibly burns down.
        const cXyz scale{35.0f*strength,45.0f*strength,35.0f*strength};
        constexpr std::array<u16,3> effects{
            ID_ZF_J_FIRE00_GLOW,ID_ZF_J_FIRE02_FIRE,ID_ZF_J_FIRE01_SPARK};
        for (unsigned i=0;i<emitters.size();++i) {
            emitters[i]=dComIfGp_particle_set(emitters[i],effects[i],&actor->current.pos,
                nullptr,nullptr,&scale,static_cast<u8>(255*(1-fade)),nullptr,51,nullptr,nullptr,nullptr);
            if (emitters[i]) dComIfGp_particle_levelEmitterOnEventMove(emitters[i]);
        }
        if (ticks<shade::flame_fade_ticks) {
            mDoAud_seStartLevel(Z2SE_OBJ_FIRE_BURNING,&actor->current.pos,0,dComIfGp_getReverb(51));
        } else if (!extinguishing) {
            extinguishing=true;
            mDoAud_seStart(Z2SE_OBJ_FIRE_OFF,&actor->current.pos,0,dComIfGp_getReverb(51));
        }
    }
} sShadeFlame;

void begin_shade_flame(daNpc_Kn_c* actor,bool arrival) { sShadeFlame.begin(actor,arrival); }
void tick_shade_flame(daNpc_Kn_c* actor,int ticks) { sShadeFlame.tick(actor,ticks); }
void end_shade_flame() { sShadeFlame.stop(); }
