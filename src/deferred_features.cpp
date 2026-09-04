// These ports are deliberately excluded until upstream exposes matching services.
#if 0

// Complete Wii U controller style:
// Needs input-layout and touch-layout services for labels, defaults, physical remapping,
// and the dedicated Midna D-Pad Down touch button.
void install_wii_u_controller_style();

// New Game+, Intro Skip, and Boss Rush:
// Need file-select/game-mode creation hooks plus dialog/warp/scene-flow services. The upstream
// stage actor service can help populate actors, but it does not replace these flow hooks yet.
void install_game_mode_creation();
void install_bossrush_flow();

// Visual HUD layout editor:
// Needs a public HUD layout or meter-pane service before hud_layout_settings.json can be applied
// without modifying host meter code.
void install_hud_editor_ui();

#endif
