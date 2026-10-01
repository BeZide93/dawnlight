# Replacing the Fierce Deity / Dark Link bar

Dawnlight exports `dev.bezide.dawnlight.fierce_deity_hud`, version **1.0**.
A separate `.dusk` HUD mod can replace only this bar, without changing Stamina,
lantern/oxygen graphics, gameplay, charge gain or transformation behavior.

## Integration

1. Copy `include/dawnlight/fierce_deity_hud.h` into your project; use the matching
   Dusklight SDK. No Dawnlight implementation sources or textures are needed.
2. Declare `IMPORT_OPTIONAL_SERVICE(DawnlightFierceDeityHudService,
   svc_dawnlight_fierce_hud)` beside your existing `DEFINE_MOD()`.
3. During `mod_initialize`, check the service pointer and call
   `register_renderer(mod_ctx, draw_callback, user_data)`.
4. Draw synchronously in that callback. The frame contains percentage (0–100),
   transformation state, full-bar bounds, scale and final opacity. Return `true`
   when your renderer handles the draw; `false` keeps Dawnlight's original bar.
5. Call `unregister_renderer(mod_ctx)` when disabling your replacement setting
   and in `mod_shutdown`. Register again when enabling your setting.

The optional import lets the mod load without Dawnlight or with an older version
that has no service. It also orders initialization after Dawnlight. Use a required
`IMPORT_SERVICE` if your mod cannot function without it. Do not add a second
`DEFINE_MOD()` when integrating this into an existing mod.

For polling, initialize `DawnlightFierceDeityHudState` with
`DAWNLIGHT_FIERCE_DEITY_HUD_STATE_INIT` and call `get_state(mod_ctx, &state)`.
`enabled` is the feature setting; `visible` is the player/menu visibility gate;
`active` means transformed, not merely that the bar is charging. Visibility does
not include opacity: the draw callback's `alpha` additionally includes native
HUD visibility/ancestor alpha, idle fading and fade-when-empty. No current player
means zero charge and no active/visible state. Polling does not advance gameplay.

## Drawing and lifecycle rules

- All service calls and callbacks are on the game thread. Draw in the callback,
  not later using cached frame pointers. No callback occurs while logically hidden.
- Coordinates are the current **J2D HUD coordinates**, not physical screen pixels.
  Bounds already include the HUD layout transform: do not multiply them by scale
  again. Scale is supplied for sizing custom decorative details.
- `graf_context` is a borrowed `J2DGrafContext*` for game-SDK renderers. Apply
  `alpha` once. Dawnlight suspends its own automatic J2D fades during your callback.
  Restore modified graphics/context state; do not recursively draw the native HUD
  or let exceptions escape the callback.
- Only one mod owns the renderer. A second receives `MOD_CONFLICT`; it cannot
  unregister the first. Re-registering from the same owner updates the callback.
- Missing/unregistered renderers and callbacks returning `false` use Dawnlight's
  renderer. Unloading/failure of the owner automatically clears its callback via
  the host lifecycle notification. No stale mod code is called afterward.

For J2D rendering, build your own mod with `add_mod(... FEATURES game ...)`.
