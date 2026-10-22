# Survivors freeze diagnostic build

This temporary diagnostic PR starts from main after PR #151. It investigates
reported multi-second stalls following the correction of Link's Gerudo Desert
immunity. Build #1123 (`1ec429f`) introduced contact lifetime protection; the
later `3cfd61c` correction preserves native incoming hits without removing the
outgoing stale-contact guard. This build does not revert either change, change
hit acceptance, add invulnerability, modify archive retention, or enable Debug.

## Getting a diagnostic build

Every platform artifact and `mod-combined` from this branch's Build workflow has
`DAWNLIGHT_FREEZE_DIAGNOSTICS=ON`. Install the `.dusk` from that workflow normally.
No Debug tab or settings toggle is needed. Local/default builds keep diagnostics
off; pass `-DDAWNLIGHT_FREEZE_DIAGNOSTICS=ON` to CMake to enable them manually.
Before using this PR for a production release, remove the workflow's diagnostic
opt-in. The compiled-out default contains no reporter or extra hooks.

1. Fully restart Dusklight with the intended mods already installed. Keep the
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
claiming a performance regression or improvement.

## Validation

The deterministic production-reporter fixture covers optional hook half-failures,
long calls, live archive names, counters, timestamp/build identity, nested calls,
foreign post callbacks, bounded stack overflow, rate limiting, normal heartbeats,
logging overhead in update gaps, mode changes during scopes and shutdown. It also
compiles the disabled module without engine dependencies. Contact lifetime tests
(including ASan/UBSan) and archive retention tests run with instrumentation both
compiled out and enabled behind a reporter fixture. Native platform CI builds
and iOS preparation checks validate the actual SDK hook metadata.
