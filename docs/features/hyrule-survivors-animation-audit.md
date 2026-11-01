# Survivors animation audit — PR #140

Compared the current scheduling, collision and presentation code with local
history commit `beaa6ab452f43cc96281d4009919d567128e556b`
(`Run Survivors abilities concurrently and preserve rapid independent hits`)
and its parent. Findings below are source-level conclusions, not an in-game
visual certification.

| Ability / interaction | Confirmed finding | State after this patch |
| --- | --- | --- |
| Spin Attack / Great Spin | The concurrency patch removed `procCutTurnInit` and replaced the native action with a six-frame collider/particle pulse. Link never performed the spin. | Fixed: native action, effect expansion and hit window restored; no extra pulse or duplicate damage. |
| Normal sword versus item poses | `update_sword` could start an ordinary body action while a cosmetic upper-body item pose was active; `update_ranged` then cancelled the pose because the body was no longer idle. Sustained sword use could suppress presentation entirely. | Fixed: ordinary cuts/facing yield; an item pose may overlay an already-running ordinary cut and is retained until completion or a higher-priority interruption. Sword Wave wind-up is treated as an ability. |
| Shield Attack | The patch deleted `update_shield_attack` and its `procGuardAttackInit` call. The independent frontal hit still damages/staggers, but has no native shield-bash action. | Fixed: native shield bash and its own damage capsule/window restored; one hit per swing. A busy body slot leaves the bash ready. Shockwave keeps its independent aiming option. |
| Hero's Bow | The draw/reload phase `ARELORD` was removed. Arrows launch before optional `ASHOOT`; there is no draw-to-release sequence. | Fixed for presented casts: ARELORD draw/reload followed by ASHOOT and a single release; bow model animates with each stage. Other concurrent shots remain independent. |
| Boomerang and Bombs | Native frame-triggered releases (5.5 and 4 respectively) were removed; projectiles launch before the throw pose. `released` starts true, so `draw_ranged` omits their held models. | Fixed for presented casts: native release cues restored, held models remain attached until release; current reticle or automatic target is sampled at release. |
| Ball and Chain | The throw-frame release and projectile-linked `IBPULL`/`IBCATCH` stages were removed. The `pulling`/`catching` fields remain but are never set true. | Throw cue and held ball restored. IBPULL/IBCATCH remain open; the chain projectile still flies and returns independently. |
| Simultaneous ranged weapons | Only one upper-body pose exists. `fire_items` tries Bow, Boomerang, Bombs, Ball in fixed order. `s_lastRanged` is written but no longer selects the next pose, so simultaneous ready weapons favor the earlier item. A shot that misses the pose slot is not queued for later presentation. | Improved: rotate the visible slot among ready weapons. One body cannot display simultaneous poses; shots outside that slot still fire without their own pose and are not queued for later visual replay. |
| Sword Wave | The concurrency patch initially detached the wave from a sword swing. Later commit `9b4b744` restored the casting swing and release frame. | Already restored; this patch additionally protects the wind-up from Spin/item pose replacement. |
| Jump Strike | The concurrency patch retained `procCutLargeJumpInit`, native launch/landing actions and their animation rate, adding only per-activation hit tracking. Later steering changes retain those actions. | No corresponding animation removal found. Native jump/landing actions remain protected while independent shots can continue without their own pose. |

The follow-up restores Shield, Bow, Boomerang and Bomb presentation without
serializing already-released projectiles. Interrupted, unreleased casts refund
their cooldown. Normal sword attacks retain the lowest presentation priority.
Ball pull/catch remains a separate open item. Live animation blending still
requires an in-game check, particularly with several weapons ready together.

The Stallord/Death Sword visibility issue is separate from this player-animation
patch: their native effects used dungeon-bank particle IDs unavailable in other
arenas. Those resources are now imported and mapped in owned actor scope.

## Verification and remaining runtime checks

Production-code regression fixtures cover native Spin entry, ordinary-cut/item
priority, protected wave wind-up, simultaneous projectile release and per-swing
hit deduplication. Grounding tests distinguish the former 240-unit placement
limit from airborne deaths at 300, 1,500 and 12,000 units, and preserve boss spawn
altitude separately. Phantom Zant tests cover direct and queued heavy hits,
existing stagger and lethal damage. Particle tests cover all imported effects,
texture remapping, actor scoping and unload; they use archive fixtures rather
than copyrighted game assets.

An in-game pass is still needed for Spin/Great Spin blending, the exact Fire Edge
flame appearance, Stallord/Death Sword trails outside their dungeons, and the feel
of tenfold knockback. Compilation and isolated regressions cannot verify those
rendered/gameplay details.

## Arena follow-up

- Throne Room uses the native final Ganondorf combat cue. Ownership is released
  on death, restart, stage transition and shutdown; a replacement area's cue is
  never stopped by stale cleanup.
- Enemy Collision defaults off and controls Link/owned-enemy body contacts,
  including child actors. Different run-owned enemies always pass through each
  other, independently of this toggle and Performance Mode. Same-owner internal
  contacts, attack/target checks and objects are unaffected.
- Bosses and elites ignore all Survivors knockback, including pending motion.
- Darknut's high/low guard and Ganondorf's defence block frontal owned attacks
  before damage attribution and on-hit effects; rear attacks remain effective.
- Puppet Zelda's attack clock and attack animations run at 2x; her movement and
  damage animations retain their normal rates.
- Post-run save actions use separate vertical navigation rows. Gameplay menu
  entry points are gated during results and save handoff. Native save creation
  must remain globally unpaused: its initializer waits while the pause flag is
  set. The post-run softlock regression is covered against that native gate.

Regression fixtures exercise release-once timing, interruption refunds, native
Shield hit deduplication, guard direction, collision pair scope, music handoff,
Zelda's non-compounding rate and result action navigation structure. Actual
rendering, audio transitions and controller feel still need runtime testing.
