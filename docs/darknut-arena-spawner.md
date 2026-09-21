# Enemy Spawner: scene ownership and arena admission

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
above. The new patch still requires device validation.

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

Device check: after a fresh launch, spawn several enemy types, verify Link stays
visible, then return to the hub. Re-enter the arena and repeat with Bomskit eggs
and Deku Baba. The old queue-guard messages should no longer occur because those
hooks have been removed. PR23 is unchanged; no Twilit Essentials code is used.
