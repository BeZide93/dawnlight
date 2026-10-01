using namespace dawnlight;
void near(float a,float b){assert(std::abs(a-b)<0.001f);}
EnemySlowStep begin(fopAc_ac_c& actor,const EnemySlowProfile& profile,float scale=.25f){
    EnemySlowStep step;step.actor=&actor;step.profile=&profile;step.scale=scale;
    step.originalPosition=actor.current.pos;step.originalOldPosition=actor.old.pos;
    profile.prepare(step,false);return step;
}
void collision(EnemySlowStep& step){
    live=&step;std::any args[]{step.directCollision};before_collision(nullptr,args,nullptr,nullptr);
}
int main(){
    mDoExt_McaMorfSO morph; mDoExt_brkAnm brk;
    for(float scale:{1.f,.5f,.25f}){
        e_mm_class a;a.modelMorf=&morph;a.timers[0]=8;
        auto step=begin(a.enemy,test_helmasaur::helmasaur_slow_profile(),scale);
        live=&step;assert(a.timers[0]==9);assert(a.timers[1]==0);assert(step.animations[0]==&morph);
        // Native jump impulse 20, then -3 gravity and direct translation.
        a.enemy.speed.y=17;a.enemy.current.pos={12,17,0};collision(step);
        near(a.enemy.speed.y,20-3*scale);near(a.enemy.current.pos.y,(20-3*scale)*scale);
        near(a.enemy.current.pos.x,12*scale);assert(!step.directCollision);
        std::any args[]{&a.acch};before_collision(nullptr,args,nullptr,nullptr);
        near(a.enemy.current.pos.x,12*scale); // no duplicate correction
        std::any sound[]{&a.sound,JAISoundID(Z2SE_EN_MM_WALK_LND)};Z2SoundHandlePool* result=nullptr;
        assert(test_helmasaur::before_sound(nullptr,sound,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        step.freshAnimationFrame=true;
        assert(test_helmasaur::before_sound(nullptr,sound,&result,nullptr)==HOOK_CONTINUE);
        Z2CreatureEnemy other;sound[0]=&other;step.freshAnimationFrame=false;
        assert(test_helmasaur::before_sound(nullptr,sound,&result,nullptr)==HOOK_CONTINUE);
        parent=&a.enemy;parent->profile=fpcNm_E_MM_e;e_mm_mt_class shell;Model model;shell.mp_model=&model;
        assert(test_helmasaur::helmasaur_armor_slow_profile().eligible(&shell.enemy));
        shell.m_action=1;assert(!test_helmasaur::helmasaur_armor_slow_profile().eligible(&shell.enemy));
    }
    {
        e_ai_class a;a.m_modelMorf=&morph;a.m_brk=&brk;a.m_timers[2]=1;a.m_lifetime=10;
        auto step=begin(a,test_armos::armos_slow_profile());live=&step;
        assert(a.m_lifetime==9&&a.m_timers[2]==3);--a.m_timers[2];
        step.profile->afterExecute(step);assert(a.m_timers[2]==1);
        assert(step.controllers[0]==brk.getFrameCtrl());
        std::any args[]{&a};a.m_mode=1;morph.frame=4.5f;
        test_armos::before_attack(nullptr,args,nullptr,nullptr);near(morph.frame,3);
        test_armos::after_attack(nullptr,args,nullptr,nullptr);near(morph.frame,4.5f);
        a.m_action=e_ai_class::ACTION_DAMAGE;a.field_0x6a8=400;a.current.angle.y=500;
        step.profile->beforeAngleChase(step,&a.shape_angle.y);assert(a.current.angle.y==200);
        // Gravity applied after movement must not change this frame's displacement.
        a.current.pos.y=20;a.speed.y=17;collision(step);near(a.current.pos.y,5);near(a.speed.y,19.25f);
    }
    {
        daE_GE_c a;a.mpMorfSO=&morph;a.mActionMode=1;a.mMode=1;a.field_0xb8c=100;
        auto step=begin(a,test_guay::guay_slow_profile());live=&step;std::any args[]{&a,0,0,s16(300)};
        test_guay::before_flight(nullptr,args,nullptr,nullptr);a.field_0xb5c+=4;near(a.field_0xb5c,1);
        a.field_0xb8a=200;a.field_0xb8c=300;
        test_guay::before_circle(nullptr,args,nullptr,nullptr);assert(a.field_0xb8c==150);assert(std::any_cast<s16>(args[3])==150);
        args[3]=s16(900);test_guay::before_circle(nullptr,args,nullptr,nullptr);assert(std::any_cast<s16>(args[3])==900);
        a.speed.y=8;test_guay::after_circle(nullptr,args,nullptr,nullptr);a.speed.y=4;
        test_guay::after_attack(nullptr,args,nullptr,nullptr);near(a.speed.y,7);
        a.mActionMode=7;a.mMode=0;assert(!step.profile->eligible(&a));
        a.current.pos.x=80;collision(step);near(a.current.pos.x,80); // absolute boomerang ownership
        a.mMode=2;assert(step.profile->eligible(&a));
    }
    {
        e_kr_class a;a.mpMorf=&morph;a.mCurAction=3;a.field_0x672=4;a.field_0x69c[0]=32;
        a.field_0x6d6=8;a.field_0x6d8=20;a.field_0x6a8=4;
        auto step=begin(a.enemy,test_kargarok::kargarok_slow_profile());live=&step;step.timerFraction=.5f;
        assert(a.field_0x69c[0]==34);--a.field_0x69c[0];++a.field_0x6d8;
        std::any hover[]{&a.enemy.current.pos.x,0.f};test_kargarok::before_hover(nullptr,hover,nullptr,nullptr);
        near(std::any_cast<float>(hover[1]),200*cM_ssin(s16(20500)));
        a.enemy.current.pos.x=40;std::any play[]{&morph};test_kargarok::before_play(nullptr,play,nullptr,nullptr);
        near(a.enemy.current.pos.x,10);assert(a.field_0x69c[0]==32&&a.field_0x6d8==20&&a.field_0x6a8==4);
        // CrrPos follows model building. Scale only the new push and retain origin offsets.
        a.enemy.current.pos.x+=8;a.enemy.current.pos.y+=100;a.enemy.old.pos.y+=100;
        collision(step);near(a.enemy.current.pos.x,12);near(a.enemy.current.pos.y,100);
        step.profile->afterExecute(step);assert(a.field_0x6d6==8);
        a.field_0x6e4=&a;assert(!step.profile->eligible(&a.enemy));a.field_0x6e4=nullptr;
        a.enemy.health=0;assert(!step.profile->eligible(&a.enemy));
    }
    {
        e_sm2_class a;a.modelMorf=&morph;a.counter=8;a.field_0x708[0]={10,0,0};
        auto step=begin(a.enemy,test_chu::chu_slow_profile());live=&step;
        assert(a.counter==7&&a.combine_off_timer==2);++a.counter;--a.combine_off_timer;
        a.field_0x708[1]={40,0,0};a.jnt_pos[1]={40,0,0};a.field_0x768[1].y=0x4000;a.field_0x7f8[1].y=0x4000;
        // Fresh head is unchanged; live collision and visual chains interpolate together.
        a.field_0x708[0]={0,0,0};test_chu::relax_body(step);
        near(a.field_0x708[1].x,10);near(a.jnt_pos[1].x,10);assert(a.field_0x7f8[1].y==0x4000);
        step.profile->afterExecute(step);assert(a.combine_off_timer==0);
        a.combine_off_timer=50;step.profile->afterExecute(step);assert(a.combine_off_timer==50);
        a.sizetype=1;a.field_0x708[1].x=80;test_chu::relax_body(step);near(a.field_0x708[1].x,80);
        a.action=ACTION_ROOF;assert(!step.profile->eligible(&a.enemy));
        a.action=ACTION_FAIL;a.mode=2;assert(!step.profile->eligible(&a.enemy));
        a.mode=0;a.enemy.eventInfo.catchCommand=true;assert(!step.profile->eligible(&a.enemy));
    }
    live=nullptr;std::any args[]{&morph};
    assert(test_chu::before_play(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(test_kargarok::before_play(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    std::cout<<"Cave enemy production profile tests passed\n";
}
