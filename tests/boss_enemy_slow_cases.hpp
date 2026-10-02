int main() {
    test_ook_boomerang_slow();
    using namespace dawnlight;
    const EnemySlowProfile profile{99,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
    fopAc_ac_c actor, foreign;
    EnemySlowStep step{};step.actor=&actor;step.profile=&profile;step.scale=.1f;live=&step;
    std::any args[5];
    // Direct movement and posMoveF -> posMove nesting must integrate exactly once.
    step.slowDirectMove=true;actor.gravity=-5;args[0]=&actor;
    before_move(nullptr,args,nullptr,nullptr);
    before_direct_move(nullptr,args,nullptr,nullptr);
    actor.current.pos.x+=10;
    after_direct_move(nullptr,args,nullptr,nullptr);
    assert(actor.current.pos.x==10);
    after_move(nullptr,args,nullptr,nullptr);
    assert(actor.current.pos.x==1 && actor.gravity==-5);
    assert(!step.moving && !step.directMoving);
    before_direct_move(nullptr,args,nullptr,nullptr);
    actor.current.pos.x+=10;
    after_direct_move(nullptr,args,nullptr,nullptr);
    assert(actor.current.pos.x==2 && actor.gravity==-5);
    args[0]=&foreign;
    before_direct_move(nullptr,args,nullptr,nullptr);
    assert(!step.moving && !step.directMoving);
    // Unowned vectors/minimum steps stay native, owned vectors use the same scale.
    cXyz target;step.chasePositions[0]=&actor.current.pos;
    args[0]=&foreign.current.pos;args[1]=&target;args[2]=1.0f;args[3]=10.0f;args[4]=2.0f;
    before_chase_position_min(nullptr,args,nullptr,nullptr);
    assert(std::any_cast<float>(args[3])==10);
    args[0]=&actor.current.pos;
    before_chase_position_min(nullptr,args,nullptr,nullptr);
    assert(std::abs(std::any_cast<float>(args[2])-.1f)<.00001f);
    assert(std::any_cast<float>(args[3])==1);
    assert(std::abs(std::any_cast<float>(args[4])-.2f)<.00001f);
    step.chaseFloats[0]=&actor.speedF;
    args[0]=&actor.speedF;args[1]=20.0f;args[2]=1.0f;args[3]=10.0f;args[4]=2.0f;
    before_chase_target_min(nullptr,args,nullptr,nullptr);
    assert(std::abs(std::any_cast<float>(args[3])-1.5f)<.00001f);
    assert(std::abs(std::any_cast<float>(args[4])-.3f)<.00001f);
    // Conditional timers are held only at an actual call, never by pre-incrementing.
    int timer=1,other=5,result=-1;step.conditionalIntTimers[0]=&timer;
    args[0]=&timer;
    assert(before_int_timer(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==1 && timer==1);
    args[0]=&other;
    assert(before_int_timer(nullptr,args,&result,nullptr)==HOOK_CONTINUE && other==5);
    step.timerTick=true;args[0]=&timer;
    assert(before_int_timer(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
    step.timerTick=false;u8 byte=0,byteResult=99;step.conditionalByteTimers[0]=&byte;args[0]=&byte;
    assert(before_byte_timer(nullptr,args,&byteResult,nullptr)==HOOK_SKIP_ORIGINAL && byteResult==0);
    live=nullptr;
    assert(before_byte_timer(nullptr,args,&byteResult,nullptr)==HOOK_CONTINUE);
    // Deletion's early return must not grow Blizzeta ice's unused clocks.
    daB_YOI_c ice;ice.mTimer1=7;ice.mTimer2=8;ice.mIFrameTimer=9;ice.mDeleteTimer=5;
    EnemySlowStep iceStep{};iceStep.actor=&ice;
    test_blizzeta_ice::prepare(iceStep,false);
    assert(ice.mDeleteTimer==6 && ice.mTimer1==7 && ice.mTimer2==8 && ice.mIFrameTimer==9);
    ice.mDeleteTimer=0;test_blizzeta_ice::prepare(iceStep,false);
    assert(ice.mTimer1==8 && ice.mTimer2==9 && ice.mIFrameTimer==10);
    // A new Death Sword projectile slows from its spawn position, not the boss origin.
    daE_VA_c sword;step.actor=&sword;step.scale=.1f;
    sword.mMagicOldPos[0]={1000,20,50};sword.mMagicPos[0]={1100,30,70};
    sword.mMagicPos[1]={2000,0,0};
    auto magicProfile=profile;magicProfile.beforeAnyCollision=before_magic_collision;
    step.profile=&magicProfile;live=&step;args[0]=static_cast<dBgS_Acch*>(&sword.mMagicAcch[0]);
    before_collision(nullptr,args,nullptr,nullptr);
    assert(sword.mMagicPos[0].x==1010 && sword.mMagicPos[0].y==21 && sword.mMagicPos[1].x==2000);
    // Triangle events fire once each even with ten display frames per native tick.
    int triangle=0,light=0,wall=0;float fraction=0;
    for(int i=0;i<2000;++i){
        int before=triangle;
        triangle=advance_boss_triangle(triangle,advance_enemy_timer(fraction,.1f));
        light += before<2 && triangle>=2;wall += before<100 && triangle>=100;
    }
    assert(light==1 && wall==1 && triangle==210);
    assert(advance_boss_triangle(100,false)==100);
    // Morpheel's history preserves 512 simulation samples, including wrap and nested actors.
    mDoExt_McaMorfSO morph;b_ob_class fish,child;fish.mpCoreMorf=&morph;child.mpCoreMorf=&morph;
    auto setup=[](b_ob_class& a){for(int i=0;i<512;++i)a.field_0x2324[i]={float(i),0,0};a.field_0x2320=511;};
    setup(fish);setup(child);
    EnemySlowStep fishStep{};fishStep.actor=&fish;fishStep.profile=&test_morpheel::morpheel_slow_profile();
    fishStep.scale=.1f;fishStep.timerFraction=.5f;live=&fishStep;args[0]=&fish;
    test_morpheel::before_fish(nullptr,args,nullptr,nullptr);
    assert(fish.field_0x2324[100].x==99.5f);
    fish.field_0x2324[511]={900,0,0};fish.field_0x2320=0;
    EnemySlowStep childStep=fishStep;childStep.actor=&child;live=&childStep;args[0]=&child;
    test_morpheel::before_fish(nullptr,args,nullptr,nullptr);
    child.field_0x2324[511]={800,0,0};child.field_0x2320=0;
    test_morpheel::after_fish(nullptr,args,nullptr,nullptr);
    assert(child.field_0x2320==511 && child.field_0x2324[511].x==511);
    live=&fishStep;args[0]=&fish;
    test_morpheel::after_fish(nullptr,args,nullptr,nullptr);
    assert(fish.field_0x2320==511 && fish.field_0x2324[100].x==100 && fish.field_0x2324[511].x==511);
    fishStep.timerTick=true;
    test_morpheel::before_fish(nullptr,args,nullptr,nullptr);
    fish.field_0x2324[511]={900,0,0};fish.field_0x2320=0;
    test_morpheel::after_fish(nullptr,args,nullptr,nullptr);
    assert(fish.field_0x2320==0 && fish.field_0x2324[511].x==900 && fish.field_0x2324[100].x==100);
    test_morpheel::reset();assert(test_morpheel::s_depth==0);
    for(const auto& history:test_morpheel::s_history)assert(history.actor==nullptr);
    live=nullptr;
    std::cout << "Boss movement, timer, projectile and history regressions passed\n";
}
