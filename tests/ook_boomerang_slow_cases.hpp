void test_ook_boomerang_slow() {
    using namespace dawnlight;
    namespace bo = dawnlight::test_ook_boomerang;
    e_mk_class ook;Model model;
    ook.actor.profile=fpcNm_E_MK_e;parent=&ook.actor;
    e_mk_bo_class a;a.model=&model;
    auto* actor=&a.enemy;
    std::any args[1]{&a};
    auto make_step=[&](float scale,bool tick){
        EnemySlowStep step{};step.actor=actor;step.profile=&bo::ook_boomerang_slow_profile();
        step.scale=scale;step.timerTick=tick;step.originalPosition=actor->current.pos;
        step.originalShapeAngles=actor->shape_angle;bo::prepare(step,tick);return step;
    };
    assert(bo::eligible(actor));
    for(int reason=0;reason<7;++reason){
        a.field_0x600=reason==0;a.field_0x9b4=reason==1;midnaTalk=reason==2;
        eventRunning=reason==3;a.action=reason==4 ? 3 : 0;
        ook.demoMode=reason==5 ? e_mk_class::DEMO_MODE_START : e_mk_class::DEMO_MODE_NONE;
        parent=reason==6 ? nullptr : &ook.actor;
        assert(!bo::eligible(actor));
    }
    a.field_0x600=a.field_0x9b4=a.action=0;midnaTalk=eventRunning=false;
    ook.demoMode=0;parent=&ook.actor;
    ook.actor.profile=999;assert(!bo::eligible(actor));ook.actor.profile=fpcNm_E_MK_e;
    assert(bo::eligible(actor));
    // Execute the existing hard-mode bonus followed by the new production hooks.
    // Native flight adds 40 units; the complete slowed distance must be 4 or 6,
    // not 24 (unscaled bonus) or 4.2 (double-scaled bonus).
    player.current.pos={0,-100,1000};
    for(float scale:{.1f,.25f,.5f,1.0f}) for(bool hard:{false,true}){
        actor->current.pos={};actor->current.angle={};actor->shape_angle={};
        actor->speed={0,0,40};a.action=0;a.mode=1;
        auto step=make_step(scale,true);live=&step;
        if(hard)test_ook_hard_mode::advance_ook_boomerang(a);
        bo::before_action(nullptr,args,nullptr,nullptr);
        assert(std::abs(actor->current.pos.z-(hard?20:0)*scale)<.0001f);
        actor->current.pos.z+=40; // native flight integration
        bo::after_flight(nullptr,args,nullptr,nullptr);
        assert(std::abs(actor->current.pos.z-(hard?60:40)*scale)<.0001f);
        // CrrPos and the live attack sphere receive this position, not a draw-only pose.
        const cXyz collisionPosition=actor->current.pos;
        actor->shape_angle.y+=0x2000;
        bo::after_action(nullptr,args,nullptr,nullptr);
        assert(actor->shape_angle.y==std::lround(0x2000*scale));
        assert(actor->current.pos.z==collisionPosition.z && actor->speed.z==40);
    }
    // Returning branches skip background correction, so the flight boundary
    // must still scale them. Collision/impact can replace speed; do not restore it.
    actor->current.pos={500,100,10};a.action=0;a.mode=3;
    auto returning=make_step(.1f,false);live=&returning;
    bo::before_action(nullptr,args,nullptr,nullptr);
    actor->current.pos.x-=40;bo::after_flight(nullptr,args,nullptr,nullptr);
    assert(actor->current.pos.x==496);
    actor->speed.y=30;a.action=1;
    bo::after_action(nullptr,args,nullptr,nullptr);
    assert(actor->speed.y==30);
    // Held clocks advance once per native tick, without repeating modulo sounds.
    for(s16 initial:{s16(0),s16(-8),s16(32760)}) {
        a.counter=initial;a.action=0;a.timers[0]=40;a.timers[1]=50;a.field_0x5f8=60;
        int sounds=0;float fraction=0;
        for(int frame=0;frame<320;++frame){
            const bool tick=advance_enemy_timer(fraction,.1f);
            auto step=make_step(.1f,tick);live=&step;
            a.counter=static_cast<s16>(static_cast<u16>(a.counter)+1);
            for(auto& timer:a.timers)if(timer)--timer;
            if(a.field_0x5f8)--a.field_0x5f8;
            sounds+=(a.counter&7)==0;
            bo::after_execute(step);
        }
        assert(static_cast<u16>(a.counter)==static_cast<u16>(static_cast<u16>(initial)+32));
        assert(sounds==4 && a.timers[0]==8 && a.timers[1]==18 && a.field_0x5f8==28);
    }
    // Room-four raw orbit turning is scaled before its movement matrix is built.
    a.action=0;a.mode=1;a.timers[0]=10;a.field_0x5ee=350;actor->current.angle.y=1000;
    auto orbit=make_step(.1f,true);live=&orbit;
    bo::before_orbit(nullptr,args,nullptr,nullptr);
    actor->current.angle.y+=a.field_0x5ee;
    assert(actor->current.angle.y==1035);
    // Pillar motion uses slowed displacement BEFORE the native floor clamp,
    // retains its bounce impulse and leaves the pillar's authored height intact.
    ook.hasira=&model;a.action=1;a.mode=0;actor->current.pos.y=100;actor->speed.y=30;a.field_0x5fc=2;
    auto pillar=make_step(.1f,false);live=&pillar;
    bo::before_pillar(nullptr,args,nullptr,nullptr);
    actor->current.pos.y+=actor->speed.y;actor->speed.y-=5;a.field_0x5fc+=.1f;
    bo::after_pillar(nullptr,args,nullptr,nullptr);
    assert(actor->current.pos.y==103 && std::abs(actor->speed.y-29.5f)<.0001f);
    assert(std::abs(a.field_0x5fc-2.01f)<.0001f);
    a.mode=2;actor->speed.y=-10;actor->current.pos.y=100;
    pillar=make_step(.1f,false);live=&pillar;
    bo::before_pillar(nullptr,args,nullptr,nullptr);
    actor->current.pos.y=100;actor->speed.y=(actor->speed.y-5)*-.4f;++a.mode;
    bo::after_pillar(nullptr,args,nullptr,nullptr);
    assert(a.mode==3 && actor->current.pos.y==100 && std::abs(actor->speed.y-4.2f)<.0001f);
    a.mode=5;actor->speed.y=0;
    pillar=make_step(.1f,false);live=&pillar;
    bo::before_pillar(nullptr,args,nullptr,nullptr);++a.mode;actor->speed.y=0;
    bo::after_pillar(nullptr,args,nullptr,nullptr);
    assert(a.mode==5 && actor->speed.y==0);
    // Missing pillar/foreign actor must not borrow or corrupt the current scope.
    ook.hasira=nullptr;actor->speed.y=30;
    pillar=make_step(.1f,false);live=&pillar;
    bo::before_pillar(nullptr,args,nullptr,nullptr);bo::after_pillar(nullptr,args,nullptr,nullptr);
    assert(actor->speed.y==30);
    e_mk_bo_class other;other.enemy.current.pos.z=99;args[0]=&other;
    bo::before_action(nullptr,args,nullptr,nullptr);bo::after_flight(nullptr,args,nullptr,nullptr);
    assert(other.enemy.current.pos.z==99);
    live=nullptr;args[0]=&a;actor->current.pos.z=40;
    bo::before_action(nullptr,args,nullptr,nullptr);bo::after_flight(nullptr,args,nullptr,nullptr);
    assert(actor->current.pos.z==40);
    parent=nullptr;
}
