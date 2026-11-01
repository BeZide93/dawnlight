# Survivors enemy health HUD

The Survivors settings tab (including the hub's run settings) offers **Show HP
Bars**, **Show Exact HP Numbers**, and **Show Damage Numbers**. All three default
to off and persist in Dawnlight's ordinary settings, including settings
export/import. Display preferences can change during a run; they do not alter
rank, combat damage or run scoring. Exact numbers require either local or
Twilit Essentials HP bars.

Presentation and feature design are adapted from **F1mmel (@F1mmel)**'s
[Twilit Essentials HP bars](https://github.com/BeZide93/dawnlight-twilit-essentials/blob/7682060c20e2e3757b2888a27bbd958506ccb4f7/src/hp_bars/hp_bars.cpp):
gold-bordered health bars with green/yellow/red fill, trailing damage chips,
outlined current/max HP and fading floating health-loss numbers. The integration
uses Dawnlight's native J2D rendering and tracked Survivors enemies, including
bosses; it does not add global actor scans or hooks.

## Twilit Essentials ownership

Live provider keys are read through the host's existing ConfigVar resolver:

| Survivors setting | Dawnlight key | TE key |
| --- | --- | --- |
| Show HP Bars | `survivors-hp-bars` | `hpBarsEnabled` |
| Show Exact HP Numbers | `survivors-hp-numbers` | `hpBarsShowNumbers` |
| Show Damage Numbers | `survivors-damage-numbers` | `damageNumbersEnabled` |

TE keys live under `mod.com_dusklight_twilit__essentials.`. Its `enabled` key
must also be true; retained values from a disabled TE instance do not suppress
Dawnlight. TE exact numbers are active only with TE HP bars enabled. Each active
provider feature disables just its matching Survivors toggle and displays a
short “managed by Twilit Essentials” notice. Local saved preferences are never
overwritten. Local numbers can accompany TE bars if TE's own numbers are off.
Provider-owned ConfigVar pointers are looked up afresh on every refresh.

## Runtime limits and lifecycle

The existing process callbacks supply initial and post-execution HP snapshots.
Only actual health loss produces numbers, including lethal hits; first sightings,
healing, changed maximum HP and reused actor slots do not. A provider handoff
clears pending feedback and starts a fresh baseline. Despawning clears the slot;
run reset/restart and stage changes clear all state. Existing Survivors pause,
menu, defeat, victory and hub guards also apply to this overlay. Animation ages
only on active player ticks. Dawnlight adds no overlay in other game modes.

Work is bounded to the existing tracked slots, the nearest 64 visible enemy
labels, 96 concurrent damage numbers, and at most eight line-of-sight queries per
active tick. Visibility checks rotate across nearby candidates. Labels fade out
before 4,500 world units and are culled behind the camera or outside the screen.
With Performance Mode enabled, [adaptive HUD budgets](survivors-performance-mode.md#enemy-hud-and-normal-enemy-shadows)
reduce these limits, prioritize the lock-on target, combine short bursts of damage
and simplify text outlines. TE-owned draws remain outside those budgets.
With all local effects off, observation, animation and drawing return immediately.
Native GX state is restored between text and rectangle draws.

## Validation

`tests/survivors_enemy_hud_test.py` executes the production implementation with
SDK service structures and engine fixtures. It covers independent provider
ownership, retained TE values, disabled setters, settings reload, concurrent
settings windows and closed-document cleanup, health baselines, lethal damage,
slot reuse, pause-safe animation, bounded visibility/popups, culling, drawing
state and lifecycle reset. Existing process/lifecycle/HUD/settings fixtures cover
the integration boundaries. Device playtesting should check font scale, anchor
height and provider coexistence in a dense wave on handheld displays.
