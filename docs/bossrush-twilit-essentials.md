# Twilit Essentials Boss Rush hub

The hub uses the Temple of Time Darknut chamber with the animated miniatures
and floating labels adapted from
[Twilit Essentials](https://github.com/F1mmel/dusklight-twilit-essentials/tree/1e7fb0afb828f924c165e8c57b7337304ed6cf4c/src/boss_rush),
revision `1e7fb0afb828f924c165e8c57b7337304ed6cf4c`.

Approach a miniature and call Midna to open **Fight [boss]? — Yes / No**,
using Dawnlight main's confirmation flow. Only **Yes** starts the replay;
**No** or cancel keeps Link in the hub. The direct **A / Fight** action is removed.
Boss miniatures are blue, translucent holograms (44% opacity), switching to red
after the first saved defeat. Weapons, multipart bosses and Ganondorf's animated
cape share their owner's color. Defeated boss names also turn red. The central
portal starts Dawnlight's complete run. The
exit door leads directly to the Cave of Ordeals; there is no separate Cave portal.
While visiting the Cave in Boss Rush, its entrance exit and all Great Fairy
return destinations lead back to the Boss Rush hub. Internal floor transitions
and Cave retries keep their original destinations; ordinary game saves and
return-to-title/reset transitions are unaffected.
Oil/red-potion stations and the fairy stand on the raised landing behind Ook.
The small room beyond it contains three small chests for normal bombs,
Bomblings and water bombs (30 each, subject to bag capacity). Their placement
uses the original reward chest's position and facing, with 220-unit spacing.
The landing is located using the room collision, checking both width and depth
to avoid placing the bottles on a stair tread; its height is not the arena floor.

The two existing pots on the left give Deku seeds; the three on the right give
30 arrows each (viewed from the arena toward Ook). These are direct drops, not
random heart tables, and use each pot's original position even when carried.
The two additional Dawnlight pots are removed. Native Darknut encounters are
unaffected. Custom music and Midna's third **Return to Hub** choice remain.

## Separate hub identity

Dawnlight uses `D_DLBR0`, room 51, as its hub identity. Essentials checks for
`D_MN06B`, room 51, so its existing physical-room check does not match this hub.
The runtime stage name stays `D_DLBR0` throughout loading and gameplay.

Three narrow hooks provide the original assets without copying game archives:

- Stage resource requests map `/res/Stage/D_DLBR0/` to `/res/Stage/D_MN06B/`.
- The room-map cache reads `D_MN06B/room*.dzs` when MULT data is needed.
- Audio scene lookup uses the Darknut arena's original sound banks.

These hooks register before save loading. Return places and restarts use the
alias. Existing PR hub saves are migrated; actual Darknut fights retain the
original stage. This isolates the hub even with an older Essentials build.
It does not change Essentials' behavior in other vanilla boss rooms; its
separate session-ownership fix remains useful for those encounters.

Hub-only completion/switch overrides and the dungeon/zone reset suppress
Darknut and the gate-opening demo. Native reward chests are removed before
Dawnlight's supplies are spawned. SaveService keeps defeated-boss progress
separately. The encounter indices, save schema and full-run finale are unchanged.

The Enemy Spawner is enabled in the hub. Its tracked actors use real switch
and boss-completion queries during their process methods, including generic
actor creation. This prevents the room's enemy-appearance gate from deleting
test enemies while retaining the native Darknut/gate suppression. Unrelated
actor calls do not inherit the exception, including nested calls.

Hologram rendering is scoped to the gallery's model instances. Their packets
enter the translucent draw buffer; a shape-draw hook applies the color and
alpha blending after normal materials load, then restores the original GPU
state. Shared archive materials are not recolored, so Enemy Spawner actors and
real boss replays retain their appearance. Depth testing stays enabled and
depth writes are disabled; texture alpha preserves cut-out surfaces. Death
Sword uses this same draw path instead of the invisible-model pass. Ganondorf's
separate cloth simulation feeds a matching translucent cape packet.

Gallery loading/drawing use the live stay room, including the first arrival
from a new save. Models use a persistent heap; archives are retained once,
failed loads can retry, and callbacks are restored before resources are freed.
Teleports retain the Essentials-style fades and departure/arrival warp sound.

## Ganondorf replay restored from main

The direct replay uses Dawnlight main revision
`3d38d31e759150fd2a63d25a5f5251cf6c0fd861`: `D_MN09C`, point 0, room 0,
actor/cape readiness, demo state 92, and the existing barrier handling with
switches 15/31. Its arena height remains 1100 independently of the new hub.
The barrier override applies only to the duel; the Darknut hub needs no wall.

The Essentials `D_MN09B` replay, dungeon-switch override, Link execute hook,
Midna call overrides and forced dialogue-root routing are removed. Dawnlight's
main Midna menu implementation, including its third return choice, is restored.
Hub entry also clears the Midna-suppression flag and restores the utility event
bit changed by the earlier PR duel so old PR saves can use the restored behavior.

## Validation

The Linux RelWithDebInfo build and a standalone harness check alias resource,
map-cache and audio routing, isolation from vanilla rooms, and restored duel
functions against main. GitHub Actions builds the configured platforms.
Visual behavior and live mod interaction still need an in-game check:

- Create a Boss Rush save and check all 18 miniatures on the first arrival.
- At a boss miniature, verify the Midna prompt, Yes/No/cancel behavior and
  re-arming after walking away. A alone must not launch a fight.
- Load an existing PR save with both mods active: check one gallery, supplies,
  music, no live Darknut and no gate-opening cutscene. Check bottles/fairy on
  the landing behind Ook and all three bomb chests inside the rear room. Break
  both left pots and all three right pots, then revisit to check their respawn.
- Replay Ganondorf, check the light wall, call Midna and use the third option.
  Repeat after returning to the hub, and check death/retry and boss defeat.
- Test a real Darknut replay, the central full run, saving/resuming, and the
  Cave exit door followed by Midna's return option. Leave the Cave via its
  normal entrance exit and each Great Fairy return choice on deeper floors;
  verify the hub spawn, camera, supplies and gallery, then re-enter the Cave.
  Continuing to another Cave floor must not return to the hub.

Hologram validation: check every miniature and its attachments before/after a
first defeat and after saving/reloading. View the room through each figure,
check walls still occlude it, and spawn the same enemy beside its miniature
to check appearance isolation. Inspect Death Sword and Ganondorf's cape.

## Recovery after the initial hologram build

The first hologram implementation could sample a TEV order's texture coordinate
without checking whether that coordinate was active. Aurora leaves inactive
pipeline texgens at `GX_MAX_TEXGENSRC` (21), then aborts shader generation with
`unhandled tcg src 21`. Pipeline descriptions are cached before compilation;
reloading the bad description can therefore abort the next app startup too.

Hologram alpha sampling now requires an active, in-range coordinate and a
supported independent matrix generator/source. Other materials use constant
hologram opacity with no texture generators. Blue/red progress colors remain.
The regression harness rejects zero generators, inactive higher slots, source
21 and dependent emboss generators; valid texture cut-outs still work. The
zero-generator case fails against the original implementation.

If an affected build already poisoned the cache, first force-stop Dusklight
and replace the mod with the fixed build. In the Android system file picker's
**Dusklight Data** root, remove only `pipeline_cache.db` and, if present,
`pipeline_cache.db-wal` and `pipeline_cache.db-shm`. These are regenerable
pipeline-cache files in the app's data directory (the reported installation
uses `/data/data/dev.twilitrealm.dusk/files`). Keep saves, mods, `config.json`
and other files. Android's ordinary **Clear cache** button may not remove
these files because they live in the app's files directory. Do not use
**Clear storage/data** for this recovery. Pipeline preloading happens before
mod initialization, so installing the fixed mod alone cannot repair an already
cached invalid pipeline. The next launch rebuilds the removed cache.
