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
DEFINE_HOOK(&Z2SeMgr::seStart, CollectionSwordEquipSound);

collection::Selection s_selection;
bool s_changingToSword=false;
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
    J2DPicture* flourishes[2]{};
    const ResTIMG* flourishTexture=nullptr;
    struct FrameTint {J2DPicture* pane;JUtility::TColor color;};
    std::vector<FrameTint> frameTints;
    struct DecorationVisibility {J2DPane* pane;bool shown;};
    std::vector<DecorationVisibility> decorations;
    bool drawing=false, drawn=false;
    std::vector<collection::Cell> cells;
    collection::Placement placement;
    bool focused=false, visible=false;
    u8 anchorX=0,anchorY=0;
    void restore_tints() {
        for(const auto& tint:frameTints) tint.pane->setWhite(tint.color);
        for(const auto& saved:decorations) {
            if(saved.shown) saved.pane->show();else saved.pane->hide();
        }
        for(auto* flourish:flourishes) if(flourish) flourish->hide();
        frameTints.clear();decorations.clear();drawing=drawn=false;
    }
    void release() {
        restore_tints();
        JKR_DELETE(iconPane);JKR_DELETE(screen);
        iconPane=nullptr;screen=nullptr;icon=frame=nullptr;
        for(auto*& flourish:flourishes) flourish=nullptr;
        flourishTexture=nullptr;
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
        text(menu->mpScreen,tag,second_sword_assets(second_sword()).name);
    const char* description=dual_wield_equipped()?"Dual Wield equipped. Select a shield to return to shield combat.":
        "Equip a second sword for Dual Wield. Requires an equipped shield.";
    for(u64 tag:{MULTI_CHAR('i_text0'),MULTI_CHAR('i_text1'),MULTI_CHAR('f_text0'),MULTI_CHAR('f_text1')})
        text(menu->mpScreen,tag,description);
    // The ordinary description renderer must not replace our text with the
    // anchor shield's message. The native grid itself remains untouched.
    menu->mItemNameString=0;
    menu->setAButtonString(menu->mIsWolf?0:0x436);
}
std::vector<collection::Cell> collect_cells(dMenu_Collect2D_c* menu) {
    std::vector<collection::Cell> cells;
    for(int y=0;y<6;++y) for(int x=0;x<7;++x) {
        // Collection-Lib replaces these managers with the actual custom-slot
        // panes. The host getItemTag table only knows vanilla cells, so relying
        // on its return value can omit an added shield (or find an old pane
        // when a native cell was replaced). Use the same pane as selection.
        auto* manager=menu->mpSelPm[x][y];
        auto* pane=manager?manager->getPanePtr():nullptr;
        if(!pane) {
            const auto tag=menu->getItemTag(x,y,true);
            if(tag) pane=menu->mpScreen->search(tag);
        }
        if(!visible(pane)) continue;
        cells.push_back(bounds(menu,pane,x,y,menu->field_0x22d[x][y]!=0 || y==5));
    }
    // Always include the Hylian Shield's reserved place, even before acquisition.
    auto* hylian=menu->mpScreen->search(MULTI_CHAR('tate_n1'));
    if(hylian && visible(hylian->getParentPane())) {
        auto c=bounds(menu,hylian,-1,1,false);
        bool found=false;for(const auto& old:cells) if(old.y==1&&std::fabs(old.left-c.left)<1) found=true;
        if(!found) cells.push_back(c);
    }
    return cells;
}
const ResTIMG* second_sword_icon() {
    auto* archive=dComIfGp_getCollectResArchive();
    return archive ? static_cast<const ResTIMG*>(
        archive->getResource('TIMG',second_sword_assets(second_sword()).icon)) : nullptr;
}
void update_sword_icon() {
    if(!s_menu.icon) return;
    const auto* icon=second_sword_icon();
    // Never leave a stale icon claiming a different sword after a live switch.
    if(!icon) {s_menu.icon->hide();return;}
    if(texture(s_menu.icon)!=icon) s_menu.icon->changeTexture(icon,0);
    s_menu.icon->show();
}
void layout(dMenu_Collect2D_c* menu) {
    if(s_menu.owner!=menu || !s_menu.screen || !menu->mpScreen || !menu->mpLinkPm) return;
    update_sword_icon();
    s_menu.cells=collect_cells(menu);
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
struct EquipmentVisuals {
    struct Pane {J2DPane* pane;bool shown;JUtility::TColor black,white;};
    std::vector<Pane> panes;
    u8 equippedShield=0xff;
};
void save_pane_visuals(J2DPane* pane,EquipmentVisuals& saved) {
    if(!pane) return;
    auto* pic=pane->getTypeID()==18?static_cast<J2DPicture*>(pane):nullptr;
    saved.panes.push_back({pane,pane->isVisible(),pic?pic->getBlack():JUtility::TColor(0),
                          pic?pic->getWhite():JUtility::TColor(0)});
    for(auto* child=pane->getFirstChildPane();child;child=child->getNextChildPane()) save_pane_visuals(child,saved);
}
EquipmentVisuals capture_visuals(dMenu_Collect2D_c* menu) {
    EquipmentVisuals saved;saved.equippedShield=menu->mEquippedShield;
    save_pane_visuals(menu->mpScreen,saved);return saved;
}
void restore_visuals(dMenu_Collect2D_c* menu,const EquipmentVisuals& saved) {
    menu->mEquippedShield=saved.equippedShield;
    for(const auto& old:saved.panes) {
        if(old.shown) old.pane->show();else old.pane->hide();
        if(old.pane->getTypeID()==18) static_cast<J2DPicture*>(old.pane)->setBlackWhite(old.black,old.white);
    }
}
// Only for temporary backing-item changes: the regular equip setters also
// grant collection ownership, even when dMeter2Info_setShield gets false.
void set_backing_shield(u8 shield) {
    g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().setSelectEquip(COLLECT_SHIELD,shield);
    g_dComIfG_gameInfo.play.setSelectEquip(COLLECT_SHIELD,shield);
}
struct ShieldOwnershipGuard {
    struct State {u8 collection,item;bool collected=false,first=false;};
    State shields[3]={
        {COLLECT_WOODEN_SHIELD,dItemNo_WOOD_SHIELD_e},
        {COLLECT_ORDON_SHIELD,dItemNo_SHIELD_e},
        {COLLECT_HYLIAN_SHIELD,dItemNo_HYLIA_SHIELD_e},
    };
    ShieldOwnershipGuard() {
        for(auto& shield:shields) {
            shield.collected=dComIfGs_isCollectShield(shield.collection)!=0;
            shield.first=dComIfGs_isItemFirstBit(shield.item)!=0;
        }
    }
    ShieldOwnershipGuard(const ShieldOwnershipGuard&)=delete;
    ShieldOwnershipGuard& operator=(const ShieldOwnershipGuard&)=delete;
    ~ShieldOwnershipGuard() {
        for(const auto& shield:shields) {
            if((dComIfGs_isCollectShield(shield.collection)!=0)!=shield.collected) {
                if(shield.collected) dComIfGs_setCollectShield(shield.collection);
                else dComIfGs_offCollectShield(shield.collection);
            }
            if((dComIfGs_isItemFirstBit(shield.item)!=0)!=shield.first) {
                if(shield.first) dComIfGs_onItemFirstBit(shield.item);
                else dComIfGs_offItemFirstBit(shield.item);
            }
        }
    }
};
struct ShieldChoice {
    dMenu_Collect2D_c* menu=nullptr;
    u8 backing=0xff;
    EquipmentVisuals visuals;
} s_shieldChoice;
void begin_shield_choice(dMenu_Collect2D_c* menu) {
    if(!menu || s_changingToSword || s_shieldChoice.menu || s_menu.owner!=menu || own_focus(menu) ||
       menu->mCursorY!=1 || !menu->field_0x22d[menu->mCursorX][1] || !can_equip(menu) || !dual_wield_equipped()) return;
    s_shieldChoice={menu,dComIfGs_getSelectEquipShield(),capture_visuals(menu)};
    // Dual Wield is the equipped choice. Its backing shield must not make a
    // real shield click look like a no-op (vanilla) or an unequip (Essentials).
    // Keep our saved selection until the downstream action actually succeeds.
    set_backing_shield(dItemNo_NONE_e);
}
void finish_shield_choice(dMenu_Collect2D_c* menu) {
    if(!menu || s_shieldChoice.menu!=menu) return;
    if(dComIfGs_getSelectEquipShield()==dItemNo_NONE_e) {
        // A debounce/unlock check rejected the action. Restore the complete
        // previous state instead of silently losing Dual Wield or its backing.
        set_backing_shield(s_shieldChoice.backing);
        restore_visuals(menu,s_shieldChoice.visuals);
    } else {
        select_sword(false);
        if(auto* link=daAlink_getAlinkActorClass()) link->setShieldChange();
    }
    s_shieldChoice={};
}
void clear_previous_shield(dMenu_Collect2D_c* menu) {
    // This synchronous handoff may run foreign hooks, but must never acquire
    // or revoke a shield. Restore only shield ownership, after Restore below;
    // foreign custom-equipment selection changes must remain in effect.
    ShieldOwnershipGuard ownership;
    // Custom-equipment mods keep their own selected shield in addition to the
    // game's backing item. Go through their changeShield hooks to clear it.
    // Column 3 is the native wooden-shield action, not the virtual cell's
    // (possibly custom) visual neighbor. Never replay that neighbor's action.
    struct Restore {
        dMenu_Collect2D_c* menu;
        u8 x,y,shield;
        EquipmentVisuals visuals;
        ~Restore() {
            menu->mCursorX=x;menu->mCursorY=y;
            // Some mods toggle the backing shield off when selecting it again.
            // Dual Wield still needs that backing item for shield combat logic.
            set_backing_shield(shield);
            // changeShield also touches cached equipment, frame tints and
            // foreign ornaments. Restoring only the item ID leaves a ghost
            // Ordon-shield highlight that HD HUD later treats as equipped.
            restore_visuals(menu,visuals);
            s_changingToSword=false;
        }
    } restore{menu,menu->mCursorX,menu->mCursorY,dComIfGs_getSelectEquipShield(),capture_visuals(menu)};
    s_changingToSword=true;
    menu->mCursorX=3;menu->mCursorY=1;
    menu->changeShield();
}
void activate(dMenu_Collect2D_c* menu) {
    auto* link=daAlink_getAlinkActorClass();
    if(!can_equip(menu) || dComIfGs_getSelectEquipShield()==dItemNo_NONE_e) return;
    if(dual_wield_equipped()) return;
    clear_previous_shield(menu);
    select_sword(true);link->setShieldChange();dMeter2Info_set2DVibration();
    mDoAud_seStart(Z2SE_SY_ITEM_SET_X,nullptr,0,0);show_name(menu);
}
HookAction before_equip_sound(ModContext*,void* args,void* result,void*) {
    if(!s_changingToSword) return HOOK_CONTINUE;
    const auto id=mods::arg<JAISoundID>(args,1);
    if(id!=Z2SE_SY_ITEM_SET_X && id!=Z2SE_SY_ITEM_COMBINE_OFF) return HOOK_CONTINUE;
    // The internal shield handoff is silent; activate() plays one equip sound.
    if(result) *static_cast<bool*>(result)=false;
    return HOOK_SKIP_ORIGINAL;
}
HookAction capture_icon(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(s_menu.owner==menu && s_menu.screen) return HOOK_CONTINUE;
    s_menu.release();
    // Read native artwork by resource name, independently of remapped/custom
    // slots. The texture replacement service still sees the original resource.
    const auto* icon=second_sword_icon();
    const auto* frame=texture(picture(menu->mpScreen,MULTI_CHAR('tate_g_1')));
    if(!icon||!frame) return HOOK_CONTINUE;
    // The same game resource used by HD HUD. Do not depend on another mod's
    // generated pane tags, frame artwork, visibility or slot coordinates.
    auto* archive=dComIfGp_getCollectResArchive();
    s_menu.flourishTexture=archive?static_cast<const ResTIMG*>(archive->getResource('TIMG',"tt_kazari_2nd_okan_64.bti")):nullptr;
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
        // Allocate the archive-backed ornaments on our menu heap.
        // These follow the icon/frame in draw order and share their lifetime.
        for(int corner=0;corner<2;++corner) {
            auto* flourish=JKR_NEW J2DPicture(MULTI_CHAR('dl_clf0')+corner,
                JGeometry::TBox2<f32>(0,0,24,24),s_menu.flourishTexture?s_menu.flourishTexture:frame,nullptr);
            s_menu.flourishes[corner]=flourish;
            if(flourish) {s_menu.screen->appendChild(flourish);flourish->hide();}
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
void hide_shield_flourishes(dMenu_Collect2D_c* menu,J2DPane* pane,const collection::Cell& frame) {
    if(!pane) return;
    // Hide the old shield's ornaments only for this draw. This is independent
    // of drawing our own pair: custom slots need not expose matching panes.
    // HD HUD owns the native pairs; Collection-Lib tags added pairs with
    // clfl{row}{column*2+corner}, with each numeric part encoded as '0'+value.
    // This includes the separate Ordon shield. Row 1 is
    // shields. Keep the frame-corner check for both families so remapped slots
    // do not hide other equipment's decorations.
    const bool hdFlourish=(pane->mInfoTag>>16)==(MULTI_CHAR('hd_cef00')>>16);
    const bool libShieldFlourish=(pane->mInfoTag>>8)==(MULTI_CHAR('clfl10')>>8);
    if((hdFlourish || libShieldFlourish) && pane->getTypeID()==18 && visible(pane->getParentPane())) {
        const auto r=bounds(menu,pane,0,0,false);
        const int corner=collection::flourish_corner(frame,r);
        if(corner>=0) {
            bool saved=false;for(const auto& old:s_menu.decorations) if(old.pane==pane) saved=true;
            if(!saved) s_menu.decorations.push_back({pane,pane->isVisible()});
            pane->hide();
        }
    }
    for(auto* child=pane->getFirstChildPane();child;child=child->getNextChildPane())
        hide_shield_flourishes(menu,child,frame);
}
void present_sword_flourishes() {
    for(auto* flourish:s_menu.flourishes) if(flourish) flourish->hide();
    if(!s_menu.owner || !s_menu.visible || !dual_wield_equipped() || !s_menu.flourishTexture) return;
    // Only the HD collection layout uses these equipment ornaments. Its root
    // remains identifiable even when Essentials replaces every shield frame.
    if(!visible(s_menu.owner->mpScreen->search(MULTI_CHAR('hd_colly')))) return;
    const auto p=s_menu.placement;
    const float scale=(p.size+6)/52.f; // HD: 46-unit icon, 52-unit frame
    for(int corner=0;corner<2;++corner) {
        auto* target=s_menu.flourishes[corner];if(!target) continue;
        target->setTexCoord(target->getTexture(0),BIND15,static_cast<J2DMirror>(corner==0?0:3),false);
        target->setBlackWhite(JUtility::TColor(0,0,0,0),JUtility::TColor(246,244,198,255));
        target->setCornerColor(JUtility::TColor(255,255,255,255));target->setAlpha(255);
        // HD HUD places the 24-unit corners 6 units outside the frame at TL,
        // and 18 units before its bottom-right edge (mirrored on both axes).
        const float offset=corner==0?-6.f*scale:p.size+6-18.f*scale;
        target->move(p.left-3+offset,p.top-3+offset);
        target->resize(24*scale,24*scale);target->show();
    }
}
void present_equipment(dMenu_Collect2D_c* menu) {
    if(!s_menu.visible || !s_menu.drawing) return;
    auto* reference=picture(menu->mpScreen,MULTI_CHAR('tate_g_1'));
    const auto* artwork=texture(reference);if(!artwork) return;
    std::vector<J2DPicture*> frames;
    shield_frames(menu,menu->mpScreen,artwork,frames);
    // Native shields remain shield slots even if another mod assigns distinct
    // frame textures or remaps their coordinates. Never leave their highlight
    // active just because the shared-artwork scan did not recognize them.
    for(u64 tag:{MULTI_CHAR('tate_g_0'),MULTI_CHAR('tate_g_1')}) {
        auto* frame=picture(menu->mpScreen,tag);
        if(frame && visible(frame) && std::find(frames.begin(),frames.end(),frame)==frames.end()) frames.push_back(frame);
    }
    auto inactive=JUtility::TColor(107,107,107,255);
    auto equipped=JUtility::TColor(255,255,0,255);
    for(auto* frame:frames) {
        const auto color=frame->getWhite();
        if(color.r>200) equipped=color;else inactive=color;
    }
    // Clearing a custom shield may leave every shield frame inactive. The HD
    // palette is fixed and must not fall back to vanilla yellow in that gap.
    if(visible(menu->mpScreen->search(MULTI_CHAR('hd_colly')))) {
        equipped=JUtility::TColor(246,244,198,255);
        inactive=JUtility::TColor(132,134,104,255);
    }
    s_menu.frame->setBlackWhite(reference->getBlack(),dual_wield_equipped()?equipped:inactive);
    if(!dual_wield_equipped()) return;
    for(auto* frame:frames) {
        hide_shield_flourishes(menu,menu->mpScreen,bounds(menu,frame,0,1,true));
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
}
void draw_icon() {
    if(!s_menu.visible || s_menu.drawn) return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf) return;graf->setup2D();
    present_sword_flourishes();
    s_menu.screen->draw(0,0,graf);s_menu.drawn=true;
}
void after_draw(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(s_menu.owner!=menu) return;
    draw_icon();s_menu.restore_tints();
}
HookAction before_cursor_screen_draw(ModContext*,void* args,void*,void*) {
    if(!s_menu.owner || !s_menu.drawing) return HOOK_CONTINUE;
    if(mods::arg<J2DScreen*>(args,0)==s_menu.owner->mpScreen) {
        // Final boundary: HUD layout/visibility updates have finished. Transfer
        // the equipped decoration before the native menu is drawn, then render
        // our own pair above our icon later with the independent sword screen.
        s_menu.restore_tints();s_menu.drawing=true;
        layout(s_menu.owner);present_equipment(s_menu.owner);
        return HOOK_CONTINUE;
    }
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
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    finish_shield_choice(menu);
    if(s_menu.owner==menu) s_menu.release();
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
    if(dMw_A_TRIGGER()) begin_shield_choice(menu);
    return HOOK_CONTINUE;
}
void after_wait(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    finish_shield_choice(menu);layout(menu);show_name(menu);
}
HookAction before_navigate(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(s_menu.owner!=menu||!s_menu.visible||!menu->mpStick) return HOOK_CONTINUE;
    const STControl previous=*menu->mpStick;
    menu->mpStick->checkTrigger();
    const int direction=menu->mpStick->checkLeftTrigger()?0:menu->mpStick->checkRightTrigger()?1:
        menu->mpStick->checkUpTrigger()?2:menu->mpStick->checkDownTrigger()?3:-1;
    if(!own_focus(menu)) {
        // STControl's trigger queries consume repeat state. Peek transactionally:
        // only retain that state if we handle the step into our virtual cell.
        // Otherwise the original/foreign navigator sees the untouched input.
        if(direction>=0 && collection::enters_slot(s_menu.cells,s_menu.placement,menu->mCursorX,menu->mCursorY,
                                                  static_cast<collection::Direction>(direction))) {
            focus(menu);
            return HOOK_SKIP_ORIGINAL;
        }
        *menu->mpStick=previous;
        return HOOK_CONTINUE;
    }
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
    begin_shield_choice(menu);
    return HOOK_CONTINUE;
}
HookAction before_shield(ModContext*,void* args,void*,void*) {
    begin_shield_choice(mods::arg<dMenu_Collect2D_c*>(args,0));
    return HOOK_CONTINUE;
}
void after_shield_choice(ModContext*,void* args,void*,void*) {
    finish_shield_choice(mods::arg<dMenu_Collect2D_c*>(args,0));
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
    // Collection-Lib also uses -100 for its final layout/frame refresh. A tie
    // follows registration order and can re-show shield ornaments after us.
    HookOptions finalScreen=late;finalScreen.priority=-200;
    // Its +100 navigator consumes STControl and skips the remaining pre-hooks.
    // Handle entry/exit of our virtual slot first; all other moves pass through.
    HookOptions virtualNavigation=early;virtualNavigation.priority=200;
#define PRE(H,F) if((result=mods::hook::add_pre<H>(svc_hook,F,&early))!=MOD_OK) return mods::set_error(error,result,"Dual Wield collection: " #H)
#define POST(H,F) if((result=mods::hook::add_post<H>(svc_hook,F,&late))!=MOD_OK) return mods::set_error(error,result,"Dual Wield collection: " #H)
    PRE(CollectionSwordScreen,capture_icon);POST(CollectionSwordScreen,after_layout);
    PRE(CollectionSwordDelete,before_delete);POST(CollectionSwordLayout,after_layout);
    PRE(CollectionSwordDraw,before_draw);POST(CollectionSwordDraw,after_draw);
    // Run after ordinary screen-pre hooks as well as cursor draw/update hooks.
    if((result=mods::hook::add_pre<CollectionSwordCursorScreen>(svc_hook,before_cursor_screen_draw,&finalScreen))!=MOD_OK)
        return mods::set_error(error,result,"Dual Wield collection: CollectionSwordCursorScreen");
    PRE(CollectionSwordWait,before_wait);POST(CollectionSwordWait,after_wait);
    if((result=mods::hook::add_pre<CollectionSwordNavigate>(svc_hook,before_navigate,&virtualNavigation))!=MOD_OK)
        return mods::set_error(error,result,"Dual Wield collection: CollectionSwordNavigate");
    PRE(CollectionSwordPointer,before_pointer);PRE(CollectionSwordClick,before_click);
    POST(CollectionSwordClick,after_shield_choice);POST(CollectionSwordShield,after_shield_choice);
    PRE(CollectionSwordShield,before_shield);POST(CollectionSwordName,after_name);
    PRE(CollectionSwordEquipSound,before_equip_sound);
#undef PRE
#undef POST
    return MOD_OK;
}
void shutdown_collection_dual_wield() {
    if(s_shieldChoice.menu) finish_shield_choice(s_shieldChoice.menu);
    if(s_menu.owner && s_menu.focused) restore_name(s_menu.owner);
    s_menu.release();s_selection={};
    if(s_saveObserver) svc_save->unobserve_saves(mod_ctx,s_saveObserver);
    if(s_settingObserver) svc_config->unsubscribe(mod_ctx,s_settingObserver);
    s_saveObserver=0;s_settingObserver=0;
}
}
