# Twilit Essentials Boss Rush hub

The hub uses the Temple of Time Darknut chamber with the animated miniatures
and floating labels adapted from
[Twilit Essentials](https://github.com/BeZide93/dawnlight-twilit-essentials/tree/805fac000711135ee2d85e086ab6100d767717bd/src/boss_rush),
revision `805fac000711135ee2d85e086ab6100d767717bd`.

Approach a miniature and call Midna to open **Fight [boss]? — Yes / No**,
using Dawnlight main's confirmation flow. Only **Yes** starts the replay;
**No** or cancel keeps Link in the hub. The direct **A / Fight** action is removed.
Defeated boss names turn red. The central portal starts Dawnlight's complete run. The
exit door leads directly to the Cave of Ordeals; there is no separate Cave portal.
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
  Cave exit door followed by Midna's return option.
