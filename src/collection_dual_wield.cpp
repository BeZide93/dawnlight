#include "collection_dual_wield.hpp"
#include "collection_dual_wield_state.hpp"
#include "config.hpp"
#include "save_state.hpp"
#include "service_imports.hpp"
#include "d/d_menu_collect.h"
#include "d/d_menu_window.h"
#include "d/d_select_cursor.h"
#include "d/d_pane_class.h"
#include "d/d_lib.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/actor/d_a_alink.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_audio.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "mods/svc/hook.hpp"
#include <cstdio>
#include <vector>

namespace dawnlight {
namespace {
DEFINE_HOOK(&dMenu_Collect2D_c::screenSet, CollectionSwordScreen);
DEFINE_HOOK(&dMenu_Collect2D_c::_delete, CollectionSwordDelete);
DEFINE_HOOK(&dMenu_Collect2D_c::menuCollectWide, CollectionSwordLayout);
DEFINE_HOOK(&dMenu_Collect2D_c::_draw, CollectionSwordDraw);
DEFINE_HOOK(&dMenu_Collect2D_c::wait_proc, CollectionSwordWait);
DEFINE_HOOK(&dMenu_Collect2D_c::cursorMove, CollectionSwordNavigate);
DEFINE_HOOK(&dMenu_Collect2D_c::pointerWait, CollectionSwordPointer);
DEFINE_HOOK(&dMenu_Collect2D_c::pointerActivateCurrent, CollectionSwordClick);
DEFINE_HOOK(&dMenu_Collect2D_c::setItemNameString, CollectionSwordName);
DEFINE_HOOK(&dMenu_Collect2D_c::changeShield, CollectionSwordShield);
DEFINE_HOOK(&J2DScreen::draw, CollectionSwordCursorScreen);

collection::Selection s_selection;
SaveObserverHandle s_saveObserver=0;
ConfigSubscriptionHandle s_settingObserver=0;
struct PointerApi {
    void (*begin)(int)=nullptr;
    bool (*hit)(CPaneMgr*,float)=nullptr;
    void (*hover)(u16)=nullptr;
    bool (*click)()=nullptr;
} s_pointer;
struct Menu {
    dMenu_Collect2D_c* owner=nullptr;
    JKRSolidHeap* heap=nullptr;
    J2DScreen* screen=nullptr;
    J2DPicture* icon=nullptr;
    CPaneMgr* iconPane=nullptr;
    J2DPicture* frame=nullptr;
    struct FrameTint {J2DPicture* pane;JUtility::TColor color;};
    std::vector<FrameTint> frameTints;
    struct DecorationPosition {J2DPane* pane;float x,y;};
    std::vector<DecorationPosition> decorations;
    bool drawing=false, drawn=false;
    std::vector<collection::Cell> cells;
    collection::Placement placement;
    bool focused=false, visible=false;
    u8 anchorX=0,anchorY=0;
    void restore_tints() {
        for(const auto& tint:frameTints) tint.pane->setWhite(tint.color);
        for(const auto& saved:decorations) saved.pane->translate(saved.x,saved.y);
        frameTints.clear();decorations.clear();drawing=drawn=false;
    }
    void release() {
        restore_tints();
        JKR_DELETE(iconPane);JKR_DELETE(screen);
        iconPane=nullptr;screen=nullptr;icon=frame=nullptr;
        if(heap) mDoExt_destroySolidHeap(heap);
        heap=nullptr;owner=nullptr;focused=visible=false;cells.clear();
    }
} s_menu;

void restore_name(dMenu_Collect2D_c* menu);

const char* blob_name() {return save_state_boss_rush_active()?"bossrush-dual-wield":"dual-wield";}
void load_selection(ModContext*,uint32_t,void*) {
    std::array<unsigned char,2> data{};size_t size=data.size();
    if(svc_save->get_blob(mod_ctx,blob_name(),data.data(),&size)!=MOD_OK || size!=data.size()) size=0;
    s_selection.load({data.data(),std::min(size,data.size())},save_state_boss_rush_active());
    if(s_menu.owner && s_menu.focused) restore_name(s_menu.owner);
}
void select_sword(bool selected) {
    s_selection.chosen=selected;s_selection.boss=save_state_boss_rush_active();
    const auto data=s_selection.encode();
    if(svc_save->set_blob(mod_ctx,blob_name(),data.data(),data.size())!=MOD_OK)
        svc_log->warn(mod_ctx,"Dual Wield: selection applies this session; no active save storage");
}
void setting_changed(ModContext*,ConfigVarHandle,const ConfigVarValue* value,const ConfigVarValue*,void*) {
    if(value && !value->bool_value) {
        select_sword(false);
        if(s_menu.owner && s_menu.focused) restore_name(s_menu.owner);
    }
}
J2DPicture* picture(J2DScreen* screen,u64 tag) {
    auto* p=screen?screen->search(tag):nullptr;
    return p && p->getTypeID()==18 ? static_cast<J2DPicture*>(p):nullptr;
}
const ResTIMG* texture(J2DPicture* pic) {
    return pic && pic->getTexture(0)?pic->getTexture(0)->getTexInfo():nullptr;
}
bool visible(J2DPane* pane) {
    for(auto* p=pane;p;p=p->getParentPane()) if(!p->isVisible()) return false;
    return pane!=nullptr;
}
collection::Cell bounds(dMenu_Collect2D_c* menu,J2DPane* pane,int x,int y,bool available) {
    Mtx matrix;
    const Vec a=menu->mpLinkPm->getGlobalVtx(pane,&matrix,0,false,0);
    const Vec b=menu->mpLinkPm->getGlobalVtx(pane,&matrix,3,false,0);
    return {x,y,std::min(a.x,b.x),std::min(a.y,b.y),std::fabs(b.x-a.x),std::fabs(b.y-a.y),available};
}
void restore_name(dMenu_Collect2D_c* menu) {
    s_menu.focused=false;
    menu->setItemNameString(menu->mCursorX,menu->mCursorY);
}
bool own_focus(dMenu_Collect2D_c* menu) {
    return s_menu.owner==menu && s_menu.visible && s_menu.focused &&
        menu->mCursorX==s_menu.anchorX && menu->mCursorY==s_menu.anchorY;
}
void text(J2DScreen* screen,u64 tag,const char* value) {
    auto* p=screen->search(tag);if(!p || p->getTypeID()!=19) return;
    auto* box=static_cast<J2DTextBox*>(p);
    // Native text buffers are already allocated on the menu heap. Reallocating
    // strings every draw would exhaust a solid heap and disrupt other mods.
    auto buffer=box->getStringPtr();
    if(buffer.buffer && buffer.size) std::snprintf(buffer.buffer,buffer.size,"%s",value);
}
void show_name(dMenu_Collect2D_c* menu) {
    if(!own_focus(menu)) return;
    for(u64 tag:{MULTI_CHAR('item_n00'),MULTI_CHAR('item_n01'),MULTI_CHAR('item_n02'),MULTI_CHAR('item_n03'),
                 MULTI_CHAR('item_n04'),MULTI_CHAR('item_n05'),MULTI_CHAR('item_n06'),MULTI_CHAR('item_n07')})
        text(menu->mpScreen,tag,"Ordon Sword");
    const char* description=dual_wield_equipped()?"Dual Wield equipped. Select a shield to return to shield combat.":
        "Equip a second sword for Dual Wield. Requires an equipped shield.";
    for(u64 tag:{MULTI_CHAR('i_text0'),MULTI_CHAR('i_text1'),MULTI_CHAR('f_text0'),MULTI_CHAR('f_text1')})
        text(menu->mpScreen,tag,description);
    // The ordinary description renderer must not replace our text with the
    // anchor shield's message. The native grid itself remains untouched.
    menu->mItemNameString=0;
    menu->setAButtonString(menu->mIsWolf?0:0x436);
}
void layout(dMenu_Collect2D_c* menu) {
    if(s_menu.owner!=menu || !s_menu.screen || !menu->mpScreen || !menu->mpLinkPm) return;
    s_menu.cells.clear();
    for(int y=0;y<6;++y) for(int x=0;x<7;++x) {
        const auto tag=menu->getItemTag(x,y,true);if(!tag) continue;
        auto* pane=menu->mpScreen->search(tag);
        if(!visible(pane)) continue;
        s_menu.cells.push_back(bounds(menu,pane,x,y,menu->field_0x22d[x][y]!=0 || y==5));
    }
    // Always include the Hylian Shield's reserved place, even before acquisition.
    auto* hylian=menu->mpScreen->search(MULTI_CHAR('tate_n1'));
    if(hylian && visible(hylian->getParentPane())) {
        auto c=bounds(menu,hylian,-1,1,false);
        bool found=false;for(const auto& old:s_menu.cells) if(old.y==1&&std::fabs(old.left-c.left)<1) found=true;
        if(!found) s_menu.cells.push_back(c);
    }
    s_menu.placement=collection::append_shield(s_menu.cells);
    const auto p=s_menu.placement;
    const bool onScreen=p.left>=mDoGph_gInf_c::getMinXF() && p.left+p.size<=mDoGph_gInf_c::getMaxXF();
    const bool wasVisible=s_menu.visible;
    s_menu.visible=dual_wield_enabled() && menu->mProcess==0 && menu->mSubWindowOpenCheck==0 && s_menu.placement.neighbor>=0 && onScreen;
    if(s_menu.focused && ((!s_menu.visible && wasVisible) || menu->mCursorX!=s_menu.anchorX || menu->mCursorY!=s_menu.anchorY))
        restore_name(menu);
    if(!s_menu.visible) return;
    s_menu.screen->setAlpha(menu->mpLinkPm->getAlpha());
    s_menu.icon->move(p.left,p.top);s_menu.icon->resize(p.size,p.size);
    s_menu.frame->move(p.left-3,p.top-3);s_menu.frame->resize(p.size+6,p.size+6);
    if(const auto* frame=texture(picture(menu->mpScreen,MULTI_CHAR('tate_g_1'))))
        if(texture(s_menu.frame)!=frame) s_menu.frame->changeTexture(frame,0);

    if(own_focus(menu)) show_name(menu);
}
void focus(dMenu_Collect2D_c* menu) {
    if(own_focus(menu)) return;
    // Reuse a shield's cursor styling even when the pointer came from Save or
    // another oversized cell. Do not change any grid entries or pane pointers.
    const int last=collection::neighbor(s_menu.cells,s_menu.placement,collection::Direction::Left);
    if(last>=0 && s_menu.cells[last].y==1) {
        menu->mCursorX=s_menu.cells[last].x;menu->mCursorY=1;
    }
    menu->cursorPosSet();
    mDoAud_seStart(Z2SE_SY_CURSOR_ITEM,nullptr,0,0);
    s_menu.focused=true;s_menu.anchorX=menu->mCursorX;s_menu.anchorY=menu->mCursorY;
    show_name(menu);
}
bool can_equip(dMenu_Collect2D_c* menu) {
    auto* link=daAlink_getAlinkActorClass();
    return !menu->mIsWolf && link && link->getShieldChangeWaitTimer()==0;
}
void activate(dMenu_Collect2D_c* menu) {
    auto* link=daAlink_getAlinkActorClass();
    if(!can_equip(menu) || dComIfGs_getSelectEquipShield()==dItemNo_NONE_e) return;
    if(dual_wield_equipped()) return;
    select_sword(true);link->setShieldChange();dMeter2Info_set2DVibration();
    mDoAud_seStart(Z2SE_SY_ITEM_SET_X,nullptr,0,0);show_name(menu);
}
HookAction capture_icon(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(s_menu.owner==menu && s_menu.screen) return HOOK_CONTINUE;
    s_menu.release();
    // Read the game's Ordon icon before starter-equipment mods change its pane.
    const auto* icon=texture(picture(menu->mpScreen,MULTI_CHAR('ken_01')));
    const auto* frame=texture(picture(menu->mpScreen,MULTI_CHAR('tate_g_1')));
    if(!icon||!frame) return HOOK_CONTINUE;
    s_menu.heap=mDoExt_createSolidHeapFromGame(0x20000,0x20);
    if(!s_menu.heap) return HOOK_CONTINUE;
    auto* old=mDoExt_setCurrentHeap(s_menu.heap);
    s_menu.screen=JKR_NEW J2DScreen();
    s_menu.frame=JKR_NEW J2DPicture(MULTI_CHAR('dl_clfr'),JGeometry::TBox2<f32>(0,0,48,48),frame,nullptr);
    s_menu.icon=JKR_NEW J2DPicture(MULTI_CHAR('dl_clic'),JGeometry::TBox2<f32>(0,0,44,44),icon,nullptr);
    if(s_menu.screen) {
        // A programmatic J2DScreen defaults to an opaque white background.
        // Clear only its backdrop; pane alpha must still reach the icon/frame.
        s_menu.screen->mColor=JUtility::TColor(0,0,0,0);
        if(s_menu.frame) s_menu.screen->appendChild(s_menu.frame);
        if(s_menu.icon) {
            s_menu.screen->appendChild(s_menu.icon);
            s_menu.iconPane=JKR_NEW CPaneMgr(s_menu.screen,MULTI_CHAR('dl_clic'),0,nullptr);
        }
    } else {
        JKR_DELETE(s_menu.frame);JKR_DELETE(s_menu.icon);
    }
    mDoExt_setCurrentHeap(old);
    if(!s_menu.screen || !s_menu.frame || !s_menu.icon || !s_menu.iconPane) {
        s_menu.release();
        svc_log->warn(mod_ctx,"Dual Wield collection: could not allocate menu resources");
        return HOOK_CONTINUE;
    }
    mDoExt_adjustSolidHeap(s_menu.heap);
    s_menu.owner=menu;
    return HOOK_CONTINUE;
}
// Find frames by their shared artwork and rendered position, so remapped or
// dynamically registered Essentials shields do not need hard-coded pane IDs.
void shield_frames(dMenu_Collect2D_c* menu,J2DPane* root,const ResTIMG* artwork,
                   std::vector<J2DPicture*>& frames) {
    if(!root) return;
    if(root->getTypeID()==18 && visible(root)) {
        auto* pic=static_cast<J2DPicture*>(root);
        if(texture(pic)==artwork) {
            const auto r=bounds(menu,root,0,0,false);
            for(const auto& cell:s_menu.cells) {
                if(cell.y==1 && std::fabs((r.left+r.width*.5f)-(cell.left+cell.width*.5f))<cell.width*.25f &&
                   std::fabs((r.top+r.height*.5f)-(cell.top+cell.height*.5f))<cell.height*.25f) {
                    frames.push_back(pic);break;
                }
            }
        }
    }
    for(auto* child=root->getFirstChildPane();child;child=child->getNextChildPane())
        shield_frames(menu,child,artwork,frames);
}
void move_equipped_flourishes(dMenu_Collect2D_c* menu,J2DPane* pane,const collection::Cell& frame) {
    if(!pane) return;
    // HD HUD's equipment flourishes are separate siblings of the frame. Move
    // that decoration with the equipped highlight, then restore after drawing.
    if((pane->mInfoTag>>16)==(MULTI_CHAR('hd_cef00')>>16) && visible(pane)) {
        const auto r=bounds(menu,pane,0,0,false);
        const float fx=frame.left+frame.width*.5f,fy=frame.top+frame.height*.5f;
        if(std::fabs(r.left+r.width*.5f-fx)<frame.width && std::fabs(r.top+r.height*.5f-fy)<frame.height && pane->getParentPane()) {
            Mtx parent;
            menu->mpLinkPm->getGlobalVtx(pane->getParentPane(),&parent,0,false,0);
            const float det=parent[0][0]*parent[1][1]-parent[0][1]*parent[1][0];
            if(std::fabs(det)>1e-6f) {
                const auto p=s_menu.placement;
                const float dx=p.left+p.size*.5f-fx,dy=p.top+p.size*.5f-fy;
                s_menu.decorations.push_back({pane,pane->getTranslateX(),pane->getTranslateY()});
                pane->translate(pane->getTranslateX()+(dx*parent[1][1]-dy*parent[0][1])/det,
                                pane->getTranslateY()+(dy*parent[0][0]-dx*parent[1][0])/det);
            }
        }
    }
    for(auto* child=pane->getFirstChildPane();child;child=child->getNextChildPane())
        move_equipped_flourishes(menu,child,frame);
}
void present_equipment(dMenu_Collect2D_c* menu) {
    if(!s_menu.visible || !s_menu.drawing) return;
    auto* reference=picture(menu->mpScreen,MULTI_CHAR('tate_g_1'));
    const auto* artwork=texture(reference);if(!artwork) return;
    std::vector<J2DPicture*> frames;
    shield_frames(menu,menu->mpScreen,artwork,frames);
    auto inactive=JUtility::TColor(107,107,107,255);
    auto equipped=JUtility::TColor(255,255,0,255);
    for(auto* frame:frames) {
        const auto color=frame->getWhite();
        if(color.r>200) equipped=color;else inactive=color;
    }
    s_menu.frame->setBlackWhite(reference->getBlack(),dual_wield_equipped()?equipped:inactive);
    if(!dual_wield_equipped()) return;
    for(auto* frame:frames) {
        if(frame->getWhite().r>200) move_equipped_flourishes(menu,menu->mpScreen,bounds(menu,frame,0,1,true));
        s_menu.frameTints.push_back({frame,frame->getWhite()});
        frame->setWhite(inactive);
    }
}
HookAction before_draw(ModContext*,void* args,void*,void*) {
    if(s_menu.owner==mods::arg<dMenu_Collect2D_c*>(args,0)) {
        s_menu.restore_tints();s_menu.drawing=true;
    }
    return HOOK_CONTINUE;
}
void after_layout(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    layout(menu);
    if(s_menu.owner==menu && s_menu.frameTints.empty()) present_equipment(menu);
}
void draw_icon() {
    if(!s_menu.visible || s_menu.drawn) return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf) return;graf->setup2D();
    s_menu.screen->draw(0,0,graf);s_menu.drawn=true;
}
void after_draw(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(s_menu.owner!=menu) return;
    draw_icon();s_menu.restore_tints();
}
HookAction before_cursor_screen_draw(ModContext*,void* args,void*,void*) {
    if(!s_menu.owner || !s_menu.drawing) return HOOK_CONTINUE;
    auto* cursor=s_menu.owner->mpDrawCursor;
    if(!cursor || mods::arg<J2DScreen*>(args,0)!=cursor->mpScreen) return HOOK_CONTINUE;
    // The cursor's draw() updates its shield-bound target again, and HUD mods
    // can realign it in their draw/update hooks. Apply our virtual cell at the
    // actual screen draw boundary, after all of that work (also when inlined).
    // Draw the icon first so the shared cursor remains above its frame.
    draw_icon();
    if(own_focus(s_menu.owner) && cursor->mpPaneMgr) {
        const auto p=s_menu.placement;
        // Keep the shared cursor's artwork, pulse and mod-provided geometry.
        cursor->mpPaneMgr->translate(p.left+p.size*.5f,p.top+p.size*.5f);
    }
    return HOOK_CONTINUE;
}
HookAction before_delete(ModContext*,void* args,void*,void*) {
    if(s_menu.owner==mods::arg<dMenu_Collect2D_c*>(args,0)) s_menu.release();
    return HOOK_CONTINUE;
}
HookAction before_wait(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);layout(menu);
    // Let page-owning mods consume shoulder-button input, including simultaneous A.
    if(mDoCPd_c::getTrigL(PAD_1) || mDoCPd_c::getTrigR(PAD_1)) {
        if(own_focus(menu)) restore_name(menu);
        return HOOK_CONTINUE;
    }
    if(own_focus(menu) && dMw_A_TRIGGER()) {activate(menu);return HOOK_SKIP_ORIGINAL;}
    // Native/foreign shield actions remain in their original hook chain.
    if(s_menu.owner==menu && !own_focus(menu) && menu->mCursorY==1 && dMw_A_TRIGGER() &&
       menu->field_0x22d[menu->mCursorX][1] && can_equip(menu) && dual_wield_equipped()) select_sword(false);
    return HOOK_CONTINUE;
}
void after_wait(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);layout(menu);show_name(menu);
}
HookAction before_navigate(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(s_menu.owner!=menu||!s_menu.visible||!menu->mpStick) return HOOK_CONTINUE;
    if(!own_focus(menu)) {
        int last=-1;
        for(std::size_t i=0;i<s_menu.cells.size();++i) {
            const auto& c=s_menu.cells[i];
            if(c.y==1 && c.x>=0 && c.available && (last<0 || c.left>s_menu.cells[last].left)) last=int(i);
        }
        if(last<0 || menu->mCursorY!=1 || menu->mCursorX!=s_menu.cells[last].x) return HOOK_CONTINUE;
        // STControl's trigger queries consume repeat state. Peek transactionally:
        // only retain that state if we handle the step into our virtual cell.
        // Otherwise the original/foreign navigator sees the untouched input.
        const STControl previous=*menu->mpStick;
        menu->mpStick->checkTrigger();
        if(menu->mpStick->checkRightTrigger()) {
            focus(menu);
            return HOOK_SKIP_ORIGINAL;
        }
        *menu->mpStick=previous;
        return HOOK_CONTINUE;
    }
    menu->mpStick->checkTrigger();
    int direction=menu->mpStick->checkLeftTrigger()?0:menu->mpStick->checkRightTrigger()?1:
        menu->mpStick->checkUpTrigger()?2:menu->mpStick->checkDownTrigger()?3:-1;
    if(direction>=0) {
        const int next=collection::neighbor(s_menu.cells,s_menu.placement,static_cast<collection::Direction>(direction));
        if(next>=0) {
            const auto& c=s_menu.cells[next];
            menu->field_0x259=menu->mCursorX;menu->field_0x25a=menu->mCursorY;
            menu->mCursorX=c.x;menu->mCursorY=c.y;
            restore_name(menu);menu->cursorPosSet();
            mDoAud_seStart(c.y==5?Z2SE_SY_CURSOR_OPTION:Z2SE_SY_CURSOR_ITEM,nullptr,0,0);
        }
    }
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_pointer(ModContext*,void* args,void* result,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(s_menu.owner!=menu||!s_menu.visible||!s_pointer.hit) return HOOK_CONTINUE;
    s_pointer.begin(4); // host Context::Collection; no private C++ ABI dependency
    if(s_pointer.hit(s_menu.iconPane,3)) {
        s_pointer.hover(0xda01);focus(menu);
        const bool click=s_pointer.click();if(click) activate(menu);
        *static_cast<bool*>(result)=click;return HOOK_SKIP_ORIGINAL;
    }
    for(const auto& c:s_menu.cells) if(c.x>=0 && s_pointer.hit(menu->mpSelPm[c.x][c.y],8)) {
        if(s_menu.focused) restore_name(menu);
        break;
    }
    return HOOK_CONTINUE;
}
HookAction before_click(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(own_focus(menu)) {activate(menu);return HOOK_SKIP_ORIGINAL;}
    if(menu->mCursorY==1&&menu->field_0x22d[menu->mCursorX][1]&&can_equip(menu)&&dual_wield_equipped()) select_sword(false);
    return HOOK_CONTINUE;
}
HookAction before_shield(ModContext*,void*,void*,void*) {
    if(dual_wield_equipped()) select_sword(false);
    return HOOK_CONTINUE;
}
void after_name(ModContext*,void* args,void*,void*) {show_name(mods::arg<dMenu_Collect2D_c*>(args,0));}
template<class T> bool resolve(const char* name,T& fn) {
    void* address=nullptr;
    if(!svc_hook->resolve || svc_hook->resolve(mod_ctx,name,&address,nullptr)!=MOD_OK || !address) return false;
    fn=reinterpret_cast<T>(address);return true;
}
}
bool dual_wield_equipped() {
    return s_selection.active(dual_wield_enabled(),save_state_boss_rush_active());
}
ModResult install_collection_dual_wield(ModError* error) {
    auto result=svc_save->observe_saves(mod_ctx,load_selection,load_selection,nullptr,nullptr,&s_saveObserver);
    if(result!=MOD_OK) return mods::set_error(error,result,"Dual Wield: save selection observer");
    load_selection(nullptr,0,nullptr);
    result=svc_config->subscribe(mod_ctx,dual_wield_config_var(),setting_changed,nullptr,&s_settingObserver);
    if(result!=MOD_OK) return mods::set_error(error,result,"Dual Wield: setting observer");
    const bool pointer=resolve("dusk::menu_pointer::begin_context",s_pointer.begin)&&
        (resolve("_ZN4dusk12menu_pointer8hit_paneEP8CPaneMgrf",s_pointer.hit) ||
         resolve("?hit_pane@menu_pointer@dusk@@YA_NPEAVCPaneMgr@@M@Z",s_pointer.hit))&&
        resolve("dusk::menu_pointer::set_hover_target",s_pointer.hover)&&
        resolve("dusk::menu_pointer::consume_click",s_pointer.click);
    if(!pointer) {s_pointer={};svc_log->warn(mod_ctx,"Dual Wield collection: pointer API unavailable; use controller navigation");}
    HookOptions early=HOOK_OPTIONS_INIT;early.priority=100;
    HookOptions late=HOOK_OPTIONS_INIT;late.priority=-100;
#define PRE(H,F) if((result=mods::hook::add_pre<H>(svc_hook,F,&early))!=MOD_OK) return mods::set_error(error,result,"Dual Wield collection: " #H)
#define POST(H,F) if((result=mods::hook::add_post<H>(svc_hook,F,&late))!=MOD_OK) return mods::set_error(error,result,"Dual Wield collection: " #H)
    PRE(CollectionSwordScreen,capture_icon);POST(CollectionSwordScreen,after_layout);
    PRE(CollectionSwordDelete,before_delete);POST(CollectionSwordLayout,after_layout);
    PRE(CollectionSwordDraw,before_draw);POST(CollectionSwordDraw,after_draw);
    // Run after ordinary screen-pre hooks as well as cursor draw/update hooks.
    if((result=mods::hook::add_pre<CollectionSwordCursorScreen>(svc_hook,before_cursor_screen_draw,&late))!=MOD_OK)
        return mods::set_error(error,result,"Dual Wield collection: CollectionSwordCursorScreen");
    PRE(CollectionSwordWait,before_wait);POST(CollectionSwordWait,after_wait);
    PRE(CollectionSwordNavigate,before_navigate);
    PRE(CollectionSwordPointer,before_pointer);PRE(CollectionSwordClick,before_click);
    PRE(CollectionSwordShield,before_shield);POST(CollectionSwordName,after_name);
#undef PRE
#undef POST
    return MOD_OK;
}
void shutdown_collection_dual_wield() {
    if(s_menu.owner && s_menu.focused) restore_name(s_menu.owner);
    s_menu.release();s_selection={};
    if(s_saveObserver) svc_save->unobserve_saves(mod_ctx,s_saveObserver);
    if(s_settingObserver) svc_config->unsubscribe(mod_ctx,s_settingObserver);
    s_saveObserver=0;s_settingObserver=0;
}
}
