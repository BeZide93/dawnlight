#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"

// Game includes
#include "d/d_item_data.h"
#include "f_op/f_op_actor_mng.h"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);

// Example game hook: turn heart drops into green rupees.
DEFINE_HOOK(fopAcM_createItem, CreateItem);

static HookAction on_create_item_pre(ModContext*, void* args, void*, void*) {
    int& itemNo = mods::arg_ref<int>(args, 1);
    if (itemNo == dItemNo_HEART_e) {
        itemNo = dItemNo_GREEN_RUPEE_e;
    }
    return HOOK_CONTINUE;
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    // Installs a pre hook on fopAcM_createItem.
    ModResult result = mods::hook_add_pre<CreateItem>(svc_hook, on_create_item_pre);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install on_create_item_pre");
        return result;
    }

    svc_log->info(mod_ctx, "my_mod initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
