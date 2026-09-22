#include "heroes_shade_encounter.hpp"
#include "heroes_shade_battle.hpp"
#include "heroes_shade_cinema.hpp"
#include "generated/pedestal_stone.hpp"
#include "pedestal_mesh.hpp"
#include "enemy_spawner.hpp"
#include "f_pc/f_pc_deletor.h"
#include "f_pc/f_pc_create_req.h"
#include "save_state.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_npc_kn.h"
#include "d/actor/d_a_npc_gwolf.h"
#include "m_Do/m_Do_graphic.h"
#include "JSystem/J3DGraphBase/J3DShape.h"
#include "d/actor/d_a_obj_knBullet.h"
#include "d/actor/d_a_obj_hsTarget.h"
#include "d/d_bg_w.h"
#include "d/d_com_inf_game.h"
#include "d/d_camera.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SoundObject.h"
#include "d/d_particle_name.h"
#include "d/d_bg_s_lin_chk.h"
#include "d/d_msg_object.h"
#include "f_op/f_op_msg_mng.h"
#include "f_pc/f_pc_layer.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/flow.hpp"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "res/Object/MstrSword.h"
#include "res/Object/E_fm.h"
#include "res/Object/E_bm6.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <unordered_set>

namespace dawnlight {
namespace {
constexpr ActorId kNone = fpcM_ERROR_PROCESS_ID_e;
constexpr u32 kShadeParams = 0x00ffff07; // lesson 7 resources, no path
constexpr char kSwordArchive[] = "MstrSword";
constexpr int kAttackCooldown = 18;
constexpr int kComboGap = 6;
constexpr int kDoubleAttackDelay = 15;
constexpr float kApproachSpeed = 6.0f;
ProfileName sProfile = -1;
ActorHandle sRegistration = 0;
ActorId sPedestal = kNone;
struct Fighter {
    ActorId id = kNone;
    int divide = 0;
    int offense = -1;
    int attackTicks = 0;
    int cooldown = kAttackCooldown;
    shade::AttackChain chain;
    shade::BladeMotion blade;
    // Stable storage, shared native actor status; no new actor or draw entry.
    std::array<dCcD_Cps,2> bladeSweeps;
    float orbitAngle = 0;
    float orbitRadius = 150;
    bool swordContact = false;
    bool jumpLaunched = false;
    bool helmTurnPending = false;
    bool animationStarted = false;
    cXyz jumpTarget{0,0,0};
    cXyz jumpBodyOffset{0,0,0};
    bool reset = true;
    bool deleting = false;
};
std::array<Fighter, 3> sFighters;
shade::Battle sBattle;
bool sStopping = false;
std::unordered_set<ActorId> sProjectiles;
shade::Cinema sCinema;
std::array<mods::flow::RegisteredMessage,4> sLines;
mods::flow::RegisteredMessage sBossTitle;
void cancel_trial();
void finish_trial();
void tick_arena_hazards();
void suspend_trial();
void sync_trial_position(daNpc_Kn_c*);
void begin_trial(daNpc_Kn_c*);
bool tick_trial(daNpc_Kn_c*);
struct CinemaRuntime {
    ActorId player = kNone;
    fpc_ProcID camera = kNone;
    int cameraIndex = 0;
    bool ownsEvent = false;
    int trim = 0;
    int wolfWait = 0;
    cXyz eye{0,0,0}, center{0,0,0};
    float fovy = 55;
    s16 facing = 0;
    MessageId message = 0;
    bool messageStarted = false;
    bool messageOpened = false;
} sCinemaRuntime;

void stop_blade_sweeps(Fighter& entry) {
    for (auto& sweep:entry.bladeSweeps) {
        sweep.OffAtSetBit();
        sweep.ClrAtHit();
    }
}

int attack_cooldown(const Fighter& entry) {
    return kAttackCooldown + entry.divide * kDoubleAttackDelay;
}

bool ui_document_visible() {
    bool visible = false;
    return svc_ui->is_any_document_visible(mod_ctx, &visible) == MOD_OK && visible;
}

bool arena() {
    return save_state_boss_rush_active() && save_state_boss_rush_state() == 0 &&
        std::strcmp(dComIfGp_getStartStageName(), "D_DLBR0") == 0 &&
        dComIfGp_getStartStageRoomNo() == 51;
}
#include "heroes_shade_music.inc"

Fighter* fighter(daNpc_Kn_c* actor) {
    if (!actor) return nullptr;
    for (auto& entry : sFighters) if (entry.id != kNone && entry.id == fopAcM_GetID(actor)) return &entry;
    return nullptr;
}
fopAc_ac_c* actor_by_id(ActorId id) {
    fopAc_ac_c* actor = nullptr;
    if (id != kNone && !fpcM_IsCreating(id)) fopAcM_SearchByID(id, &actor);
    return actor;
}
bool pending_or_live(ActorId id) {
    return id != kNone && (fpcM_IsCreating(id) || actor_by_id(id));
}
ModResult spawn(ProfileName profile, const cXyz& pos, s16 yaw, u32 parameters, ActorId& id) {
    const ActorSpawnParams params{
        .parameters=parameters, .argument=-1, .room_num=51,
        .position={pos.x,pos.y,pos.z}, .angle={-1,yaw,0}, .scale={1,1,1},
        .create_function=nullptr,
    };
    return create_standalone_actor(profile,params,id);
}
void remove_actor(ActorId id) {
    if (id == kNone) return;
    // ActorService::delete_actor only sees completed actors. Cancel async
    // requests as well, otherwise a late double can outlive the encounter.
    for (auto* node=g_fpcCtTg_Queue.mpHead;node;) {
        auto* next=NODE_GET_NEXT(node);
        auto* request=static_cast<create_request*>(reinterpret_cast<create_tag*>(node)->base.mpTagData);
        if (request->id==id && !fpcCtRq_IsDoing(request)) {
            fpcCtRq_Cancel(request);
            return;
        }
        node=next;
    }
    if (actor_by_id(id)) svc_actor->delete_actor(mod_ctx,id);
}
#include "heroes_shade_wolf.inc"

void remove_companions() {
    for (unsigned i=1;i<sFighters.size();++i) {
        sFighters[i].deleting = true;
        remove_actor(sFighters[i].id);
    }
    const auto projectiles=sProjectiles;
    for (const auto id:projectiles) remove_actor(id);
    auto* boss = static_cast<daNpc_Kn_c*>(actor_by_id(sFighters[0].id));
    if (boss) {
        auto* bullet = actor_by_id(boss->parentActorID);
        if (bullet && fopAcM_GetName(bullet)==fpcNm_KN_BULLET_e) remove_actor(boss->parentActorID);
        boss->parentActorID = kNone;
    }
}

ModResult register_cinema_lines() {
    // Original dialogue, not game text or text from another mod. The regret
    // about unpassed knowledge and the inherited courage belong to his story.
    constexpr const char* english[] = {
        "My blade fell silent long ago,\nwith lessons still untold.\nYet our courage lives on in you, Link.",
        "Come. Meet my steel,\nand show me the hero\nyou have become.",
        "A true hero, Link!\nYou have surpassed even my blade.\nYour courage honors those before you.",
        "Carry our legacy beyond this place.\nWhere your light endures,\nhope shall not fade."
    };
    constexpr const char* german[] = {
        "Mein Schwert verstummte einst,\nbevor ich mein Wissen weitergeben konnte.\nDoch unser Mut lebt in dir, Link.",
        "Begegne meiner Klinge.\nZeig mir den Helden,\nder du geworden bist.",
        "Ein wahrer Held, Link!\nDu hast selbst meine Klinge bezwungen.\nDein Mut ehrt jene, die vor dir kamen.",
        "Trage unser Erbe hinaus in die Welt.\nWo dein Licht leuchtet,\nwird die Hoffnung niemals weichen."
    };
    constexpr MessageLanguage languages[]={MESSAGE_LANGUAGE_ENGLISH,MESSAGE_LANGUAGE_GERMAN,
        MESSAGE_LANGUAGE_FRENCH,MESSAGE_LANGUAGE_SPANISH,MESSAGE_LANGUAGE_ITALIAN,MESSAGE_LANGUAGE_JAPANESE};
    constexpr auto style=mods::flow::MessageStyle{}.box_kind(MESSAGE_BOX_DEMO_CAPTION)
        .box_position(MESSAGE_POSITION_BOTTOM).draw_type(MESSAGE_DRAW_INSTANT);
    for (unsigned i=0;i<sLines.size();++i) {
        std::vector<mods::flow::MessageVariant> variants;
        for (const auto language:languages) variants.push_back(mods::flow::MessageBuilder{style}
            .text(language==MESSAGE_LANGUAGE_GERMAN ? german[i] : english[i]).auto_advance(120).build(language));
        sLines[i]=mods::flow::register_message(0,variants);
        if (!sLines[i]) return sLines[i].result();
    }
    // The same native screen used by the story bosses (zelda_boss_name.blo),
    // with our own registered text rather than overriding a stock boss name.
    constexpr auto titleStyle=mods::flow::MessageStyle{}.box_kind(MESSAGE_BOX_BOSS_NAME)
        .box_position(MESSAGE_POSITION_BOTTOM).draw_type(MESSAGE_DRAW_INSTANT);
    std::vector<mods::flow::MessageVariant> titleVariants;
    for (const auto language:languages) titleVariants.push_back(mods::flow::MessageBuilder{titleStyle}
        .text("Hero's Shade").auto_advance(90).build(language));
    sBossTitle=mods::flow::register_message(0,titleVariants);
    if (!sBossTitle) return sBossTitle.result();
    return MOD_OK;
}
void close_cinema_line() {
    auto* message=dMsgObject_getMsgObjectClass();
    if (sCinemaRuntime.messageStarted && message && message->msg_idx==sCinemaRuntime.message) {
        const auto status=message->getStatusLocal();
        if (status==1 && !sCinemaRuntime.messageOpened) {
            // An accepted demo message can still be queued at idle. The kill
            // flag alone only clears demo flags in that state; schedule native
            // deleteProc as well so its pending text cannot open after cancel.
            message->setStatusLocal(19);
            message->onKillMessageFlagLocal();
        } else if (status!=1) message->onKillMessageFlagLocal();
    }
    sCinemaRuntime.messageStarted=false;
    sCinemaRuntime.messageOpened=false;
    sCinemaRuntime.message=0;
}
void start_cinema_line(MessageId text) {
    const auto id=fopMsgM_messageSetDemo(text);
    if (id!=0 && id!=kNone) {
        sCinemaRuntime.message=text;
        sCinemaRuntime.messageStarted=true;
        auto* message=dMsgObject_getMsgObjectClass();
        sCinemaRuntime.messageOpened=message && message->msg_idx==text && message->getStatusLocal()>1;
    }
}
void release_cinema(bool keepWolf=false) {
    if (keepWolf) sShadeWolf.end_visual(); else sShadeWolf.clear();
    close_cinema_line();
    if (sCinemaRuntime.ownsEvent) {
        auto* boss=actor_by_id(sFighters[0].id);
        // Only release the event we actually obtained, never somebody else's
        // story event or a replacement player/camera after a scene transition.
        if (boss && boss->eventInfo.checkCommandDemoAccrpt()) dComIfGp_event_reset();
        auto* player=daAlink_getAlinkActorClass();
        if (player && fopAcM_GetID(player)==sCinemaRuntime.player) player->cancelOriginalDemo();
        auto* camera=dComIfGp_getCamera(sCinemaRuntime.cameraIndex);
        if (camera && fpcM_GetID(camera)==sCinemaRuntime.camera) {
            camera->mCamera.Reset(sCinemaRuntime.center,sCinemaRuntime.eye,sCinemaRuntime.fovy,s16(0));
            camera->mCamera.SetTrimSize(sCinemaRuntime.trim);
            camera->mCamera.Start();
        }
    }
    sCinema={};
    sCinemaRuntime={};
}
void finish_cinema(daNpc_Kn_c* actor,Fighter& entry) {
    const bool victory=sCinema.victory;
    release_cinema(!victory);
    if (victory) { entry.deleting=true; remove_actor(entry.id); }
    else {
        start_shade_music(false);
        actor->mNoDraw=false;
        actor->field_0x16f4.set(1,1,1);
        entry.reset=true;
    }
}
void cinema_pose(daNpc_Kn_c* actor, int motion) {
    actor->mFaceMotionSeqMngr.setNo(1,-1,0,0);
    actor->mMotionSeqMngr.setNo(motion,3,1,0);
    actor->field_0x15bc=0;
}
bool cinema_landing() {
    return sCinema.victory && (sCinema.shot==shade::Shot::Request || sCinema.shot==shade::Shot::Recover);
}
void cinema_camera(daNpc_Kn_c* actor) {
    auto* camera=dComIfGp_getCamera(sCinemaRuntime.cameraIndex);
    if (!camera || fpcM_GetID(camera)!=sCinemaRuntime.camera) return;
    const float sn=cM_ssin(sCinemaRuntime.facing), cs=cM_scos(sCinemaRuntime.facing);
    const bool words=sCinema.shot==shade::Shot::Words1 || sCinema.shot==shade::Shot::Words2;
    const bool wolf=sShadeWolf.visible();
    const float distance=words ? 340.0f : 510.0f;
    cXyz center=actor->current.pos+cXyz(0,wolf ? 75.0f : 145.0f,0);
    cXyz eye=actor->current.pos+cXyz(sn*distance+cs*130,185,cs*distance-sn*130);
    // Short, gentle dolly; stage walls constrain every intermediate eye point.
    center=camera->mCamera.Center()+(center-camera->mCamera.Center())*0.08f;
    eye=camera->mCamera.Eye()+(eye-camera->mCamera.Eye())*0.06f;
    dBgS_CamLinChk line;
    line.Set(&center,&eye,actor);
    if (dComIfG_Bgsp().LineCross(&line)) eye=center+(line.GetCross()-center)*0.9f;
    camera->mCamera.Set(center,eye,50.0f,s16(0));
}
void cinema_sword_shot(daNpc_Kn_c* actor) {
    auto* camera=dComIfGp_getCamera(sCinemaRuntime.cameraIndex);
    auto* player=daAlink_getAlinkActorClass();
    if (!camera || !player) return;
    const float sn=cM_ssin(sCinemaRuntime.facing),cs=cM_scos(sCinemaRuntime.facing);
    const cXyz eye=actor->current.pos+cXyz(sn*180,150,cs*180);
    const cXyz center=player->current.pos+cXyz(0,80,0);
    camera->mCamera.Set(center,eye,50.0f,s16(0));
}
// Called once per native main-actor simulation tick, never by Draw. Keeping the
// main actor alive until Afterglow finishes also keeps event ownership valid.
void tick_cinema(daNpc_Kn_c* actor,Fighter& entry) {
    using shade::Shot;
    if (dComIfGp_isPauseFlag() || ui_document_visible()) return;
    auto* player=daAlink_getAlinkActorClass();
    // Prepare the hidden wolf before moving the camera. Failure is bounded and
    // cancels the intro so the sword can be retried, without starting combat.
    if (sCinema.shot==Shot::Request && !sShadeWolf.ready()) {
        if (++sCinemaRuntime.wolfWait>=180 || !pending_or_live(sShadeWolf.id)) {
            release_cinema(); entry.deleting=true; remove_actor(entry.id);
        }
        return;
    }
    if (sCinema.shot==Shot::Request && actor->eventInfo.checkCommandDemoAccrpt()) {
        sCinemaRuntime.ownsEvent=true;
        auto* camera=dComIfGp_getCamera(dComIfGp_getPlayerCameraID(0));
        if (!camera) { finish_cinema(actor,entry); return; }
        if (!sCinema.victory) start_shade_music(true);
        sCinemaRuntime.player=fopAcM_GetID(player);
        sCinemaRuntime.camera=fpcM_GetID(camera);
        sCinemaRuntime.cameraIndex=dComIfGp_getPlayerCameraID(0);
        sCinemaRuntime.eye=camera->mCamera.Eye();
        sCinemaRuntime.center=camera->mCamera.Center();
        sCinemaRuntime.fovy=camera->mCamera.Fovy();
        sCinemaRuntime.trim=camera->mCamera.mTrimSize;
        sCinemaRuntime.facing=fopAcM_searchPlayerAngleY(actor);
        camera->mCamera.Stop(); camera->mCamera.SetTrimSize(3);
        if (!sCinema.victory) cinema_sword_shot(actor);
        player->changeOriginalDemo();
        player->changeDemoMode(daPy_demo_c::DEMO_WAIT_TURN_e,0,0,0);
        player->mDemo.setMoveAngle(cLib_targetAngleY(&player->current.pos,&actor->current.pos));
        actor->setAngle(sCinemaRuntime.facing);
        actor->offDownFlg(); actor->offHeadLockFlg();
        const int motion=actor->mMotionSeqMngr.getNo();
        // A final counter can still leave him airborne. Finish that landing
        // before selecting a get-up motion, rather than flattening him in midair.
        if (!sCinema.victory || shade::knockdown_landing_motion(motion)<0)
            cinema_pose(actor,sCinema.victory ? (motion==15 || motion==16 ? 16 :
                motion>=19 && motion<=22 ? 22 : 0) : 0);
        actor->field_0x170c=0;
        actor->field_0x170d=0;
    } else if (sCinema.shot==Shot::Request) {
        fopAcM_orderPotentialEvent(actor,2,0xffff,0);
        actor->eventInfo.onCondition(2);
    }
    if (sCinema.shot==Shot::Recover && actor->mAcch.ChkGroundHit() &&
        actor->mMotionSeqMngr.getStepNo()>0) {
        const int motion=actor->mMotionSeqMngr.getNo();
        if (motion==15 || motion==19) cinema_pose(actor,motion==15 ? 16 : 22);
    }
    const auto previous=sCinema.shot;
    bool message_done=false;
    if (previous==Shot::Words1 || previous==Shot::Words2 || previous==Shot::BossName) {
        const auto index=(sCinema.victory ? 2 : 0)+(previous==Shot::Words2);
        const auto text=previous==Shot::BossName ? sBossTitle.id() : sLines[index].id();
        auto* message=dMsgObject_getMsgObjectClass();
        if (!sCinemaRuntime.messageStarted) {
            start_cinema_line(text);
        } else if (!message || message->msg_idx!=sCinemaRuntime.message) {
            message_done=true; // another message owns the object now
        } else {
            const auto status=message->getStatusLocal();
            if (status>1) sCinemaRuntime.messageOpened=true;
            // setMessageIndexDemo(false) does not open synchronously. Idle is
            // completion only after this particular message was seen opening.
            message_done=sCinemaRuntime.messageOpened && status==1;
        }
    }
    // Show the banner during KN_DEMO_KAMAE, not after its transition to idle.
    // Cue the last 30% of the gesture so the native fade-in overlaps the sword
    // being pulled back. Ignore the previous clip until ctrlMotion installs it.
    auto* readyModel=actor->mpModelMorf[0];
    const bool title_cue=actor->mMotionSeqMngr.getNo()==24 &&
        !actor->mMotionSeqMngr.checkEntryNewMotion() &&
        (actor->mMotionSeqMngr.getStepNo()>0 ||
            (readyModel && readyModel->getEndFrame()>0 &&
                readyModel->getFrame()>=readyModel->getEndFrame()*0.70f));
    const bool pose_done=previous==Shot::Ready ?
        title_cue :
        actor->mMotionSeqMngr.getNo()==0 || actor->mMotionSeqMngr.getStepNo()>0;
    const bool visual_done=(previous==Shot::Arrival || previous==Shot::Depart) &&
        sShadeWolf.tick(actor,sCinema.ticks);
    if (sCinema.tick(sCinemaRuntime.ownsEvent,message_done,pose_done,visual_done)) {
        finish_cinema(actor,entry);
        return;
    }
    if (previous!=sCinema.shot) {
        close_cinema_line();
        if (sCinema.shot==Shot::Arrival) sShadeWolf.begin(actor,true);
        if (previous==Shot::Arrival) { sShadeWolf.end_visual(); actor->mNoDraw=false; }
        if (sCinema.shot==Shot::Words1 || sCinema.shot==Shot::Words2) cinema_pose(actor,3); // TALK_A
        if (sCinema.shot==Shot::Ready) cinema_pose(actor,24); // ready the sword
        // Start on the cue tick while the gesture continues, without waiting
        // for a subsequent tick or restarting/changing the body animation.
        if (sCinema.shot==Shot::BossName) start_cinema_line(sBossTitle.id());
        if (sCinema.shot==Shot::Depart) {
            cinema_pose(actor,0);
            sShadeWolf.begin(actor,false);
        }
    }
    if (sCinema.shot==Shot::Afterglow) { sShadeWolf.end_visual(); actor->mNoDraw=true; }
    if (sCinemaRuntime.ownsEvent) cinema_camera(actor);
}
void add_doubles(daNpc_Kn_c* boss) {
    for (unsigned i=1;i<sFighters.size();++i) {
        auto& entry=sFighters[i];
        if (pending_or_live(entry.id)) continue;
        entry = {};
        entry.divide = i;
        cXyz pos=boss->current.pos;
        const s16 angle=static_cast<s16>(boss->shape_angle.y+(i==1 ? -0x4000 : 0x4000));
        pos.x+=cM_ssin(angle)*180;
        pos.z+=cM_scos(angle)*180;
        spawn(fpcNm_NPC_KN_e,pos,boss->shape_angle.y,kShadeParams,entry.id);
    }
}

DEFINE_HOOK(&Z2SeqMgr::processBgmFramework, ShadeMusicFrameworkHook);
HookAction before_music_framework(ModContext*,void*,void*,void*) {
    advance_shade_music();
    return HOOK_CONTINUE;
}

DEFINE_HOOK(&daNpc_Kn_c::isDelete, ShadeAdmissionHook);
DEFINE_HOOK(&daNpc_Kn_c::reset, ShadeResetHook);
DEFINE_HOOK(&daNpc_Kn_c::Execute, ShadeExecuteHook);
DEFINE_HOOK(&daNpc_Kn_c::Draw, ShadeDrawHook);
DEFINE_HOOK(&daNpc_Kn_c::Delete, ShadeDeleteHook);
DEFINE_HOOK(&daNpc_Kn_c::evtProc, ShadeEventHook);
DEFINE_HOOK(&daNpc_Kn_c::evtOrder, ShadeOrderHook);
DEFINE_HOOK(&daNpc_Kn_c::action, ShadeActionHook);
DEFINE_HOOK(&daNpc_Kn_c::teach01_swordFinishWait, ShadeEndingBlowHook);
DEFINE_HOOK(&daNpc_Kn_c::calcSwordAttackMove, ShadeApproachHook);
DEFINE_HOOK(&daNpc_Kn_c::ctrlMotion, ShadeMotionHook);
DEFINE_HOOK(&daNpc_Kn_c::afterSetMotionAnm, ShadeAccessoryMotionHook);
DEFINE_HOOK(&daNpc_Kn_c::beforeMove, ShadeMovementHook);
DEFINE_HOOK(&daNpc_Kn_c::afterMoved, ShadeLandingHook);
DEFINE_HOOK(&daNpc_Kn_c::setAttnPos, ShadeJumpPoseHook);
DEFINE_HOOK(&daNpc_Kn_c::setCollisionSword, ShadeSwordHook);
DEFINE_HOOK(&daNpc_Kn_c::setCollision, ShadeBodyHook);
DEFINE_HOOK(&daObjKnBullet_c::Create, ShadeBulletHook);
DEFINE_HOOK(&daObjKnBullet_c::Delete, ShadeBulletDeleteHook);
DEFINE_HOOK(&fopAcM_createChild, ShadeChildHook);

HookAction admit(ModContext*,void* args,void* result,void*) {
    if (!fighter(mods::arg<daNpc_Kn_c*>(args,0))) return HOOK_CONTINUE;
    *static_cast<int*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_reset(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    // reset checks mDivideNo separately from the no-path parameter. A temporary
    // double prevents its automatic Golden Wolf child without changing saves.
    if (fighter(actor)) actor->mDivideNo=1;
    return HOOK_CONTINUE;
}
void after_reset(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    if (auto* entry=fighter(actor)) {
        actor->mDivideNo=entry->divide;
        actor->field_0xe2c=0;
        actor->field_0x15af=1;
        actor->mNoDraw=false;
        if (!entry->divide && sCinema.active() && !sCinema.victory) {
            actor->mNoDraw=true;
            actor->field_0x16f4.set(1,1,1);
            actor->field_0x15af=0;
        }
        actor->parentActorID=entry->divide ? sFighters[0].id : kNone;
        actor->health=shade::starting_health;
        for (auto& sphere:actor->mSphCc) sphere.SetAtAtp(shade::normal_attack_power);
        const dCcD_SrcCps source{daNpc_Kn_c::mCcDSph.mObjInf,
            {{{0,0,0},{0,0,0},30}}};
        for (auto& sweep:entry->bladeSweeps) {
            sweep.Set(source);
            sweep.SetStts(&actor->mCcStts);
            sweep.SetAtType(AT_TYPE_800);
            sweep.SetAtAtp(shade::special_attack_power);
            sweep.SetAtSpl(dCcG_At_Spl_UNK_1);
            sweep.SetAtSe(dCcD_SE_HARD_BODY);
            sweep.OnAtSPrmBit(0xc);
            sweep.OffAtNoConHit();
            sweep.OffCoSetBit();
            sweep.OffAtSetBit();
        }
    }
}
HookAction no_event(ModContext*,void* args,void* result,void*) {
    if (!fighter(mods::arg<daNpc_Kn_c*>(args,0))) return HOOK_CONTINUE;
    *static_cast<int*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
HookAction no_order(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry) return HOOK_CONTINUE;
    if (!entry->divide && !sCinema.active() && !shade::pauses_combat(sBattle.trial)) {
        const int previous_health=sBattle.health;
        sBattle.event(actor->mEvtNo);
        actor->health=sBattle.health;
        if (actor->mEvtNo==11 && sBattle.health<previous_health) {
            // The native reflected-ball success requests a lesson event but
            // no damage animation. Show one short flinch for the accepted hit.
            actor->mFaceMotionSeqMngr.setNo(1,-1,0,0);
            actor->mMotionSeqMngr.setNo(29,0,1,0); // KN_DAMAGE_S -> KN_WAIT_A
            actor->field_0x15bc=0;
        }
        // Recover from the teacher's failure branch without ordering its lecture.
        if ((actor->mEvtNo==1 || actor->mEvtNo==2 || actor->mEvtNo==4) && !sBattle.recovery) {
            entry->reset=true;
        }
    }
    actor->mEvtNo=0;
    actor->mSpeakEvent=false;
    return HOOK_SKIP_ORIGINAL;
}
void select_phase(daNpc_Kn_c* actor,Fighter& entry) {
    const auto& phase=shade::phases[sBattle.phase];
    actor->offDownFlg();
    actor->offHeadLockFlg();
    actor->mType=phase.type;
    actor->mActionMode=entry.divide ? (phase.type==5 ? 14 : 20) : phase.action;
    actor->mMode=1;
    actor->speedF=0;
    actor->speed.zero();
    actor->mTargetPos=actor->current.pos;
    actor->field_0x15bc=0;
    actor->field_0x15bd=0;
    actor->field_0x170c=0;
    actor->field_0x170d=0;
    actor->field_0x16f4.set(1,1,1);
    actor->field_0x15af=1;
    entry.offense=-1;
    entry.attackTicks=0;
    entry.chain.cancel();
    entry.blade={};
    stop_blade_sweeps(entry);
    entry.chain.counters=0;
    entry.swordContact=false;
    entry.helmTurnPending=false;
    entry.cooldown=attack_cooldown(entry);
    entry.reset=false;
}
HookAction before_execute(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry) return HOOK_CONTINUE;
    if (sStopping || entry->deleting || !arena() || !daAlink_getAlinkActorClass()) {
        if (!entry->divide) { stop_shade_music(); release_cinema(); cancel_trial(); }
        stop_blade_sweeps(*entry);
        entry->deleting=true;
        remove_actor(entry->id);
        *static_cast<int*>(result)=1;
        return HOOK_SKIP_ORIGINAL;
    }
    // Freeze trial time and disarm volumes during other events/menus. Preserve
    // progress so an item equip or pause cannot strand a half-finished trial.
    if (dComIfGp_isEnableNextStage() || (dComIfGp_event_runCheck() && !sCinema.active()) ||
        daAlink_getAlinkActorClass()->checkDeadHP()) {
        if (!entry->divide) {
            release_cinema();
            if (dComIfGp_isEnableNextStage() || daAlink_getAlinkActorClass()->checkDeadHP()) {
                stop_shade_music(); cancel_trial();
            }
            else suspend_trial();
        }
        stop_blade_sweeps(*entry);
        *static_cast<int*>(result)=1;
        return HOOK_SKIP_ORIGINAL;
    }
    if (dComIfGp_isPauseFlag() || ui_document_visible()) {
        if (!entry->divide) suspend_trial();
        stop_blade_sweeps(*entry);
        *static_cast<int*>(result)=1;
        return HOOK_SKIP_ORIGINAL;
    }
    if (!entry->divide && sCinema.active()) {
        actor->mType=6;
        actor->field_0x15af=0;
        if (!actor->mCreating) tick_cinema(actor,*entry);
        if (entry->deleting) { *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL; }
        if (sCinema.active()) return HOOK_CONTINUE;
    }
    if (!entry->divide && !sCinema.active() && !sBattle.dying) tick_arena_hazards();
    actor->mType=shade::phases[sBattle.phase].type;
    if (!entry->divide && !actor->mCreating && sBattle.tick()) {
        remove_companions();
        entry->reset=true;
        if (sBattle.trial!=shade::Trial::None) {
            entry->offense=-1; entry->chain.cancel(); entry->helmTurnPending=false;
            entry->animationStarted=false; entry->blade={}; stop_blade_sweeps(*entry);
            actor->offDownFlg(); actor->offHeadLockFlg();
            actor->mCcStts.ClrCcMove();
            if (!shade::pauses_combat(sBattle.trial)) select_phase(actor,*entry);
            else cinema_pose(actor,sBattle.trial==shade::Trial::Fire ? 24 : 0);
            actor->mCylCc.ClrTgHit();
            begin_trial(actor);
        } else if (!sBattle.dying && sBattle.phase>=6) add_doubles(actor);
    }
    if (sBattle.trial!=shade::Trial::None) {
        if (!shade::pauses_combat(sBattle.trial)) {
            if (entry->reset) select_phase(actor,*entry);
            actor->mType=shade::phases[sBattle.phase].type;
            actor->field_0x15af=1;
        } else { actor->mType=6; actor->field_0x15af=0; }
        if (!entry->divide && tick_trial(actor)) {
            finish_trial(); sBattle.trial=shade::Trial::None;
            entry->reset=true;
        }
        return HOOK_CONTINUE;
    }
    if (!entry->divide && sBattle.dying) {
        cancel_trial();
        stop_shade_music();
        if (!pending_or_live(sShadeWolf.id)) sShadeWolf.prepare(actor->current.pos,actor->shape_angle.y);
        sCinema.begin(true);
        entry->offense=-1;
        entry->helmTurnPending=false;
        entry->chain.cancel();
        stop_blade_sweeps(*entry);
        actor->field_0x15af=0;
        return HOOK_CONTINUE;
    }
    if (entry->reset) select_phase(actor,*entry);
    if (sBattle.dying) {
        actor->mType=6;
        if (actor->mActionMode!=23) {
            actor->mActionMode=23;
            actor->mMode=1;
            actor->field_0x170d=0;
            actor->field_0x15af=0;
        }
    }
    if (!entry->divide && sBattle.phase>=6 && !sBattle.dying && !sBattle.recovery) add_doubles(actor);
    // fpcBs_Execute enters this actor's owner layer, so its native KN_BULLET
    // requests inherit the play scene as well as the spawner's state exemption.
    return HOOK_CONTINUE;
}
HookAction draw_cinema(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    const auto* entry=fighter(actor);
    if (!entry || entry->divide) return HOOK_CONTINUE;
    // Native twilight() clears mNoDraw during execute. Enforce cinematic
    // visibility here, after native execution and before either body is queued.
    const bool hidden=entry->deleting ||
        (sCinema.shot==shade::Shot::Request && !sCinema.victory) ||
        sCinema.shot==shade::Shot::Afterglow;
    if (hidden || sShadeWolf.hides_shade()) {
        *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_execute(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    if (!fighter(actor)) return;
    sync_trial_position(actor);
    // All combat animations live in KN_a. Keep the original lesson-7 resource
    // pattern outside execution so Delete releases exactly what create loaded.
    actor->mType=6;
    actor->mEvtNo=0;
    actor->attention_info.flags &= ~(fopAc_AttnFlag_TALK_e|fopAc_AttnFlag_SPEAK_e);
    if (sCinema.active()) actor->attention_info.flags=0;
}
HookAction before_delete(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    if (auto* entry=fighter(actor)) {
        if (!entry->divide) { stop_shade_music(); release_cinema(); cancel_trial(); remove_companions(); }
        stop_blade_sweeps(*entry);
        actor->mType=6;
        *entry={};
    }
    return HOOK_CONTINUE;
}
void after_bullet(ModContext*,void* args,void* result,void*) {
    if (*static_cast<int*>(result)!=cPhs_COMPLEATE_e) return;
    auto* bullet=mods::arg<daObjKnBullet_c*>(args,0);
    if (bullet->parentActorID==sFighters[0].id && sFighters[0].id!=kNone) {
        bullet->mCcSph.SetAtAtp(shade::special_attack_power); // tutorial balls normally have zero damage
    }
}

void after_child(ModContext*,void* args,void* result,void*) {
    if (sFighters[0].id==kNone || mods::arg<s16>(args,0)!=fpcNm_KN_BULLET_e ||
        mods::arg<fpc_ProcID>(args,1)!=sFighters[0].id) return;
    const auto id=*static_cast<fpc_ProcID*>(result);
    if (id!=kNone) sProjectiles.insert(id);
}
HookAction delete_bullet(ModContext*,void* args,void*,void*) {
    auto* bullet=mods::arg<daObjKnBullet_c*>(args,0);
    if (sProjectiles.erase(fopAcM_GetID(bullet))) {
        for (const auto id:bullet->mEmtIds) {
            if (auto* emitter=dComIfGp_particle_getEmitter(id)) {
                emitter->becomeInvalidEmitter();
                emitter->deleteAllParticle();
            }
        }
    }
    return HOOK_CONTINUE;
}

// Native lessons still decide whether Link landed the required counter. Our
// controller owns attack scheduling, so native random attacks cannot starve a
// combo or leave it waiting for sequence 9 after returning to the ready pose.
int waiting_action(const Fighter& entry) {
    const auto& phase=shade::phases[sBattle.phase];
    // 14/20 are formation movement; native doubles actually wait in 15/21.
    return entry.divide ? (phase.type==5 ? 15 : 21) : phase.action;
}
HookAction before_approach(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    if (fighter(actor) && !sBattle.recovery && !sBattle.dying) {
        actor->field_0x15d0=0; // ordinary sword attacks now belong to the combo
    }
    return HOOK_CONTINUE;
}
void after_approach(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry || sBattle.recovery || sBattle.dying) return;
    if (entry->helmTurnPending) {
        // Native approach enables forward speed before its gradual turn has
        // reached Link. After Helm Splitter that produces an arc when Link is
        // behind the landing stance. Keep the native turn, but do it in place.
        const s16 remaining=static_cast<s16>(fopAcM_searchPlayerAngleY(actor)-actor->current.angle.y);
        if (std::abs(static_cast<int>(remaining))>0x400) {
            actor->speedF=0;
            actor->speed.x=actor->speed.z=0;
            return;
        }
        entry->helmTurnPending=false;
    }
    if (actor->speedF>0) actor->speedF=kApproachSpeed;
}

HookAction accessory_motion(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    if (!fighter(actor) || actor->mpPodModel) return HOOK_CONTINUE;
    // The arena creates the lesson-7 heap, which has no sheath model or
    // mPodBck matrix calculator. Native afterSetMotionAnm calls init(modify=true)
    // for Mortal Draw and dereferences that absent calculator. The body motion
    // has already been installed by setMotionAnm; only skip the absent prop.
    actor->mPodAnmFlags=0;
    actor->field_0x15cd=0;
    *static_cast<bool*>(result)=true;
    return HOOK_SKIP_ORIGINAL;
}
void after_motion(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry) return;
    const bool attacking=entry->offense>=0 && actor->mMotionSeqMngr.getNo()==entry->offense &&
        !actor->mMotionSeqMngr.checkEntryNewMotion();
    if (attacking) entry->animationStarted=true;
    const float rate=attacking && entry->offense==shade::sword &&
        actor->mMotionSeqMngr.getStepNo()==0 ? 1.65f : 1.0f;
    for (auto* model:actor->mpModelMorf) if (model) model->setPlaySpeed(rate);
}
void start_attack(daNpc_Kn_c* actor,Fighter& entry) {
    entry.offense=entry.chain.next();
    entry.attackTicks=0;
    entry.animationStarted=false;
    entry.blade={};
    stop_blade_sweeps(entry);
    for (auto& sphere:actor->mSphCc) {
        sphere.OffAtSetBit();
        sphere.ClrAtHit();
    }
    entry.jumpLaunched=false;
    entry.helmTurnPending=false;
    const auto* body=actor->mpModelMorf[0]->getModel()->getAnmMtx(actor->getBackboneJointNo());
    entry.jumpBodyOffset.set(body[0][3]-actor->current.pos.x,0,body[2][3]-actor->current.pos.z);
    const auto offset=actor->current.pos-daPy_getPlayerActorClass()->current.pos;
    entry.orbitAngle=std::atan2(offset.x,offset.z);
    entry.orbitRadius=offset.absXZ();
    actor->speedF=0;
    actor->speed.x=actor->speed.z=0;
    actor->field_0x15bc=0;
    actor->field_0x15ce=0;
    actor->mMotionSeqMngr.setNo(entry.offense,3,1,0);
    actor->mFaceMotionSeqMngr.setNo(1,-1,0,0);
    actor->setAngle(fopAcM_searchPlayerAngleY(actor));
}
void move_toward(daNpc_Kn_c* actor,const cXyz& target,float speed,float stop_distance) {
    const auto delta=target-actor->current.pos;
    const auto velocity=shade::approach_velocity(delta.x,delta.z,speed,stop_distance);
    actor->speedF=0;
    actor->speed.x=velocity.x;
    actor->speed.z=velocity.z;
}
void before_movement(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    if (!fighter(actor) || (sBattle.dying && !cinema_landing()) || actor->speedF!=0) return;
    if (sBattle.recovery && shade::knockdown_landing_motion(actor->mMotionSeqMngr.getNo())<0) return;
    // We decouple movement from facing for the orbit. Native execute therefore
    // chooses posMove, which unlike posMoveF does NOT integrate gravity. Apply
    // it exactly once here, including after a jump is interrupted by a block.
    actor->speed.y=std::max(actor->speed.y+actor->gravity,fopAcM_GetMaxFallSpeed(actor));
}
void hold_recovery(daNpc_Kn_c* actor) {
    // Recovery blocks new attacks, not the physical knockback already in flight.
    // Otherwise the airborne (tilted) pose can be frozen at floor height.
    if (shade::knockdown_landing_motion(actor->mMotionSeqMngr.getNo())>=0) return;
    actor->speedF=0;
    actor->speed.zero();
}
void after_knockdown_movement(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry || entry->deleting || (sBattle.dying && !cinema_landing())) return;
    // Ending Blow already owns its landing/down flag and follow-up window.
    if (!entry->divide && actor->mActionMode==3 && !cinema_landing()) return;
    const int landing=shade::knockdown_landing_motion(actor->mMotionSeqMngr.getNo());
    if (landing<0 || actor->speed.y>0 || !actor->mAcch.ChkGroundHit()) return;
    // This runs after background collision resolves this frame's floor contact,
    // before animation/model calculation. No manual model tilt or Y offset.
    actor->speedF=0;
    actor->speed.zero();
    actor->mTargetPos=actor->current.pos;
    actor->field_0x15bc=0;
    actor->mFaceMotionSeqMngr.setNo(1,-1,0,0);
    actor->mMotionSeqMngr.setNo(landing,-1,0,0);
    actor->setLandingPrtcl();
}
void after_jump_pose(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry || !entry->animationStarted || !shade::jumping_attack(entry->offense) ||
        actor->mMotionSeqMngr.getNo()!=entry->offense || sBattle.recovery || sBattle.dying) return;
    // These demonstration BCKs already contain a jump and forward travel
    // (native Helm Splitter ends 594 units forward). Retain the authored height
    // and rotation, but anchor horizontal body translation to collision-tested
    // actor movement. Update both visible models before sword/cylinder setup.
    const auto* body=actor->mpModelMorf[0]->getModel()->getAnmMtx(actor->getBackboneJointNo());
    const cXyz correction(actor->current.pos.x+entry->jumpBodyOffset.x-body[0][3],0,
                          actor->current.pos.z+entry->jumpBodyOffset.z-body[2][3]);
    for (auto* morf:actor->mpModelMorf) {
        if (!morf) continue;
        auto* model=morf->getModel();
        Mtx base;
        MTXCopy(model->getBaseTRMtx(),base);
        base[0][3]+=correction.x;
        base[2][3]+=correction.z;
        model->setBaseTRMtx(base);
        morf->modelCalc(); // pose only: no animation advance or draw registration
    }
    actor->eyePos+=correction;
    actor->attention_info.position+=correction;
}
void finish_helm_splitter(daNpc_Kn_c* actor,Fighter& entry) {
    if (!entry.animationStarted || entry.offense!=shade::helm_splitter ||
        actor->mMotionSeqMngr.getNo()!=shade::helm_splitter) return;
    // The BCK ends displaced and facing backwards. Our pose hook has already
    // consumed its translation, so keep actor position and transfer only yaw.
    // Install the matching root-neutral stance without blending from the old
    // 594-unit root offset. Native ready/walk blending can resume next tick.
    actor->setAngle(static_cast<s16>(actor->current.angle.y+0x8000));
    actor->mMotionSeqMngr.setNo(6,0,1,0);
    entry.helmTurnPending=true;
}
HookAction before_ending_blow_wait(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    const auto* entry=fighter(actor);
    if (!entry || entry->divide || sBattle.recovery || sBattle.dying) return HOOK_CONTINUE;
    // Only accelerate the actual vulnerable lying window. The native routine
    // still owns fall/landing, the finishing hit, expiry and an in-progress
    // Ending Blow. Seven extra ticks plus its native tick give 8x speed:
    // one quarter of the previously halved window.
    if (actor->mMode==2 && actor->checkDownFlg() &&
        actor->mMotionSeqMngr.getNo()==19 && actor->mMotionSeqMngr.getStepNo()>0 &&
        actor->field_0xdec>1) actor->field_0xdec=std::max(1,actor->field_0xdec-7);
    return HOOK_CONTINUE;
}
HookAction combat_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry) return HOOK_CONTINUE;
    entry->swordContact=false;
    if (sCinema.active() || shade::pauses_combat(sBattle.trial) || !actor->field_0x15af) {
        if (!cinema_landing() || shade::knockdown_landing_motion(actor->mMotionSeqMngr.getNo())<0) {
            actor->speedF=0;
            actor->speed.zero();
        }
        actor->mCcStts.ClrCcMove();
        entry->offense=-1;
        return HOOK_SKIP_ORIGINAL;
    }
    if (sBattle.dying) return HOOK_CONTINUE;
    if (sBattle.recovery) {
        hold_recovery(actor);
        return HOOK_SKIP_ORIGINAL;
    }
    const auto& phase=shade::phases[sBattle.phase];
    auto* player=daPy_getPlayerActorClass();
    if (actor->mCylCc.ChkTgHit() || actor->mCylCc.ChkTgShieldHit()) {
        auto* hit=actor->mCylCc.GetTgHitObj();
        entry->swordContact=actor->mCylCc.ChkTgHit() &&
            actor->mCylCc.GetTgHitAc()==player && hit &&
            hit->ChkAtType(AT_TYPE_NORMAL_SWORD|AT_TYPE_MASTER_SWORD);
        // Resolve blocks, successful Hidden Skills and follow-ups natively
        // before deciding whether to retaliate. A hit is not itself a block.
        entry->offense=-1;
        entry->helmTurnPending=false; // real hit reactions retain their movement
        entry->chain.cancel();
        entry->cooldown=attack_cooldown(*entry);
        return HOOK_CONTINUE;
    }
    if (entry->offense<0) {
        if (entry->cooldown>0) --entry->cooldown;
        if (phase.attack<0 || actor->mMode!=2 || actor->mActionMode!=waiting_action(*entry) ||
            actor->field_0x15bc || !shade::ready_motion(actor->mMotionSeqMngr.getNo(),
                                                      actor->mMotionSeqMngr.getStepNo())) {
            return HOOK_CONTINUE;
        }
        if (entry->cooldown>0) return HOOK_CONTINUE;
        if ((actor->current.pos-player->current.pos).absXZ()>300) return HOOK_CONTINUE;
        if (!entry->chain.remaining) entry->chain.begin(sBattle.phase);
        start_attack(actor,*entry);
    }
    ++entry->attackTicks;
    actor->mCcStts.Move();
    actor->speedF=0;
    actor->speed.x=actor->speed.z=0;
    const int step=actor->mMotionSeqMngr.getStepNo();
    const float frame=actor->mpModelMorf[0]->getFrame();
    const float end=actor->mpModelMorf[0]->getEndFrame();
    const float progress=end>0 ? std::clamp(frame/end,0.0f,1.0f) : 0;
    if (entry->animationStarted) {
        if (entry->offense==shade::back_slice && step<2) {
            // Sidestep and roll follow a semicircle around Link, not a forward
            // drift along the Shade's continually rotating facing direction.
            const auto offset=shade::back_slice_offset(entry->orbitAngle,entry->orbitRadius,step,progress);
            cXyz target=player->current.pos;
            target.x+=offset.x; target.z+=offset.z;
            move_toward(actor,target,14,0);
            actor->setAngle(step==1 && actor->speed.absXZ()>0.01f ?
                cLib_targetAngleY(&actor->current.pos,&target) : fopAcM_searchPlayerAngleY(actor));
        } else if (shade::jumping_attack(entry->offense)) {
            if (!entry->jumpLaunched && progress>=0.2f && actor->mAcch.ChkGroundHit()) {
                entry->jumpLaunched=true;
                entry->jumpTarget=player->current.pos;
                const auto target=shade::jump_landing_target(entry->offense,
                    {actor->current.pos.x,actor->current.pos.z},
                    {player->current.pos.x,player->current.pos.z});
                entry->jumpTarget.x=target.x; entry->jumpTarget.z=target.z;
                actor->setAngle(fopAcM_searchPlayerAngleY(actor));
                // Vertical jump travel is already in the BCK; adding another
                // physical jump lifts the blade above Link at the impact pose.
            }
            if (entry->jumpLaunched && progress<0.75f) move_toward(actor,entry->jumpTarget,12,0);
        } else {
            // Reacquire Link before the Back Slice cut; stop at sword reach.
            // Once the swing is committed, dodging still works.
            if (progress<0.4f || (entry->offense==shade::back_slice && step==2)) {
                actor->setAngle(fopAcM_searchPlayerAngleY(actor));
                move_toward(actor,player->current.pos,10,120);
            }
        }
    }
    const bool finished=entry->animationStarted && !actor->mMotionSeqMngr.checkEntryNewMotion() &&
        (entry->offense==shade::back_slice ? step>=3 :
         (step>0 || actor->mpModelMorf[0]->isStop()));
    const bool landed=!shade::jumping_attack(entry->offense) || !entry->jumpLaunched ||
        (actor->speed.y<=0 && actor->mAcch.ChkGroundHit());
    if ((finished && landed) || entry->attackTicks>=240) {
        if (entry->attackTicks>=240) entry->chain.cancel();
        if (finished && landed) finish_helm_splitter(actor,*entry);
        entry->offense=-1;
        entry->cooldown=entry->chain.remaining ? kComboGap : attack_cooldown(*entry);
        actor->speedF=0;
        actor->speed.x=actor->speed.z=0;
        actor->mMode=1;
    }
    return HOOK_SKIP_ORIGINAL;
}
void after_combat_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry || !entry->swordContact || sBattle.dying || sBattle.recovery) return;
    entry->swordContact=false;
    // Sequence 27 is the native sword-block reaction. Damage, Shield Attack
    // openings, head-locks and other successful lesson counters are excluded.
    if (actor->mEvtNo || actor->mActionMode!=waiting_action(*entry) ||
        actor->mMotionSeqMngr.getNo()!=27 || !actor->mMotionSeqMngr.checkEntryNewMotion()) return;
    entry->chain.blocked_sword();
    if (entry->chain.counters) {
        entry->chain.cancel();
        entry->chain.begin(sBattle.phase);
        start_attack(actor,*entry); // second block: sword riposte, then Back Slice
    }
}
void trial_body_collision(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    if (fighter(actor) && sBattle.trial!=shade::Trial::None) {
        // Keep movement/body separation and offensive sword colliders. Only
        // trial-specific targets accept hits during protection, so native hit
        // reactions cannot strand the offensive controller in a lesson state.
        actor->mCylCc.OffTgSetBit(); actor->mCylCc.ClrTgHit();
    }
}
HookAction sword_collision(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daNpc_Kn_c*>(args,0);
    auto* entry=fighter(actor);
    if (!entry) return HOOK_CONTINUE;
    if (sCinema.active() || shade::pauses_combat(sBattle.trial) || !actor->field_0x15af) {
        for (auto& sphere:actor->mSphCc) { sphere.OffAtSetBit(); sphere.ClrAtHit(); }
        entry->blade={};
        stop_blade_sweeps(*entry);
        return HOOK_SKIP_ORIGINAL;
    }
    if (entry->offense<0 && !sBattle.recovery && !sBattle.dying) {
        for (auto& sphere:actor->mSphCc) sphere.SetAtAtp(shade::normal_attack_power);
        entry->blade={};
        stop_blade_sweeps(*entry);
        return HOOK_CONTINUE;
    }
    // Called after playAllAnm / setAttnPos / modelCalc: these are the same
    // posed joints as the visible sword, not last tick's pose or a timer proxy.
    std::array<cXyz,2> positions;
    std::array<shade::BladePoint,2> points;
    for (unsigned i=0;i<positions.size();++i) {
        cXyz offset(60.0f+60*i,0,0);
        MTXMultVec(actor->mpModelMorf[0]->getModel()->getAnmMtx(13),&offset,&positions[i]);
        points[i]={positions[i].x,positions[i].y,positions[i].z};
        auto& sphere=actor->mSphCc[i];
        // All contact volumes share one strike budget, including shield hits.
        if ((sphere.ChkAtHit() || sphere.ChkAtShieldHit()) &&
            sphere.GetAtHitAc()==daPy_getPlayerActorClass()) entry->blade.contact();
        auto& sweep=entry->bladeSweeps[i];
        if ((sweep.ChkAtHit() || sweep.ChkAtShieldHit()) &&
            sweep.GetAtHitAc()==daPy_getPlayerActorClass()) entry->blade.contact();
    }
    const auto previous=entry->blade.previous;
    const bool attacking=entry->offense>=0 && entry->animationStarted &&
        actor->mMotionSeqMngr.getNo()==entry->offense &&
        !sBattle.recovery && !sBattle.dying && !entry->deleting;
    bool active=false;
    if (attacking) {
        bool clear_of_target=true;
        for (const auto& target:daAlink_getAlinkActorClass()->mTgCyls) {
            const auto& center=target.GetC();
            clear_of_target &= shade::blade_clear_of_body(points,
                {center.x,center.y,center.z},target.GetR(),target.GetH());
        }
        active=entry->blade.sample(entry->offense,actor->mMotionSeqMngr.getStepNo(),
                                   actor->mpModelMorf[0]->getFrame(),points,clear_of_target);
    } else entry->blade={};
    for (unsigned i=0;i<positions.size();++i) {
        auto& sphere=actor->mSphCc[i];
        if (active) {
            sphere.SetC(positions[i]);
            sphere.SetR(30); // native blade coverage; no delayed area-damage proxy
            sphere.SetAtAtp(shade::attack_power(entry->offense));
            sphere.OnAtSetBit();
            dComIfG_Ccsp()->Set(&sphere);
        } else sphere.OffAtSetBit();
        sphere.ClrAtHit();
        auto& sweep=entry->bladeSweeps[i];
        if (active && entry->blade.sweep && entry->offense!=shade::sword) {
            // Trace the actual blade points between consecutive poses. A fast
            // cut can cross Link entirely between two endpoint sphere tests.
            const cXyz start(previous[i].x,previous[i].y,previous[i].z);
            static_cast<cM3dGCps*>(&sweep)->Set(start,positions[i],30);
            cXyz direction=positions[i]-start;
            sweep.SetAtVec(direction);
            sweep.SetAtAtp(shade::special_attack_power);
            sweep.OnAtSetBit();
            dComIfG_Ccsp()->Set(&sweep);
        } else sweep.OffAtSetBit();
        sweep.ClrAtHit();
    }
    return HOOK_SKIP_ORIGINAL;
}

// Original mesh and original bundled stone. Image storage belongs to the mod,
// not the room archive; the texture object and draw packet live with Pedestal.
class PlinthPacket : public J3DPacket {
public:
    cXyz position{0,0,0};
    GXTexObj texture{};
    bool textureReady=false;

    void draw() override {
        if (!textureReady) {
            GXInitTexObj(&texture,pedestal_art::kPixels,pedestal_art::kSize,pedestal_art::kSize,
                GX_TF_RGB565,GX_REPEAT,GX_REPEAT,GX_TRUE);
            GXInitTexObjLOD(&texture,GX_LIN_MIP_LIN,GX_LINEAR,0,pedestal_art::kLastMip,
                0,GX_FALSE,GX_FALSE,GX_ANISO_1);
            textureReady=true;
        }
        j3dSys.reinitGX();
        GXLoadPosMtxImm(j3dSys.getViewMtx(),GX_PNMTX0);
        GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);
        GXLoadTexObj(&texture,GX_TEXMAP0);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_MODULATE);
        GXSetZMode(GX_TRUE,GX_LEQUAL,GX_TRUE);
        GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
        GXSetCullMode(GX_CULL_NONE);
        using namespace pedestal_mesh;
        GXBegin(GX_QUADS,GX_VTXFMT0,kBands*8*4);
        for (unsigned ring=0;ring<kBands;++ring) for (unsigned side=0;side<8;++side) {
            const bool cap=kRings[ring].height==kRings[ring+1].height;
            const float light=cap ? 1.0f : 0.82f+0.12f*std::cos((side+0.5f)*0.7853981634f-0.7f);
            const auto finish=kRings[ring].finish;
            const float red=finish==trim ? 255 : finish==groove ? 100 : 222;
            const float green=finish==trim ? 225 : finish==groove ? 104 : 225;
            const float blue=finish==trim ? 171 : finish==groove ? 104 : 226;
            for (unsigned corner=0;corner<4;++corner) {
                const auto& point=kRings[ring+(corner>=2)];
                const bool right=corner==1 || corner==2;
                const float a=(side+right)*0.7853981634f;
                const float x=std::sin(a)*point.radius, z=std::cos(a)*point.radius;
                GXPosition3f32(position.x+x,position.y+point.height,position.z+z);
                GXColor4u8(static_cast<u8>(red*light),static_cast<u8>(green*light),static_cast<u8>(blue*light),255);
                // One world-space grain scale: planar cap/ledge, face-local
                // tangent on walls/bevels. Adjacent height bands share UVs.
                if (cap) GXTexCoord2f32(0.5f+x/160,0.5f+z/160);
                else GXTexCoord2f32(0.5f+(right ? 1 : -1)*point.radius*0.3826834324f/160,
                    (62-point.height)/160);
            }
        }
        GXEnd();
        j3dSys.reinitGX();
    }
};
// Lesson particle banks are not necessarily present in the Darknut room.
// Keep the real KN_BULLET collision/reflection and provide an asset-free orb
// when its native emitter is unavailable. This is one reusable scene packet.
class EnergyPacket : public J3DPacket {
public:
    cXyz position{0,0,0};
    bool reflected=false;
    void draw() override {
        j3dSys.reinitGX();
        GXLoadPosMtxImm(j3dSys.getViewMtx(),GX_PNMTX0);
        GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
        GXSetZMode(GX_TRUE,GX_LEQUAL,GX_FALSE);
        GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_ONE,GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
        GXSetCullMode(GX_CULL_NONE);
        for (unsigned shell=0;shell<3;++shell) {
            const float radius=12.0f+10.0f*shell;
            const u8 alpha=shell==0 ? 210 : shell==1 ? 65 : 22;
            GXBegin(GX_QUADS,GX_VTXFMT0,8*16*4);
            for (unsigned band=0;band<8;++band) for (unsigned side=0;side<16;++side) {
                for (unsigned corner=0;corner<4;++corner) {
                    const float latitude=-1.5707963268f+(band+(corner>=2))*0.3926990817f;
                    const float longitude=(side+(corner==1 || corner==2))*0.3926990817f;
                    const float ring=radius*std::cos(latitude);
                    GXPosition3f32(position.x+ring*std::sin(longitude),position.y+radius*std::sin(latitude),position.z+ring*std::cos(longitude));
                    GXColor4u8(reflected ? 180 : 80,210,255,alpha);
                }
            }
            GXEnd();
        }
        j3dSys.reinitGX();
    }
};
#include "heroes_shade_trials.inc"

class Pedestal : public fopAc_ac_c {
public:
    request_of_phase_process_class phase{};
    J3DModel* sword=nullptr;
    mDoExt_btkAnm btk;
    mDoExt_brkAnm brk;
    PlinthPacket plinth;
    EnergyPacket energy;
    dCcD_Stts collisionStatus;
    dCcD_Cyl collision;
    ShadeTrials trials;
    bool ready=false;
    ~Pedestal() { dComIfG_resDelete(&phase,kSwordArchive); }
    static int heap(fopAc_ac_c* actor) {
        auto* self=static_cast<Pedestal*>(actor);
        auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes(kSwordArchive,dRes_INDEX_MSTRSWORD_BMD_O_AL_SWM_e));
        if (!data) return 0;
        self->sword=mDoExt_J3DModel__create(data,0x80000,0x11000284);
        auto* btk=static_cast<J3DAnmTextureSRTKey*>(dComIfG_getObjectRes(kSwordArchive,dRes_INDEX_MSTRSWORD_BTK_O_AL_SWM_e));
        auto* brk=static_cast<J3DAnmTevRegKey*>(dComIfG_getObjectRes(kSwordArchive,dRes_INDEX_MSTRSWORD_BRK_O_AL_SWM_e));
        return self->sword && btk && brk &&
            self->btk.init(data,btk,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1) &&
            self->brk.init(data,brk,TRUE,J3DFrameCtrl::EMode_LOOP,1,0,-1);
    }
};
int create_pedestal(void* ptr) {
    auto* self=static_cast<Pedestal*>(ptr);
    fopAcM_ct(self,Pedestal);
    if (self->ready) return cPhs_COMPLEATE_e;
    const auto phase=dComIfG_resLoad(&self->phase,kSwordArchive);
    if (phase!=cPhs_COMPLEATE_e) return phase;
    if (!fopAcM_entrySolidHeap(self,Pedestal::heap,0x4000)) return cPhs_ERROR_e;
    float minY=0;
    auto* data=self->sword->getModelData();
    if (data->getShapeNum()) {
        minY=data->getShapeNodePointer(0)->getMin()->y;
        for (u16 i=1;i<data->getShapeNum();++i) minY=std::min(minY,data->getShapeNodePointer(i)->getMin()->y);
    }
    mDoMtx_stack_c::transS(self->current.pos.x,self->current.pos.y+34-minY,self->current.pos.z);
    self->sword->setBaseTRMtx(mDoMtx_stack_c::get());
    fopAcM_SetMtx(self,self->sword->getBaseTRMtx());
    self->plinth.position=self->current.pos;
    self->attention_info.flags=0;
    self->collisionStatus.Init(255,0,self);
    self->collision.SetStts(&self->collisionStatus);
    self->collision.SetCoSPrm(0x79);
    self->collision.SetC(self->current.pos);
    self->collision.SetR(90);
    self->collision.SetH(62);
    self->trials.init(self);
    sTrials=&self->trials;
    self->ready=true;
    return cPhs_COMPLEATE_e;
}
int delete_pedestal(void* ptr) {
    sStopping=true;
    stop_shade_music();
    release_cinema();
    remove_companions();
    remove_actor(sFighters[0].id);
    sPedestal=kNone;
    sTrials=nullptr;
    static_cast<Pedestal*>(ptr)->~Pedestal();
    return 1;
}
int execute_pedestal(void* ptr) {
    auto* self=static_cast<Pedestal*>(ptr);
    if (!self->ready) return 1;
    self->btk.play(); self->brk.play();
    dComIfG_Ccsp()->Set(&self->collision);
    auto* player=daAlink_getAlinkActorClass();
    if (!arena() || !player || dComIfGp_isEnableNextStage() || dComIfGp_event_runCheck() ||
        dComIfGp_isPauseFlag() || ui_document_visible() || player->checkDeadHP() || player->checkWolf()) return 1;
    // Do both independently: loading effects must not delay wall-target spawn.
    const bool targetsReady=self->trials.place();
    const bool effectsReady=self->trials.prepare_effects();
    if (!targetsReady || !effectsReady) return 1;
    const bool endingWind=sBattle.trial==shade::Trial::Wind &&
        self->trials.clock.kind==shade::Trial::Wind && !self->trials.clock.windReleased &&
        !sCinema.active() && pending_or_live(sFighters[0].id);
    if (!endingWind) {
        for (const auto& entry:sFighters) if (pending_or_live(entry.id)) return 1;
        if (sCinema.active()) release_cinema(); // asynchronous creation failed
    }
    const cXyz delta=player->current.pos-self->current.pos;
    const s16 facing=static_cast<s16>(cLib_targetAngleY(&player->current.pos,&self->current.pos)-player->shape_angle.y);
    if (delta.absXZ()>230 || std::abs(delta.y)>100 || std::abs(static_cast<int>(facing))>0x3000) return 1;
    dComIfGp_setDoStatusForce(8,0); // native A: Pull
    if (!mDoCPd_c::getTrigA(PAD_1)) return 1;
    if (endingWind) {
        // The Shade execute hook completes the trial; preserve HP, phase and
        // the existing fighter. Never start a second encounter from this A press.
        self->trials.clock.release_wind();
        mDoCPd_c::getCpadInfo(PAD_1).mPressedButtonFlags &= ~PAD_BUTTON_A;
        return 1;
    }
    sStopping=false;
    sBattle={};
    sFighters[0]={};
    cXyz pos=self->current.pos;
    pos.z-=450;
    cXyz probe=pos; probe.y+=500;
    dBgS_ObjGndChk ground;
    ground.SetPos(&probe);
    const float height=dComIfG_Bgsp().GroundCross(&ground);
    if (height==-1.0e9f) return 1;
    pos.y=height;
    if (spawn(fpcNm_NPC_KN_e,pos,cLib_targetAngleY(&pos,&player->current.pos),kShadeParams,sFighters[0].id)==MOD_OK) {
        sShadeWolf.clear();
        sShadeWolf.prepare(pos,cLib_targetAngleY(&pos,&player->current.pos));
        sCinema.begin(false);
        mDoCPd_c::getCpadInfo(PAD_1).mPressedButtonFlags &= ~PAD_BUTTON_A;
    }
    return 1;
}
int draw_pedestal(void* ptr) {
    auto* self=static_cast<Pedestal*>(ptr);
    if (!self->ready) return 1;
    g_env_light.settingTevStruct(0x10,&self->current.pos,&self->tevStr);
    g_env_light.setLightTevColorType_MAJI(self->sword,&self->tevStr);
    auto* data=self->sword->getModelData();
    self->btk.entry(data); self->brk.entry(data);
    mDoExt_modelUpdateDL(self->sword);
    self->btk.remove(data); self->brk.remove(data);
    dComIfGd_getOpaList()->entryImm(&self->plinth,0);
    self->trials.draw(self->tevStr);
    for (const auto id:sProjectiles) {
        auto* actor=actor_by_id(id);
        if (!actor || fopAcM_GetName(actor)!=fpcNm_KN_BULLET_e) continue;
        auto* bullet=static_cast<daObjKnBullet_c*>(actor);
        if (!dComIfGp_particle_getEmitter(bullet->mEmtIds[0])) {
            self->energy.position=bullet->current.pos;
            self->energy.reflected=bullet->getActionMode()==2;
            dComIfGd_setList();
            j3dSys.getDrawBuffer(1)->entryImm(&self->energy,0);
        }
        break; // the native throw owns at most one ball at a time
    }
    return 1;
}
int can_delete(void*) { return 1; }
} // namespace

ModResult initialize_heroes_shade_encounter(ModError* error) {
    ModResult result=MOD_OK;
    if ((result=register_cinema_lines())!=MOD_OK) goto fail;
#define PRE(H,F) if ((result=mods::hook::add_pre<H>(svc_hook,F))!=MOD_OK) goto fail
#define POST(H,F) if ((result=mods::hook::add_post<H>(svc_hook,F))!=MOD_OK) goto fail
    PRE(ShadeMusicFrameworkHook,before_music_framework);
    PRE(ShadeAdmissionHook,admit);
    PRE(ShadeResetHook,before_reset); POST(ShadeResetHook,after_reset);
    PRE(ShadeExecuteHook,before_execute); POST(ShadeExecuteHook,after_execute);
    PRE(ShadeDeleteHook,before_delete);
    PRE(ShadeEventHook,no_event); PRE(ShadeOrderHook,no_order);
    PRE(ShadeDrawHook,draw_cinema);
    PRE(ShadeWolfMainHook,wolf_main);
    PRE(ShadeWolfExecuteHook,wolf_execute);
    PRE(ShadeWolfDrawHook,wolf_draw);
    PRE(ShadeActionHook,combat_action); POST(ShadeActionHook,after_combat_action);
    PRE(ShadeEndingBlowHook,before_ending_blow_wait);
    PRE(ShadeSwordHook,sword_collision);
    POST(ShadeBodyHook,trial_body_collision);
    PRE(ShadeAccessoryMotionHook,accessory_motion);
    POST(ShadeMotionHook,after_motion);
    POST(ShadeMovementHook,before_movement);
    POST(ShadeLandingHook,after_knockdown_movement);
    POST(ShadeJumpPoseHook,after_jump_pose);
    PRE(ShadeApproachHook,before_approach); POST(ShadeApproachHook,after_approach);
    POST(ShadeBulletHook,after_bullet);
    PRE(ShadeBulletDeleteHook,delete_bullet);
    POST(ShadeChildHook,after_child);
#undef PRE
#undef POST
    {
        const ActorProfileDesc desc{
            .name="DLShade", .priority_group=3, .process_size=sizeof(Pedestal),
            .draw_priority=fpcDwPi_Obj_MasterSword_e, .status=fopAcStts_UNK_0x40000_e,
            .group=fopAc_ACTOR_e, .cull_type=fopAc_CULLBOX_CUSTOM_e,
            .create_function=create_pedestal,.delete_function=delete_pedestal,
            .execute_function=execute_pedestal,.is_delete_function=can_delete,.draw_function=draw_pedestal,
        };
        result=svc_actor->register_actor(mod_ctx,&desc,&sProfile,&sRegistration);
        if (result==MOD_OK) return MOD_OK;
    }
fail:
    shutdown_heroes_shade_encounter();
    return mods::set_error(error,result,"failed to initialize Hero's Shade arena encounter");
}
void update_heroes_shade_arena() {
    if (sRegistration==0 || !arena() || dComIfGp_isEnableNextStage()) return;
    if (pending_or_live(sPedestal)) return;
    for (auto& entry:sFighters) if (!pending_or_live(entry.id)) entry={};
    if (pending_or_live(sFighters[0].id)) return;
    auto* player=daAlink_getAlinkActorClass();
    if (!player) return;
    cXyz probe(0,player->current.pos.y+1000,0);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float height=dComIfG_Bgsp().GroundCross(&ground);
    if (height==-1.0e9f) return;
    sStopping=false;
    spawn(sProfile,cXyz(0,height,0),0,0,sPedestal);
}
void shutdown_heroes_shade_encounter() {
    sStopping=true;
    const auto wolfId=sShadeWolf.id;
    stop_shade_music();
    release_cinema();
    std::unordered_set<ActorId> owned=sProjectiles;
    owned.insert(wolfId);
    for (const auto& entry:sFighters) if (entry.id!=kNone) owned.insert(entry.id);
    owned.insert(sPedestal);
    if (sTrials) {
        cancel_trial();
        for (const auto id:sTrials->anchors) owned.insert(id);
    }
    remove_companions();
    for (const auto id:owned) remove_actor(id);
    // Finish only our deletion tags, not the engine's whole deletion queue.
    // Native actors are not automatically drained by unregistering DLShade.
    auto* previousLayer=fpcLy_CurrentLayer();
    for (auto* node=g_fpcDtTg_Queue.mpHead;node;) {
        auto* next=NODE_GET_NEXT(node);
        auto* tag=reinterpret_cast<delete_tag_class*>(node);
        auto* process=static_cast<base_process_class*>(tag->base.mpTagData);
        if (owned.contains(fpcM_GetID(process))) {
            tag->timer=0;
            fpcDtTg_Do(tag,[](void* proc) { return fpcDt_deleteMethod(static_cast<base_process_class*>(proc)); });
        }
        node=next;
    }
    fpcLy_SetCurrentLayer(previousLayer);
    if (sRegistration) {
        if (svc_actor->unregister_actor(mod_ctx,sRegistration)!=MOD_OK) return;
        sRegistration=0; sProfile=-1;
    }
    sFighters={};
    sProjectiles.clear();
    sPedestal=kNone;
    sShadeWolf.id=kNone;
    for (auto& line:sLines) line.reset();
    sBossTitle.reset();
    mods::hook::uninstall<ShadeMusicFrameworkHook>(svc_hook);
    mods::hook::uninstall<ShadeAdmissionHook>(svc_hook);
    mods::hook::uninstall<ShadeResetHook>(svc_hook);
    mods::hook::uninstall<ShadeExecuteHook>(svc_hook);
    mods::hook::uninstall<ShadeDrawHook>(svc_hook);
    mods::hook::uninstall<ShadeWolfMainHook>(svc_hook);
    mods::hook::uninstall<ShadeWolfExecuteHook>(svc_hook);
    mods::hook::uninstall<ShadeWolfDrawHook>(svc_hook);
    mods::hook::uninstall<ShadeDeleteHook>(svc_hook);
    mods::hook::uninstall<ShadeEventHook>(svc_hook);
    mods::hook::uninstall<ShadeOrderHook>(svc_hook);
    mods::hook::uninstall<ShadeActionHook>(svc_hook);
    mods::hook::uninstall<ShadeEndingBlowHook>(svc_hook);
    mods::hook::uninstall<ShadeApproachHook>(svc_hook);
    mods::hook::uninstall<ShadeAccessoryMotionHook>(svc_hook);
    mods::hook::uninstall<ShadeMotionHook>(svc_hook);
    mods::hook::uninstall<ShadeMovementHook>(svc_hook);
    mods::hook::uninstall<ShadeLandingHook>(svc_hook);
    mods::hook::uninstall<ShadeJumpPoseHook>(svc_hook);
    mods::hook::uninstall<ShadeSwordHook>(svc_hook);
    mods::hook::uninstall<ShadeBodyHook>(svc_hook);
    mods::hook::uninstall<ShadeBulletHook>(svc_hook);
    mods::hook::uninstall<ShadeBulletDeleteHook>(svc_hook);
    mods::hook::uninstall<ShadeChildHook>(svc_hook);
}
} // namespace dawnlight
