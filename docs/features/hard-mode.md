# Hard Mode

[← Dawnlight overview](../../README.md)

Open `Mod Manager -> Dawnlight -> Open Dawnlight Settings -> Hard Mode` to
configure Dawnlight's combat difficulty options. The three Hard Mode switches
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
  their normal invulnerability window.

Enemy Hard Mode also adds selected enemy-specific mechanics: every second
Bulblin bow shot becomes a three-arrow spread, every Fire Toadpoli shot and
every second Water Toadpoli shot become three-ball spreads, Chilfos lead thrown
spears, selected Stalhounds can immediately follow up a pounce, and Bokoblins,
Lizalfos, and Dynalfos only suffer knockdown from every second normal combo
finisher.

Enemy HP scaling and enemy damage scaling remain separate settings and can be
combined freely with these options. See
[ENEMY_HARD_MODE.md](../../ENEMY_HARD_MODE.md) for the complete enemy and boss profile
list, exact multipliers, exclusions, and implementation details.
