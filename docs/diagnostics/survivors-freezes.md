# Survivors freeze diagnostic build

Optional Performance Logs are available in Survivors Settings. They investigate
reported multi-second stalls following the correction of Link's Gerudo Desert
immunity. Build #1123 (`1ec429f`) introduced contact lifetime protection; the
later `3cfd61c` correction preserves native incoming hits without removing the
outgoing stale-contact guard. This build does not revert either change, change
hit acceptance, add invulnerability, modify archive retention, or enable Debug.

## Getting a diagnostic build

Release and local builds include the **Performance Logs** toggle, default **Off**.
Set it in Dawnlight's Survivors Settings or the Survivors hub Settings tab, then
**fully restart Dusklight**. Both On and Off apply at the next startup; the text
under the toggle shows the state of the current session. No Debug tab is required.
The setting persists through ConfigService, including settings export/import. It
is independent of run difficulty, score eligibility and saved run options.

When started with Off, the reporter installs no diagnostic callbacks, reads no
clocks, collects no counters/context and writes no diagnostic startup/shutdown or
periodic records. Existing gameplay/archive hooks are unchanged. Inline boolean
guards and the compiled code/static storage remain; this is not literally the
same binary as removing diagnostics. A build with
`-DDAWNLIGHT_FREEZE_DIAGNOSTICS=OFF` removes instrumentation and the toggle entirely.

Changes require restart because HookService 1.1 cannot unregister one callback:
uninstalling the resource target also removes Dawnlight's archive-retention
callbacks. Replacing a live execute detour inside its own call is unsafe as well.
The toggle therefore only saves the requested value; it never hot-uninstalls hooks.

1. Turn Performance Logs On, then fully restart Dusklight with the intended mods already installed. Keep the
   same gameplay/settings/texture pack initially so the reported case is preserved.
2. Confirm the log contains `[SurvivorsDiag] enabled schema=1`, with `version`,
   `commit` and `build`. CI records its checked-out revision (a PR merge revision
   on pull-request runs), and its Actions run number. Local builds say `local`.
3. Play Gerudo Desert until the pause recurs. Note whether it happened during
   combat, a new enemy/boss spawn, a menu, or a stage change; resume after the pause.
4. Exit normally and provide the full log. No extra tool, screenshot or overlay
   is required. The first banner after a mod reload starts a fresh time origin.

## Reading `[SurvivorsDiag]` records

The clock is monotonic wall time, in microseconds internally. `t_ms` is elapsed
since this module initialized, not game time or a calendar timestamp. Each line
summarizes a window of **completed** calls, rather than attributing everything to
one particular frame. A stall crossing a window boundary is charged on completion.

- `gap_max_ms`, `gaps`, `slow_gaps`: intervals between Survivors host-update
  pulses. Gaps >=100 ms are flagged, including alt-tab, suspended execution,
  menus, pacing or work outside the measured native function. They are not
  automatically CPU stalls. The reporter excludes its own preceding log-write
  duration from the next gap.
- `execute_*`: optional paired hooks around native `fapGm_Execute`; elapsed
  wall time includes the native game update and callbacks inside that boundary.
  This is not GPU timing or the complete presentation/vsync pipeline.
- `resource_*`: optional paired hooks around `dRes_info_c::setRes()` processing,
  including other callbacks within the measured interval. `resource_peak_archive`
  identifies the archive of the longest completed outer processing call. Pending
  calls are included; this does not measure background file I/O separately.
- `contacts_*`: lifetime validation of Link's and Survivors' attack colliders.
  `contact_dispatch_*` measures bookkeeping after a Link-owned attack passes the
  observer's ownership filter. `contact_delete_*` measures cleanup after a native
  actor deletion passes the method filter. `update_*` and `player_*` measure
  Survivors maintenance and its playable player-update callback respectively.
- Each metric has `calls`, `total_ms`, `max_ms`. Timings are **inclusive and can
  overlap** (e.g. resource processing within execute); do not sum them as separate
  exclusive costs. Same-metric nested intervals are counted only at the outer level.
- `cache_held`: retained archive slots. `acquire`, `full`, `release` count
  acquisitions, full-capacity misses and cleared held slots in the window.
  `duplicate_ready` counts repeated ready callbacks for an already-held archive;
  it is **not** the number of successful native cache lookups.
- `rejected_in` / `rejected_out`: existing stale-reference validation resets.
  These counters only observe the existing decisions. They do not reject more hits.
- `map` uses 0 Darknut Arena, 1 Hyrule Field, 2 Gerudo Desert, 3 Palace of Twilight,
  4 Throne Room. `phase` is the existing Survivors Phase enum; `tick` is run time.
  `hub`, `arena`, `transition`, `paused`, `event`, `menu` provide the most recent
  host-update snapshot for the window; they may have changed during that window.

Slow windows are reported at most once per second when a gap or execute call is
>=100 ms, or another measured section is >=50 ms. Otherwise a heartbeat appears
about every 30 seconds while Survivors is active, including its hub/menus. Mode
exit and shutdown flush once even if the interval is shorter. No messages are
written from individual resource/contact hooks. Records use bounded buffers and
16-entry hook stacks; `stack_overflow` / `unmatched_post` disclose lost matching
coverage rather than overwriting an outer measurement. Mode changes invalidate
in-flight samples and reset the gap baseline.

Startup reports `execute_hook` and `resource_hook` separately. If either paired
hook is unavailable on a host, it remains inert and emits one warning. Gameplay
initialization continues, local scopes and gap detection still work, and no shared
hook target is uninstalled. Successful installation does not alone prove coverage;
check heartbeat call counts after playing.

A permanent hang cannot flush its unfinished call. These logs diagnose pauses
that return; a never-returning hang needs a native thread capture. Timers/hook
callbacks add overhead, and the logger's write can affect a surrounding execute
sample, so compare this diagnostic build with the matching ordinary build before
claiming a performance regression or improvement. Switch Performance Logs Off
and restart when finished collecting logs.

## Validation

The deterministic production-reporter fixture covers optional hook half-failures,
long calls, live archive names, counters, timestamp/build identity, nested calls,
foreign post callbacks, bounded stack overflow, rate limiting, normal heartbeats,
logging overhead in update gaps, mode changes during scopes and shutdown. It also
checks runtime Off for zero clock reads, hook installs and log writes, and
compiles the disabled module without engine dependencies. A separate production
settings fixture verifies default Off, persistence, the restart boundary and UI. Contact lifetime tests
(including ASan/UBSan) and archive retention tests run with instrumentation both
compiled out and enabled behind a reporter fixture. Native platform CI builds
and iOS preparation checks validate the actual SDK hook metadata.


## Buffer-crash diagnostics

Performance Logs also enables `[SurvivorsBuffer]` records, **independently of
Performance Mode and Performance Monitor**. The restart requirement remains.
Sampling alone does not enable effect limits, change difficulty or alter combat.
With Performance Logs off, scene/effect inventory and these records are inactive;
Performance Mode can still use its existing buffer-pressure feedback.

- All four arenas report used bytes, actual capacity and high-water marks:
  `vertex`, `index`, `uniform`, `storage`. Peaks reset with the run/measurement
  epoch (including a Performance Mode switch).
- `buffer_threshold` records crossings of 80%, 90% and 95%. Each arena rearms
  below 65%. `buffer_growth` also flags an increase of at least 15 percentage
  points and 256 KiB between requests no more than 250 ms apart. Its bitmask is
  vertex=1, index=2, uniform=4, storage=8. Threshold/growth/sample-loss reports
  share a one-second rate limit; pending crossings are retained. The highest
  utilization snapshot during that pending window supplies the detailed context;
  crossed thresholds can summarize multiple observations in that window.
- Normal heartbeats are every 30 seconds. Scene changes have a separate
  one-second limit. A newly identified item event may emit immediately using a
  separate one-second allowance, so the initial scene record does not suppress
  the Ordon Sword event. These `context_only=1` records deliberately contain no
  buffer measurement. They are issued before that update's later render work.
- `map`, `phase`, `tick`, hub/transition/pause/event/menu flags, enemy/projectile
  counts and `item_event` identify the captured context. Item event 1 is the hub
  Ordon Sword presentation, 2 a Survivors treasure presentation, 0 neither of
  those identified events. `reward` is a `ChestReward` enum, not a native item ID.
- Each named visual has `active`, `full`, `reduced` and `emitters` counters. They
  cover burns, waves, lightning, explosions, boomerangs, fire-arrow trails,
  Fire Edge, Jump Strike and chains. Emitter counts are live owned emitters, not
  particles. Full means a non-reduced group with a living emitter; native chain
  models are counted separately. Active groups can have no visual yet (e.g. a
  delayed bolt or unavailable particle resource). Fire Edge/Jump counters report
  living decorations, not suppressed attempts. `reserved_emitters` is the
  Performance Mode reservation budget; it is normally zero with Mode off.
- Once per second, the existing read-only mod inventory is scanned for an active
  package whose ID or display name contains “deferred fog”, ignoring separators
  and case. `fog_inventory` is -1 unavailable, 0 no matching active package,
  1 one or more matches. ID/version of the first match and match count are logged.
  This is package recognition, **not proof that fog rendering is enabled**:
  `fog_render_setting=unknown` is intentional. Renamed/unrecognized packages can
  be missed; inactive packages are excluded. Inventory polling can lag a change.

### Frame attribution and persistence limits

AuroraStats does not expose the source frame ID. `stats_frame=unknown` therefore
remains explicit. `request` identifies Dawnlight's AFTER_HUD sampling request;
`sample_sequence` identifies a worker copy, **not a game or GPU frame number**.
`context_request` associates the copied game-thread scene snapshot with that
request. The statistics may describe earlier completed recording work, so the
scene is correlation evidence rather than exact attribution of those bytes.

`request_age_ms` is time since enqueue, `sample_age_ms` time since the worker copy,
and `context_age_ms` time since the game-state snapshot. `request_lag` counts
subsequent sampling requests. None measures the unknown age of the original
Aurora frame. A missing context is marked explicitly rather than replaced with
current game state. `consumed_sequence` can exceed the reported warning sample
when an earlier spike was retained by the rate limiter.

The worker queue and context history have 64 fixed entries each. Older pending
samples can be overwritten; `samples_dropped` and `sample_loss` expose this, while
per-arena high-water marks survive sample loss. `requests_failed` counts rejected
compute submissions. No worker callback logs or touches game objects. Run epochs
reject queued work from before a reset. Unknown Dusklight versions, missing
symbols and graphics registration failures produce explicit measurement status;
scene context remains available without guessed capacities.

Warnings are passed to the host logger as they become reportable, rather than
being held only for a shutdown dump. The mod cannot force the host logger or OS
to flush durable storage. It cannot catch Aurora's fatal overflow, guarantee the
last line survives a process crash, or measure a frame that aborts before the
callback. Native actors/effects and other mods are outside the owned effect counts.

### Presentation budgets and pressure source

`SurvivorsBuffer` records and the once-per-second Performance Monitor now include:

- `buffer_max_mask`: arena(s) with the highest utilization in this measured sample.
- `tier_trigger_mask`: arena(s) that last raised the adaptive tier, retained during
  hysteresis until recovery to tier zero. It is not necessarily the current maximum.
  Both masks use vertex=1, index=2, uniform=4, storage=8; ties combine bits.
- `hud_bars`, `hud_hp_numbers`, `hud_damage_numbers`, `hud_text_passes`: actual
  submissions from the last Dawnlight enemy-overlay draw, not other mods' UI.
  Text passes include outlines. These are game-thread context, not GPU attribution.
- `te_hud_mask`: enabled TE providers (bars=1, exact HP=2, damage numbers=4).
  Zero does not imply other mods have no UI. Boss/stamina settings are not measured.
- `shadow_budget_available`: whether both optional shadow hooks registered.
- `shadow_allowed_total`, `shadow_suppressed_total`: cumulative normal-enemy shadow
  submission decisions since the current monitoring epoch began. Allowed means
  passed to the next hook/native code, not necessarily drawn. Replays count again;
  these are not unique actors, per-frame counts or byte savings.

Diagnostic presentation fields are copied with their request context. A missing
context (`context_missing=1`) has no valid presentation values. Pressure thresholds
are 70/80%; warning thresholds remain 80/90/95%. None of these delayed samples can
catch an engine abort or guarantee a buffer reservation for subsequent UI.
