#pragma once

#include "mods/svc/actor.h"
#include "mods/svc/config.h"
#include "mods/svc/flow.h"
#include "mods/svc/game.h"
#include "mods/svc/game_mode.h"
#include "mods/svc/hook.h"
#include "mods/svc/host.h"
#include "mods/svc/log.h"
#include "mods/svc/message.h"
#include "mods/svc/overlay.h"
#include "mods/svc/save.h"
#include "mods/svc/texture.h"
#include "mods/svc/ui.h"

extern const ActorService* svc_actor;
extern const ConfigService* svc_config;
extern const FlowService* svc_flow;
extern const GameService* svc_game;
extern const GameModeService* svc_game_mode;
extern const HookService* svc_hook;
extern const HostService* svc_host;
extern const LogService* svc_log;
extern const MessageService* svc_message;
extern const OverlayService* svc_overlay;
extern const SaveService* svc_save;
extern const TextureService* svc_texture;
extern const UiService* svc_ui;
