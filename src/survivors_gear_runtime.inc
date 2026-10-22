// Persistent equipment is independent of run rewards and high-score resets.
constexpr const char* kGearBlob="survivors-gear";
GearInventory s_gear;
bool s_gearWritable=true;
unsigned s_gearNoticeTicks=0;
const char* s_gearNotice="";
GearItem s_gearNoticeItem{};
unsigned s_gearNoticeSlot=GearInventory::none;
struct GearNotice { GearItem item; const char* message; unsigned slot=GearInventory::none; };
std::vector<GearNotice> s_gearNoticeQueue;
void clear_gear_notices() {
    s_gearNoticeTicks=0;s_gearNotice="";s_gearNoticeItem={};s_gearNoticeSlot=GearInventory::none;s_gearNoticeQueue.clear();
}
void show_gear_notice(GearNotice notice) {
    if(s_gearNoticeTicks) {s_gearNoticeQueue.push_back(notice);return;}
    s_gearNoticeItem=notice.item;s_gearNoticeSlot=notice.slot;s_gearNotice=notice.message;s_gearNoticeTicks=8*ticks_per_second;
}
void dismiss_gear_notice() {
    s_gearNoticeTicks=0;s_gearNoticeItem={};s_gearNoticeSlot=GearInventory::none;s_gearNotice="";
    if(!s_gearNoticeQueue.empty()) {
        const auto next=s_gearNoticeQueue.front();s_gearNoticeQueue.erase(s_gearNoticeQueue.begin());
        show_gear_notice(next);
    }
}
void update_gear_notice() {
    if(s_gearNoticeTicks && !--s_gearNoticeTicks)dismiss_gear_notice();
}
bool gear_notice_equip_available() {
    return s_gearNoticeTicks && s_gearNoticeSlot<GearInventory::capacity &&
        s_gearNoticeItem.kind!=GearKind::Empty && s_gearNoticeItem.valid() &&
        s_gear.items[s_gearNoticeSlot]==s_gearNoticeItem;
}
UiWindowHandle s_gearWindow=0;
void close_gear_window();
void restore_gear_collection();
void open_gear_window();
bool hub_gear_interaction(daAlink_c*);
void load_gear() {
    s_gear={};s_gearWritable=true;
    GearInventory::Bytes bytes{};size_t size=bytes.size();
    const auto result=svc_save->get_blob(mod_ctx,kGearBlob,bytes.data(),&size);
    if(result==MOD_UNAVAILABLE)return; // Older slots have no equipment blob.
    if(result!=MOD_OK || !s_gear.decode(std::span(bytes.data(),std::min(size,bytes.size())))) {
        s_gearWritable=false;
        svc_log->warn(mod_ctx,"Survivors: invalid equipment inventory; preserving stored data");
    }
}
bool store_gear(const GearInventory& next) {
    if(!s_gearWritable)return false;
    const auto bytes=next.encode();
    if(svc_save->set_blob(mod_ctx,kGearBlob,bytes.data(),bytes.size())!=MOD_OK)return false;
    s_gear=next;s_progressSaved=false;return true;
}
bool reorder_gear(const GearInventory::SlotOrder& order) {
    auto next=s_gear;
    if(!next.reorder(order)||!store_gear(next))return false;
    const auto remap=[&](unsigned old){
        for(unsigned i=0;i<order.size();++i)if(order[i]==old)return i;
        return GearInventory::none;
    };
    s_gearNoticeSlot=remap(s_gearNoticeSlot);
    for(auto& notice:s_gearNoticeQueue)notice.slot=remap(notice.slot);
    return true;
}
void equip_gear_notice() {
    if(!gear_notice_equip_available())return;
    auto next=s_gear;
    if(!next.equip(s_gearNoticeSlot) || !store_gear(next)) {
        s_gearNotice="Equip failed - try again";return;
    }
    // Preserve earned Heart Containers and capped tunic bonuses across swaps.
    // A stronger tunic increases capacity without granting free healing.
    s_run.maximum_hearts=dComIfGs_getMaxLife()/5;
    s_run.equip_drop(s_gearNoticeItem);
    if(s_gearNoticeItem.kind==GearKind::Tunic) {
        dComIfGs_setMaxLife(s_run.maximum_hearts*5);
        dComIfGs_setLife(std::min<unsigned>(dComIfGs_getLife(),s_run.maximum_hearts*4));
    }
    dismiss_gear_notice();
}
void drop_boss_gear() {
    // Called only after the kill credit latch. Cleanup/victory-wave kills and
    // ordinary debug runs never create equipment. The explicit test override
    // permits saved equipment, but never restores scores/XP/map unlocks.
    const auto debug=s_run.debug_options();
    const bool maximum=s_run.debug_run && debug.max_equipment_drop_rate;
    const bool legendary=s_run.debug_run && debug.always_legendary_equipment;
    if((s_run.debug_run && !maximum && !legendary) || (!maximum && !s_run.roll_chance(s_run.equipment_drop_basis_points())))return;
    auto random=[](unsigned n){
        const uint32_t limit=UINT32_MAX-UINT32_MAX%n;
        uint32_t value;do{value=s_run.next_random()-1;}while(value>=limit);
        return value%n;
    };
    const auto item=GearInventory::roll(random,s_run.talents.rank(Talent::EquipmentRarity),legendary,s_run.options.equipment_progress());
    auto next=s_gear;
    const auto slot=next.add(item);
    if(slot==GearInventory::none)show_gear_notice({{},"Inventory Full!"});
    else if(!store_gear(next))show_gear_notice({{},"Equipment save failed!"});
    else show_gear_notice({item,item.is_weapon()?"Weapon acquired":"Tunic acquired",slot});
}
