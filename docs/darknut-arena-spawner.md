# Enemy Spawner: scene ownership and arena admission

Status: the user confirmed the fix in `bd7347d` working in-game on September 21,
2026, after testing the spawn/visibility/hub-return issue. GitHub Actions Build
[#342](https://github.com/BeZide93/dawnlight/actions/runs/35645597162) passed for
that code revision. This is the reference implementation for future enemies.

## Root cause of duplicate drawing and hub-return crashes

`spawn_enemy_for_testing` runs from a mod settings UI callback. The ActorService
forwards creation to `fopAcM_create`; `fpcM_Create` records `fpcLy_CurrentLayer()`
in the asynchronous creation request. The caller's current layer is not
necessarily the player's scene layer. Setting `room_num` only sets the room
field: it does not set scene ownership.

A UI request on the root layer creates a root-owned enemy. This violates two
engine assumptions:

- `dScnPly_Draw` draws actors through the global actor draw queue. The root
  traversal in `fpcM_DrawIterater` also draws a root-owned enemy directly.
  Submitting the same intrusive J3D packets twice creates cycles and can destroy
  links to other packets, including Link's. Breaking the resulting cycle cannot
  reconstruct the lost links.
- Scene deletion removes the scene's descendants. A root-owned enemy survives
  removal of the player and can dereference the missing player during its next
  update.

The device logs first showed an index staging-buffer capacity abort, then
confirmed repeated cyclic material queues with the temporary draw guards. The
`211904` log shows a different failure: a SIGSEGV at `cLib_targetAngleY` with fault
address `0x5ac`, during the spawned Bomskit's update. This matches a missing
player position during the reported hub transition. Together with Link becoming
invisible after any spawn, these observations fit the scene-ownership failure
above. The subsequent device test confirmed the fix working.

## Current fix

`create_test_actor_in_player_layer` temporarily selects Link's owner layer while
submitting the asynchronous ActorService request. It restores the caller's
previous layer on success, failure or exception. The request retains the player
layer after restoration. Missing players, root/null player layers, deleting
layers and pending stage transitions are rejected.

Native scene traversal now owns both drawing and deletion. The temporary J3D
material/shape/line cycle-repair hooks from `0a66046` and `4912865` have been
removed: they treated damaged queues after the duplicate draw had already
occurred and could not recover discarded packet links. No renderer buffer size
or model visibility flags are changed.

The following arena fixes remain:

- Manually spawned processes use native switch/completion queries, including
  base creation before `actor_type` exists. Native room objects see the cleared
  arena state. No real Temple of Time save bits are written.
- Successful child requests inherit the spawner exemption. Bomskit eggs must
  pass room admission before their initialization; otherwise native deletion
  calls `stopAnime` on an uninitialized sound object. Descendants are tracked
  individually and ordinary native children remain unaffected.
- The private arena remains empty of authored bosses, supplies and event tags.

## Adding another enemy

The scene-ownership fix is shared by the entire spawner; **do not copy it into
individual enemy implementations**. New entries using the existing path inherit
it automatically.

1. Add the UI label to `kEnemySpawnerProfileLabels` in
   [`src/enemy_spawner.hpp`](../src/enemy_spawner.hpp) and update its array size.
   Add the matching profile and parameters at the same index in
   `kEnemySpawnerProfiles` and `kEnemySpawnerParameters` in
   [`src/enemy_spawner.cpp`](../src/enemy_spawner.cpp).
2. Inspect the native enemy's Create/Execute/Delete functions before choosing
   parameters. Use the profile's actual no-switch/no-path sentinel in the
   appropriate bit fields; zero is not a universal standalone configuration.
   Check parent/boss ownership assumptions and environmental requirements such
   as water, lava, ceiling attachment or time of day. Keep any necessary
   actor-specific adaptation restricted to tracked test actors.
3. Keep creation routed through `spawn_enemy_for_testing` and
   `create_test_actor_in_player_layer`. Supply room and position separately from
   ownership. The helper must select `player->layer_tag.layer` **before** calling
   ActorService, then restore the previous layer. Moving the actor after creation
   or changing only `room_num` does not protect the asynchronous creation path.
4. Preserve ID registration immediately after a successful request. Arena
   admission runs before `fopAcM_IsActor` becomes true, so tracking must begin at
   the process-ID level. The nested pre/post process scope lets native room gates
   keep their cleared state while spawned actors query native save state.
5. Check children and projectiles. `after_create_child` automatically tracks
   successful `fopAcM_createChild` requests whose parent is tracked, including
   descendants. Their native creation during the parent's process runs in its
   scene layer. If a new enemy uses another creation API or an external callback,
   inspect both scene ownership and ID inheritance explicitly; the current hook
   does not cover every possible creation API. Do not grant the exception to all
   native room actors or remove child IDs when only their parent is deleted.
6. Run the regression scripts below, build the mod, and perform the device
   checks. Update the relevant tests if the new profile needs extra lifecycle
   handling. This fix does not override the existing mirror-hub spawn restriction.

### Where each responsibility lives

| Responsibility | Implementation |
| --- | --- |
| UI entry point and profile selection | `spawn_enemy_for_testing` in `src/enemy_spawner.cpp` |
| Correct owner layer and caller-layer restoration | `create_test_actor_in_player_layer` in the same file |
| Tracked descendants | `after_create_child` in the same file |
| Early/nested process context and per-ID deletion cleanup | `before_process`, `after_process`, `s_testActors` in the same file |
| Cleared native arena versus spawned actors | `on_bossrush_area_switch_pre` and `on_bossrush_area_dungeon_bit_pre` in `src/new_save_modes.cpp` |

### Diagnosing a similar regression

- Enemy disappears before resource loading: check room admission and whether its
  ID (or child ID) is tracked before base creation.
- Link disappears, material lists cycle, or the renderer's index buffer fills:
  check owner layer and duplicate draw traversal before changing renderer limits.
- An enemy updates after a scene change and dereferences a missing player:
  check that it and its pending creation requests belong to the scene being
  deleted.
- Bomskit egg deletion crashes in `stopAnime`: check rejection before profile
  initialization, not only the egg's Delete method.

The earlier queue-cutting hooks were a temporary diagnostic workaround. Do not
restore them as the standard solution: once a duplicate submission overwrites a
packet link, cutting the cycle cannot recover the lost geometry. There is no
remaining requirement to increase GPU buffers or force Link visible.

## Validation

- `python tests/enemy_spawner_scene_test.py` exercises the production creation
  helper against stubbed engine layer rules. It reproduces the old double draw,
  lost Link packet and orphan actor, then checks scene ownership, single drawing
  and scene cleanup with the fix. It also checks asynchronous ownership capture,
  layer restoration, error/exception paths and unavailable scene states.
- `python tests/bossrush_cave_routes_test.py` checks room isolation, nested
  process context, child admission/cleanup, Cave routes and Continue behavior.
- `python tests/bossrush_warp_test.py` checks the existing warp lifecycle.
- C++ syntax checks cover both modified units against the pinned SDK.

Device regression checks for each future enemy: after a fresh launch, spawn several enemy types, verify Link stays
visible, then return to the hub. Re-enter the arena and repeat with Bomskit eggs
and Deku Baba. The old queue-guard messages should no longer occur because those
hooks have been removed. PR23 is unchanged; no Twilit Essentials code is used.
