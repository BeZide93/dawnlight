# Hard Mode

[← Dawnlight overview](../../README.md)

Open `Mod Manager -> Dawnlight -> Open Dawnlight Settings -> Hard Mode` to
configure Dawnlight's combat difficulty options. The Hard Mode switches
are independent and disabled by default:

- `Enemy Hard Mode` makes supported regular enemies more aggressive without
  changing their health or damage. Selected attack, waiting, and recovery
  timers lose five timer points every three frames, shortening those intervals
  by approximately 40%. Supported enemies also track and approach Link more
  quickly, while their animation speed and exact hit, sound, and projectile
  event frames remain unchanged.
- `Boss Hard Mode` enables faster or expanded behavior for Ook, Diababa,
  Dangoro, Fyrus, and Death Sword. It also enables the additional Ganondorf
  arena projectiles in Boss Rush.
- `No Normal-Hit Invulnerability` removes Link's post-hit invulnerability after
  normal damage. Knockdowns, launches, wall impacts, and landing reactions keep
  their normal invulnerability window. This option defaults to Off and remains
  independently editable while Dawnlight Mode is on.

### Cave of Ordeals Randomizer

`Cave of Ordeals Randomizer` replaces each placed combat enemy in `D_SB01`
with a random, different enemy type when the room's actors load. It is Off by
default and independent of Enemy Hard Mode, Boss Hard Mode, and Dawnlight Mode.
Reload the Cave after changing the option: actors already loaded, including
preloaded adjacent rooms, are not changed in place. Reloading a room can roll a
new layout.

- One replacement per authored combat spawn: the original room, scene ownership,
  and actor set ID are retained. No additional encounter enemies are spawned.
- Ceiling and wall placements are projected onto the lowest valid floor beneath
  the slot **in the same room**. Small horizontal offsets handle wall placements;
  flying replacements start 200 units above that floor. Original ceiling flags,
  paths, rotations, scales, and switches are replaced with standalone settings.
- The pool contains 28 variants: Bokoblins, Mini Freezards, Keese and Bubbles
  (normal/fire/ice), Tektites, Gibdos, Chilfos, Stalchildren, Rats, club and bow
  Bulblins, Aeralfos, Bomskits, ground Deku Babas, Helmasaurs, Helmasauruses,
  Kargaroks, Puppets, Lizalfos, Dodongos, Dynalfos, ground Skulltulas, Stalfos,
  and Darknuts. Each eligible entry has equal probability; entries sharing the
  original actor profile are excluded. There is no per-room Darknut limit.
  Splitters, generators, and bosses with arena scripts are not replacement types.
- NPCs, Great Fairies, doors, chests, items, and room-clear logic stay native.
  Projectiles and runtime children are not rerolled. Invisible Wolfos pack
  coordinators and their type-dependent children stay native to preserve their
  enemy count and prevent invalid child casts; directly placed wolves are rerolled.
- Enemy creation waits for room collision. If no safe floor is found within
  120 creation attempts, that slot keeps its original enemy and parameters.
  This fallback neither removes the enemy nor stalls room loading indefinitely.

Poes are combat enemies too; a randomized replacement does not award a Poe soul.
Use the native Cave layout when collecting those souls.

Enemy Hard Mode also adds selected enemy-specific mechanics: every second
Bulblin bow shot becomes a three-arrow spread, every Fire Toadpoli shot and
every second Water Toadpoli shot become three-ball spreads, Chilfos lead thrown
spears, selected Stalhounds can immediately follow up a pounce, and Bokoblins,
Lizalfos, and Dynalfos only suffer knockdown from every second normal combo
finisher.

Enemy HP scaling and enemy damage scaling remain separate settings and can be
combined freely with these options. **HP Scaling** defaults to 100% and remains
independently editable while Dawnlight Mode is on; enabling the mode does not
force 300%. See
[ENEMY_HARD_MODE.md](../../ENEMY_HARD_MODE.md) for the complete enemy and boss profile
list, exact multipliers, exclusions, and implementation details.
