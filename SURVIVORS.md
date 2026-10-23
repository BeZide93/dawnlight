# Hyrule Survivors

[← Dawnlight](README.md)

A 20-minute survival mode included in `dawnlight_mod.dusk`. Select **Hyrule
Survivors** in Dusklight's game-mode menu and create/load its separate save.
You arrive in the freely walkable Darknut Arena hub. Interact with the central
Master Sword using A to open **Talents / Settings**; closing saves automatically.
Interact with the exit door beneath **Start Run** to open map selection. Pick a
map and press **Start run**: the menu closes, the native door opens, and Link walks
through before the scene changes to the selected arena. Story and Boss Rush saves remain separate.
The Dusklight menu shortcut (keyboard, controller or three-finger tap) closes
this setup menu and opens Dusklight. Closing Dusklight returns to setup;
your map, difficulty, settings and allocated talents remain unchanged.

## Run results

The results screen puts damage first: active damaging abilities are sorted by
actual damage dealt, with level, damage, DPS and a percentage bar. Abilities that
dealt no damage are omitted. The side panel shows score, map high score, run
level, elites, chests, meta progression and equipment. Save records or return to
the hub using the bottom buttons.

Damage counts enemy HP actually removed, excluding blocked hits and overkill.
DPS divides by the full active run time; menus and other pauses do not inflate it.
Simultaneous hits share the HP loss proportionally, rounded to whole HP while
preserving the total. Critical damage remains with its ability. Passive upgrades
have no separate rows: extra Dual Wield hits count as Sword Attack, Fire Arrows
and Bomb Arrow as Bow, Fire Bomb as Bombs, and Fire Edge as the ability that
ignited it. **Dark Link** separately shows its extra multiplier damage (including
boosted burns) and its Dark Aura damage; those points are excluded from the other
rows. Each new high score keeps its damage details for later viewing; live
damage counters reset for the next run. Older saves retain their progression.

## Playing a run

- Move and dodge while Link attacks automatically. Cliff jumps follow movement,
  including sideways/backward combat movement. Automatic attacks and their cooldowns
  pause during jumps, Gale charging and gliding; released projectiles finish normally.
  Collect blue XP crystals and choose one of three power-ups at each level-up. Selection supports controller,
  keyboard and touch; menus and Get Item animations pause the run.
- Press or hold the sword button to aim unlocked attacks manually. **Settings →
  Manual aiming abilities** lets you toggle Sword, Spin, Jump Strike, Sword Wave,
  Bow, Boomerang, Bombs, Ball and Chain, Shield Attack and Shockwave independently.
  All default to on. Off always keeps automatic targeting; on follows the reticle
  during manual input. Unlocks and cooldowns still apply. Jump Strike jumps in
  place, stays steerable and requires a nearby enemy; Spin remains centered on Link.
  Elemental perks inherit their weapon's aim; auras/passives remain automatic.
- Holding attack uses the third-person shoulder view and reticle. Projectiles
  converge on the reticle within their reach; bombs keep their arc and boomerangs
  retain earned follow-up hops. Bow/Wave multishot includes a centered projectile.
  Releasing restores the normal camera. The native charged spin remains disabled.
- **Gyro Aim** defaults to on for this view. **Switch Attack/Lock-on Keybinds**
  defaults to off; enabling it also moves the touch sword icon from B to L.
- **Settings → 3rd Person Reticle Y** adjusts only this Survivors sword-aim view
  from -40 to +40 (default 0); positive values move the reticle up. It is separate
  from Dawnlight's general item-aim setting and applies to the next run.
- Start with three hearts and normal sword attacks at 130% of the original base melee range. Unlock Sword Wave and
  Spin Attack through level-ups. Passive upgrades, including Heart Container,
  remain available independently of other choices (maximum 20 hearts).
- The HUD shows time, run level, score, XP and the next chest price. Inspect equipment
  in the item wheel and power-ups via the Hidden Skills scroll in Collection.
  Only bottles can be assigned to item buttons. Magnet also attracts rupees,
  hearts, heart pieces and small keys; nearby pots trigger normal automatic sword swings.
- Call Midna for **Cancel**, **End Run** or **Restart Run**. Survive 20 minutes and defeat all four timed elites to win;
  **New run** resets upgrades and returns to the hub. Map exits are sealed during runs.

## Run setup

The door menu has **Maps**, **Difficulty**, **Settings** and (in debug builds)
**Debug** tabs. The sword menu has **Talents** and **Settings**. Dawnlight Settings
also has a mirrored **Survivors** tab beside **Boss Rush**. Shared settings and
difficulty persist across app restarts and support settings export/import.
Edit outside a run; values are fixed when the run starts.

**Allow Cheats** defaults to **off**. During a run, all settings in Dusklight's
Cheats tab are temporarily suppressed, including attempts to enable them in the
menu. Your saved cheat preferences return when the run ends, restarts or exits.
Turning Allow Cheats on marks the result **Cheated** in small red text at the top,
even if no individual cheat was used. A second red **Modded** label appears if
any other mod was active during the run, including cosmetic or asset-only mods;
disabling it later does not remove the label. Installed but inactive mods do not
count. These labels select the Cheated high-score list without reducing the
awarded score, XP or progression.

Survivors also suppresses Dawnlight's ordinary gameplay overrides and Enemy
Spawner regardless of Allow Cheats. Its own talents, weapons and balancing
remain in control; your ordinary Dawnlight preferences are retained.

| Setting | Range | Default |
| --- | --- | --- |
| Enemy HP | ×1–×5, ordinary enemies only | ×1 |
| Boss HP | ×1–×5, timed bosses and stationary elites | ×1 |
| Enemy Damage | ×1–×5 | ×1 |
| Enemy Speed | ×1–×5 normal locomotion; attacks keep native movement | ×1 |
| Horde | ×1–×5 enemy population and spawn amount | ×1 |
| Camera distance | 100–500%, walls may limit distance | 200% |

All five difficulty controls use 0.5 steps. At Horde ×N, one in N normal kills
gives XP and rolls a drop (fractional remainders carry forward); pots and elite
rewards are unchanged. Camera 100% is native distance; event cameras stay native.

## Score and permanent talents

Each map has two independent high scores: **Ranked** for runs without Allow
Cheats or other active mods, and **Cheated** for runs with either condition.
Select a score in the Maps tab to reopen that run's result screen, including its
damage breakdown, equipment, integrity flags and awarded score. This view is
an archived snapshot; it does not restart the run or award XP again. Ties keep the original
record. Empty lists show a dash; even a completed zero-score run can be opened.

**Delete Score** at the bottom of each archived result screen deletes only that
map’s selected Ranked or Cheated record after **Yes / No** confirmation (default
No). Success returns to the map list; a failed write retains the score and offers
retry. Other records, unlocked maps, talents, XP and lifetime statistics remain.

**Reset High Scores** is in the normal Survivors Settings tab (also mirrored in
Dawnlight Settings). It is available in a loaded Survivors hub and asks for
**Yes / No** confirmation, defaulting to No. It clears both lists and their saved
summaries for the current save, while preserving unlocked maps, talents, XP and
lifetime statistics. A failed write keeps the original records and offers retry.
Old saves retain their progression, but their unclassified scores cannot populate
the new lists because they did not store integrity flags or result snapshots.

Final score:

| Source | Points before difficulty bonus |
| --- | ---: |
| Survival | 2 per second, up to 20 minutes |
| Normal kills | 10 each, divided by Horde |
| Timed elite / additional elite | 500 / 250 |
| Run levels gained | 50 each after level 1 |
| Complete the run | 3,000 |

Difficulty multiplier: `(Enemy HP + Boss HP + Damage + Speed + Horde) / 5`
(×1–×5). All five controls range from x1 to x5 in 0.5 steps, using the same
minus/plus buttons as Talents. Camera settings do not
change score. Final totals round down; normal kill points are normalized before
applying difficulty. Example: a win at default difficulty with 400 normal kills,
four timed elites and run level 20 earns **12,350 points**.

On victory, defeat or **Midna → End Run / Restart Run**, the final score also grants permanent
XP, shared across maps/runs in this Survivors save. Meta level 1 starts at 0 XP;
the next level costs 2,000 XP, then 2,500, 3,000… Each grants **one Unlock Point**.
Abruptly quitting an unfinished run does not award its score. Old saves retain
previous statistics and start this new progression at level 1.

Interact with the central sword in the hub to open **Talents**. Each rank costs one point;
use **+ / −** to spend or refund points in the scrollable list. Click a perk name
for its description. Removing a prerequisite also refunds its dependent perks;
**Reset all perks** refunds every rank without changing XP, level or high scores.
Changes apply next run; run power-up ranks remain separate. Closing the menu
saves the native slot in the background automatically. **Save progress** also
allows an explicit save/retry while keeping the menu open. A failed write leaves
you in the hub and displays a retry message. The results screen also offers **Save records**.

| Talent | Ranks | Effect / prerequisite |
| --- | ---: | --- |
| Max Health | 5 | +1 starting heart/rank, up to 8; overall health cap stays 20. |
| Crit Chance | 10 | +1 percentage point base chance/rank, up to +10 points before Luck. Combined final chance caps at 100%. |
| Sprint | 1 | Unlock 125% running speed, including automatic combat. |
| Sprint Speed | 2 | Requires Sprint; 137.5%, then 150%. |
| Magnet | 5 | +10% pickup range/rank after run upgrades; includes all magnet pickups. |
| Manual Jump | 1 | Unlock the configured Jump input/touch button. |
| Glider | 1 | Requires Manual Jump; press Jump while airborne. |
| Revali's Gale | 1 | Requires Glider; one charge, 120-second recovery. |
| Gale Charges | 2 | Requires Gale; +1 charge/rank, up to three. Refilled each run. |
| Rerolls | 2 | +1 free reroll/rank/run, up to three; shared by both choice menus. |
| Banishes | 2 | +1 banishment/rank/run, up to three; shared by both choice menus. |
| Dodge | 5 | 5%, 6.25%, 7.5%, 8.75%, then 10% chance to dodge an enemy hit. Independent of Luck; no shield required. |
| Starting Item | 1 | Start each run with a free choice of one of three chest rewards. Normal rarity; shared rerolls and banishes. |
| Auto Rupees | 5 | 1–5 rupees every five seconds of active play. |
| Bonus Damage | 10 | +1 weapon damage/rank, up to +10 before crits and Dark Link. |
| Melee Haste | 5 | +5% normal sword speed/rank, up to +25%. |
| Crit Damage | 5 | +10 percentage points critical damage/rank, up to +50%. |
| Legendary Chance | 5 | +0.6 percentage points base Legendary chance/rank: 1% → 4% at rank 5, before Luck. Applies to item and power-up rarities. |
| Multi Strike | 3 | Requires Dual Wield. Epic grants 1 rank; Legendary grants 2. Each rank adds one sequential activation of Sword Attack (including its splash), Spin Attack, Sword Wave and Lightning Strike. |
| Lightning Strike | 30 | 20% proc on an accepted melee hit, +2 percentage points per Luck rank (60% at rank 20), gated by its own cooldown. See below. |
| Life Regen | 5 | Shortens regeneration by one second/rank: 20 → 15 seconds. |
| Equipment Drop Rate | 10 | +5 percentage points per rank: 20% base → 70% at rank 10, for timed and stationary bosses. |
| Equipment Rarity | 10 | Improves newly rolled equipment stats; see the equipment probability table below. |

Link regenerates **1/4 heart every 20 seconds** even without Life Regen ranks.
Regeneration pauses with the run, continues during the final fight after 20 minutes,
and never exceeds maximum health or revives Link. Each Legendary Chance rank
moves 0.6 percentage points from Common to Legendary before Luck is applied.

**Midna → Restart Run** credits the current run like End Run, then reloads the
same map and returns to run setup without opening the results screen.

The starting selection pauses the timer and combat until you choose. Closing it
keeps the same offers. It grants the reward directly, without a world chest,
and does not increase later chest prices.

Dodge is checked before shield block. Successful dodges and blocks display a
small rising, fading **Dodge** or **Block** above Link; damaging critical hits
display **Crit** above their target. Enemy hits still reduce health, but no longer
make Link flinch. **No Normal-Hit Invulnerability** is always enabled during
Survivors; your saved setting outside this mode remains unchanged.

Automatic sword attacks recover after an interrupted sword draw. Once the sword
is drawn, a ready melee attack, Sword Wave or Spin Attack gets an opportunity
before another item attack puts it away again.

## Maps, elites and pots

| Map | Additional dormant elites | Pots at once | Starting 40-rupee chests |
| --- | --- | ---: | ---: |
| Darknut Arena | None | 6–10 | 3 |
| Hyrule Field | Four at random positions | 40–72 | 18 |
| Gerudo Desert | Four at random positions | 40–72 | 18 |
| Palace of Twilight | One per building doorway | 12–24 | 6 |
| Hyrule Castle Throne Room | None | 8–12 | 4 |

Each map has its own roster. Specialists unlock gradually; once available they
receive 25% of ordinary spawn rolls, reduced to 10% during **HORDE!** events.
The remaining rolls use the basic pool. Full specialist pools fall back to basics.

| Map | Basic enemies | Specialists (unlock time) |
| --- | --- | --- |
| Darknut Arena | Bokoblin, Rat, Keese | Lizalfos (1:00), Helmasaur (2:00), club Bulblin (3:00) |
| Hyrule Field | Bokoblin, club Bulblin, Tektite | bow Bulblin (1:00), Guay (2:00), Kargarok (3:00) |
| Gerudo Desert | Stalchild, club Bulblin, bow Bulblin | Stalhound (1:00), Dodongo (2:00), Gibdo (4:00) |
| Palace of Twilight | Shadow club Bulblin, Shadow bow Bulblin, Shadow Keese | Shadow Beast (2:00), Shadow Kargarok (3:00) |
| Hyrule Castle Throne Room | Bokoblin, club Bulblin, bow Bulblin | Lizalfos (1:00), Helmasaurus (2:00), Dynalfos (3:00) |

Staltroops and Bomskits are excluded. Ordinary bow Bulblins and Guays each cap at
`min(12, 4 × Horde)`; Kargaroks cap at `min(6, 2 × Horde)`;
Gibdos, Dynalfos and Shadow Beasts each at `min(4, 2 × Horde)`. Shadow bow
Bulblins and Shadow Kargaroks use the corresponding bow/Kargarok caps.
Pending spawns count toward caps.
Bulblins take regular arrow damage. New roster enemies use Survivors damage and
HP without puzzle-specific armor immunity, resurrection, Chu merging/splitting
or Gibdo paralysis; their native appearance, movement and attacks remain active.
Random enemies require dry ground and a walkable approach to Link, avoiding
water, isolated ledges and gaps.
**Waiting for a clear enemy spawn position** means the game is retrying because
it could not find a valid position. The warning clears after a successful spawn,
when the population is full, or when the final elite fight starts.
Nearby enemies continue turning toward Link during melee attacks, including
their windup and attack movement. Hurt, death and dormant states are unaffected.

Each map spawns four timed elites:

| Map | 5:00 | 10:00 | 15:00 | 20:00 |
| --- | --- | --- | --- | --- |
| Darknut Arena | Giant Lizalfos | Giant Helmasaur | Dynalfos | Darknut |
| Hyrule Field | Giant Bokoblin | Bullbo with two Bulblin riders | Aeralfos | King Bulblin |
| Gerudo Desert | Giant Stalhound | Stalfos | Death Sword | Stallord’s flying head |
| Palace of Twilight | Giant Shadow Beast | Twilit Bloat | Giant Twilit Carrier Kargarok | Phantom Zant |
| Hyrule Castle Throne Room | Darknut | Darkhammer | Puppet Zelda | Ganondorf |

At 20:00, new horde spawns and guard respawns stop; the fight continues until all
four timed elites are defeated. Giant variants are twice normal size; naturally
large elite species keep their native scale. Elites use the map/time HP budgets described below, multiplied by difficulty,
and use the native Tear-of-Light minimap icon. Dormant guards remain
giant Bokoblins except in the Palace, where they are giant Shadow club Bulblins. Dormant
elites wake only after taking damage; after a kill they respawn in three minutes,
with new positions in Field/Desert. Timed elites do not respawn.

Hyrule Field's Bullbo uses native charges with a front Bulblin driver and a rear
Bulblin archer. All three share one boss HP budget and one kill/reward; hits on
either rider damage the mount. The Bullbo and Aeralfos retain the timed-elite HP
scaling of the enemies they replace. All Bullbo/rider attacks (including arrows)
and elite Aeralfos attacks deal two base hearts before difficulty/Endless scaling
and damage reduction. Riders and their arrows follow
the encounter's pause and cleanup rules. Mount/rider target spheres are 150% of
native radius, and the rear archer always fires triple arrows with half the
native readiness delay. Rider targets remain at their visible saddle positions.
The mounted boss anticipates the light-ring boundary, stops to turn inward, and
returns to the inner half of the circle before resuming pursuit. The check
includes Endless movement scaling; native collision and rider attacks stay active.
The elite Aeralfos flies at 250–400 units above terrain (half the native band),
has quarter-length native readiness waits, and alternates air/ground phases every ten
active seconds, descending over a dry floor and using the native takeoff.
A ready aerial attack starts without waiting for the native paired-Aeralfos AI.
On the ground, a gap closer becomes due every three active seconds, waits for an
ongoing strike to finish, and uses a bounded, collision-checked dash into melee.
It starts a native attack immediately on arrival; blocked paths abort the dash.
Nearby Link triggers a strike without another sidestep/idle cycle.

Stalhounds emerge and fight in daylight as well as at night. The giant elite
uses a lower pounce arc so its bite can reach grounded Link. Shadow Beasts do
not require simultaneous group kills. Shadow Kargaroks use independent dive attacks with their native approach, brake
and catch animations, with the scripted
capture sequence replaced by a talon hit. The elite Carrier also has one native
mounted archer; its arrows inherit the Carrier damage factor. Stallord uses his native phase-two
fire breath and recovery, adapted to ground combat without tower rails or traps.
Death Sword uses native sword strikes and both magic shots and is visible to human
Link. Twilit Bloat retains its electric charge, flight and retreat. Phantom Zant
teleports between safe map positions and uses native projectile volleys and portals
summoning Shadow Keese/Shadow Beasts. During his projectile charge, nonlethal hits
cannot stagger or trigger his native warp reaction (including bomb arrows); HP
damage and lethal hits still apply. Darkhammer spins, throws and retrieves his
ball and chain. Puppet Zelda retains her sword charge, triangle and energy-ball
attacks. King Bulblin fights on foot with his axe; Ganondorf uses his native sword
duel. Darknut sheds armor as his run HP falls and uses the native sword throw and
unarmored fighting phase. Dungeon intros, story flags and camera/QTE sequences
are excluded; attacks, effects and attack timing remain native where portable.


Boss balance: active elites/bosses move 50% faster. Promoted ordinary elites have
half attack-readiness/recovery timers and double damage; native bosses/minibosses
do not inherit those two generic factors. Explicit damage overrides are Darknut ×2;
elite Aeralfos and Bullbo/riders ×8 on normalized native power 1 (two hearts); Death Sword, Stallord, elite Carrier and Puppet Zelda ×4; Bloat and Darkhammer
×3; Ganondorf and Phantom Zant ×5. Overrides do not stack with the generic ×2.
King Bulblin can spin every six seconds at a safe chase boundary, including at
range. Normal axe swings launch one Sword Wave toward Link, even outside melee
range; active spins emit five waves every second: one aimed at Link's current position
and four randomly directed waves. Aimed waves include Link's height and fly
toward his body position at release, without homing afterward. Waves travel
up to the light ring's diameter (3,000 units), stop at walls/hits, and use the
native axe's base damage. Each five-wave salvo shares one accepted-hit budget.
Darknut repeats
his sword throw every 1.5 active seconds outside melee range in either phase.
Death Sword can enter ranged combat after four seconds of distant pursuit;
Stallord can start breath out to 6,000 units without Link's damage-protection gate.
Bloat can attack immediately while retaining its telegraph. Ganondorf charges
from distant WAIT with a shared five-second cooldown and summons five native
ghost riders every eight active seconds. Summons/riders retire with their boss;
all owned children pause and clean up with the run. Full native baselines and
current overrides: [boss attack reference](docs/features/hyrule-survivors-boss-attacks.md).

Each timed elite and the first kill of each guard slot drops a **free large chest**. Guard respawns still give XP but no repeated chest, including after relocation. The four timed elites also drop an
**large gold-yellow EXP crystal** nearby: walk over it to pull all currently lying XP crystals, rupees and hearts
to Link from anywhere on the map. It is a one-time pickup, not an inventory item. Pots are scattered according to floor
area and safe placement, drop rupees of different values, rare heart pieces or small keys,
and respawn at a different safe position 20 seconds after breaking (menus pause the timer). Ordinary enemies can drop healing hearts and
rupees; every level-up fully heals Link, including newly chosen heart containers. Dropped hearts, rupees and small keys remain until collected
or the run ends; their native despawn timer is disabled. Ordinary enemies also
have a 2% bonus chance to drop a gold EXP magnet, increased by Luck up to 3%.
This follows the same Horde-difficulty reward normalization as other ordinary
drops. Magnets pull only pickups already present when collected; later drops
require another magnet or normal pickup range.

An active elite or boss gets a red HP bar below the EXP bar, using the native
oxygen/oil artwork and the native boss-title font (also used for Hero's Shade).
Timed bosses take priority over awakened chest guards. With several candidates,
the nearest is selected and retained until defeated or replaced by a timed boss;
sleeping guards and ordinary enemies never occupy this bar.

Normal enemies have a fixed ceiling of **345**, from the start at every Horde
difficulty. Spawn intervals still decrease from 2.5 seconds to 0.5 seconds over
the first 10 minutes; Horde difficulty still controls batch size (1–5).
Random **HORDE!** events triple spawn batches for 15 seconds, within
that population limit. They average two events per five minutes, without a quota;
overlaps add 15 seconds instead of stacking the multiplier. The HUD shows the
remaining duration. Events stop at 20:00 and pause with the run.

Up to **64 EXP crystals** can lie on the ground. Further XP merges into the nearest
crystal without loss; merged crystals turn **red**, ordinary crystals stay turquoise.
The large gold-yellow elite crystal collects existing XP globally, without giving inventory items.

## Chests and equipment

Up to **six replenishing small chests** can be present. When their slots are full,
a successful new spawn replaces the oldest unopened, unselected chest; failed
placement keeps the existing chest. Large elite reward chests use the same rule
in their separate pool. Starting 40-rupee map chests are never replaced.
The replenishing chests cost **20, 25, 32, 41, 52… rupees**: increases of +5, +7, +9, +11…
Each arena also starts with randomly placed **40-rupee small chests** (counts above),
which never respawn or affect the replenishing chests' price. Starting chests
are spread across safe, dry map floor, including sites around terrain obstacles;
their minimum spacing scales with map area. Both kinds show their own price
floating overhead only within Link's shooting range.
They are interactable only when affordable. Opening takes priority over automatic
attacks and offers three items; choose one to receive it through the native Get
Item animation. Cancelling preserves offers. Opened chests disappear; free elite
chests never raise the paid-chest price.

Pots have the same chance to drop a **small key** as a heart piece: **2% base,
up to 3% with Luck**. A key automatically pays for the next paid chest, including
a starting 40-rupee chest, without spending rupees. Exactly one key is consumed
when the reward finishes; cancelling or a failed reward consumes nothing.
Free elite chests and the free starting item never consume keys. Regular chest
prices still advance after a key purchase. Keys reset with each new run and
remain available in Endless, including after 40:00.
The Next Chest HUD uses the native rupee icon for the rupee price, or shows
**1 key** and your key count when a key is available. Nearby paid chest labels
also show **1 key** in that case.

Once no eligible item reward remains, all existing chests despawn (including
fixed map chests and free elite chests) and no more chests spawn for that run,
including its Endless continuation. A live item-get finishes before cleanup.
Chests never offer ordinary level-up rewards. This state resets with a new run.

Equip up to **four chest items**; their icons (or empty slots) appear above the
choices. Red Potion uses one shared slot for all bottles, retained even when
they are empty. Passive power-ups have no
slot limit. Dual Wield and Shield can coexist, each using its own equipment slot.

Each run shares **one free Reroll and one Banish** across both selection menus.
Reroll refreshes all offers; further uses cost **50, 100, 150… rupees**. Paid
chests reserve their purchase price unless a key covers it; rerolls still cost rupees. Banish removes the named choice from future
offers without removing its current ranks or equipment; weapon bans apply to
both menus. The top-left **Reroll** button shows **Free** or its rupee cost; the
icons beside your equipment show remaining free rerolls and banishes. Use the red
banish symbol at the top right of a card to remove that choice. Talents can add two more free uses of each. All counters and bans reset on a new run.

| Chest reward | Main effect |
| --- | --- |
| Hero's Bow | Automatic aimed arrows; one arrow at ranks 1–9, three at ranks 10–19, five at ranks 20–30. The symmetric fan spans 22.5° with three arrows and 45° with five (±22.5°). Additional hits on the same enemy within one volley deal 25% damage; each new volley starts at full damage. Unlocks Fire Arrows level-ups: burning damage over time, refreshed on hit. |
| Boomerang | Gale throws that hop between enemies; one extra hop every three ranks (4, 7, 10…), max 10 hops / 11 targets. |
| Bombs | Automatic throws and area explosions; upgrades improve damage, radius and frequency. |
| Ball and Chain | Pierces outward and back, damaging and staggering enemies on both legs. |
| Master Sword | More sword damage; rank 5 becomes the Light Sword. Unlocks Jump Strike level-ups. After unlocking it from a chest, further ranks are also available as level-up rewards; each two sword levels add 1 Lightning Strike damage (whole completed pairs). |
| Ordon Sword | Dual Wield: 2/3/4/5 total targets at ranks 1/10/20/30; +2.5% melee speed per rank (whole-percent rounding), capped at rank 30. Also grants +1 percentage point melee crit chance per rank. Can coexist with a shield. |
| Shield | 5% block chance per rank, max 50%; evolves to Ordon at 20% and Hylian at 35%. Unlocks Shield Attack level-ups. |
| Dark Link | Starts at 1.3× damage for 5 seconds every 20 seconds. Odd ranks increase damage, even ranks increase duration; rank 30 is permanent 2× damage. |
| Red Potion | One equipment slot; adds filled bottles up to four, then refills empty ones. Rank 0 heals 1 heart. Potion Healing level-ups add ¼ heart per rank, up to 8½ hearts at rank 30. Use manually. |

**Jump Strike:** 20% chance at an eligible normal melee attack, with a 6-second
cooldown reduced by Haste. Requires a nearby enemy, but jumps in place without
a target-directed lunge. Normal stick movement remains available through takeoff,
airborne attack and landing; no stick input means no horizontal travel. Uses the
native quick jump attack and the original bomb shockwave on landing. Landing splash deals 30% of melee damage at rank 1,
+5 percentage points per rank (175% at rank 30), rounded to native whole damage.
Direct sword contact deals normal melee damage; critical hits and Dark Link apply.
**Reach** adds 15% per rank to normal melee reach (on top of its 130% base),
Dual Wield splash targeting, Jump Strike radius and Lightning Strike hop range.
Other linear ranges retain 7.5% per rank. Spin doubles the previous radius bonus:
`base × (1 + 2 × (sqrt(1 + 0.075 × Reach) - 1))`.
Bomb and Shield retain `base × sqrt(1 + 0.075 × Reach)`.
Spin and Jump visuals follow their actual hitboxes.
Automatic item attacks, Sword Waves, Spin and Shield Attack have independent
cooldowns and overlap, including during Jump Strike. Distinct attacks damage
immediately without a shared enemy iFrame gate. A persistent hitbox still hits
each enemy only once per activation (Ball: once on each leg).

Weapons, ordinary power-ups, Master Sword, Ordon Sword and Dark Link reach rank 30.
Luck caps at 20 and shield block at 10. Haste uses approximately
`1 / (1 + 0.025 × rank)` cooldown scaling, with integer rounding.
Item intervals start from 75/85/110/100 ticks for Bow/Boomerang/Bombs/Ball,
divided by `1 + 0.025 × (item rank − 1)`, then Haste. Their minimum intervals are
20/30/45/40 ticks respectively; gameplay runs at 30 ticks per second.
Waves and Spin use base intervals of 60 and 120 ticks. Shield Attack uses
`90 / (1 + 0.03 × rank)` ticks before Haste, with a 30-tick minimum.
Bomb radius is `(170 + 12 × Bomb rank) × sqrt(Reach factor)`;
Shield Attack radius is `(60 + 3 × rank) × sqrt(Reach factor)`.
Unlocked chest weapons can also be upgraded at level-up. Ordinary hits do not
stagger enemies; Shield Attack and Ball and Chain do. Knocked-down enemies remain
damageable. Ball lifetime grows with Reach so its return leg is preserved.

### Enemy health curves

Normal HP multiply the species' base HP by the following fixed map/time factors.
Interpolate linearly; start at 1× at minute zero and stop growth at minute 20.
These curves do not react to the player's equipment or luck. HP is sampled on
spawn; untouched dormant guards refresh until engaged. Ordinary elite HP is
`max(120, 32 × factor)`. Final bosses instead use the separate absolute budgets
below. The selected Enemy HP difficulty multiplier applies once to every type;
round once and cap at native signed-HP maximum 32,767.

| Map | 1 min | 5 min | 10 min | 15 min | 20 min | Final boss HP |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Darknut Arena | 1.00 | 4.09 | 10.76 | 19.22 | 28.30 | 2,300 |
| Hyrule Field | 1.08 | 8.76 | 25.39 | 49.66 | 67.44 | 5,400 |
| Gerudo Desert | 1.04 | 8.39 | 23.26 | 44.30 | 59.43 | 4,800 |
| Palace of Twilight | 1.00 | 5.70 | 14.62 | 24.38 | 34.54 | 2,800 |
| Hyrule Castle Throne Room | 1.10 | 4.98 | 12.79 | 22.41 | 32.73 | 2,600 |

These are balance starting points from the progression model; actual boss flight
windows, attack geometry and survival pressure still need in-game validation.
No optional Legendary global-damage bonus is included.

## Critical hits

Two passive level-up rewards, both capped at **rank 30**:

| Power-up | Start | Per rank | At rank 30 |
| --- | --- | --- | --- |
| Crit Chance | 0% | +0.75 percentage points base chance | 22.5% base; 33.75% at Luck rank 20 |
| Crit Damage | ×2 damage | +0.05 multiplier | ×3.5 damage |

Talents add up to five base percentage points; combined final chance caps at 100%.
Base final chance is `base chance × (1 + 0.025 × Luck rank)`, with integer basis-point rounding.
Dual Wield adds another absolute percentage point per rank (up to 30) for Sword,
Spin, Shield Attack, Jump Strike, Dual Wield splash and Lightning Strike, capped
at 100% total. Sword Wave and other ranged attacks retain their existing crit chance.
Each direct sword, spin, projectile, shield or Dual Wield hit rolls independently.
Burn ticks cannot crit. Critical damage rounds to native integer health before
Dark Link doubles it. Both power-ups support rarities, rerolls and banishment;
the menu shows the effective chance including current Luck.

## Additional run power-ups

| Power-up | Cap | Rarity / effect |
| --- | ---: | --- |
| Knockback | 10 | Any rarity. +40 units of horizontal push per rank on direct attacks, without interrupting enemy actions. Ball and Shield Attack have a fixed 240-unit base independent of their weapon ranks, Power and Reach. |
| Piercing | 10 | Requires Bow. Each rank lets every arrow damage one additional distinct enemy (up to 11 total). Includes every Multishot arrow. |
| Multishot | 3 | Epic grants 1 rank; Legendary grants 2. Each rank adds one projectile/chain to Bow, Boomerang, Bombs, Ball and Chain, Sword Wave and Lightning Strike. |
| Multi Strike | 3 | Requires Dual Wield. Epic grants 1 rank; Legendary grants 2. Each rank adds one sequential activation of Sword Attack (including its splash), Spin Attack, Sword Wave and Lightning Strike. |
| Lightning Strike | 30 | 20% proc on an accepted melee hit, +2 percentage points per Luck rank (60% at rank 20), gated by its own cooldown. See below. |
| Life Regen | 5 | Rare or better. Each rank removes 1 second from passive quarter-heart regeneration; stacks with the talent (20 → 15 seconds, or 10 seconds with both maxed). |
| Life Steal | 1 | Legendary only. Only kills by Sword Attack, Dual Wield splash, Spin, Sword Wave, Jump Strike or Lightning Strike restore a quarter heart, once per death. Includes Multishot and Multi Strike; burns and other abilities do not heal. |

Knockback runs over six simulation ticks, with twelve ticks between pushes.
Simultaneous hits take the strongest push instead of adding them. Elites and
bosses are immune; the maximum heavy push is 640 units.
Walls, arena barriers and unsafe ledges stop displacement. Native heavy launches
on living ordinary enemies are suppressed in favor of this bounded movement,
including Bokoblins with one HP remaining. Attacks, animation timers and native
velocity continue; pushes preserve airborne height instead of snapping to the floor.
Burn ticks do not apply a second push. Damage still has no shared iFrame gate.

Multishot chooses different visible targets in range for Boomerang, Bombs and
Ball before reusing the nearest target when fewer enemies exist than shots.
Boomerangs retain their assigned first targets. Arrows distribute their extra
shots within the Bow rank's existing 0°/22.5°/45° fan; at rank 20+ and Multishot 3,
eight arrows still cover only 45°. Same-volley extra hits on one target retain
25% damage, also when piercing. Waves fan out symmetrically, up to four waves
across 36°, with each wave retaining its normal/Great Spin tier and hit deduplication.

Lightning Strike uses the Boomerang damage/cooldown curves at its own level;
no Boomerang equipment is required. Power, Haste, critical upgrades, Dark Link,
Knockback and Multishot apply. Its hop count is twice Boomerang's (2 at level 1,
20 at level 30), and propagation travels at 210 units/tick versus 42.
Reach gives double range growth; Master Sword adds `floor(sword level / 2)` damage.
Multishot adds one complete chain per rank (1/2/3/4 chains). Each cast distributes
initial targets and subsequent hops across reachable enemies before reusing targets;
when reuse is needed, prefer the least-targeted enemy, then the nearest. Multi Strike
repeats start a fresh target distribution. Each chain remembers its own targets,
respects walls and boss blocks, and cannot
trigger further chains. Native electric sparks and a jagged cyan/white arc show
its path. Lightning appears as its own damage row in Results.

Multi Strike grants 2/3/4 total activations at levels 1/2/3. Sword and Spin repeats
wait for their native hit window/animation; Sword Wave volleys and Lightning casts
repeat four simulation ticks apart. Repeats do not recursively create more repeats.
Each repeat gets independent hit tracking, and each wave/lightning cast retains
its Multishot count. Run teardown and level-up interruption clear pending casts.

Rarity is rolled before selecting an eligible upgrade: restricted power-ups do
not promote common rolls to rare ones. Life Regen follows normal rarity units
(Rare +3 ranks, Epic +4, Legendary +5); Multishot and Life Steal are the explicit
unit exceptions, together with the one-rank Legendary powers below. Cap overflow pays 20 rupees per unused unit. All reset per run.

## Legendary weapon powers

All seven powers below have **one rank**, reset each run, and appear only on
Legendary level-up cards. They require the corresponding weapon/family; Fire
Edge works with the starting sword. A collected power no longer appears.

| Power | Requirement | Effect |
| --- | --- | --- |
| Hurricane | Boomerang | Doubles the boomerang, wind visuals and hit radius. Incidental contacts do not interrupt its flight toward a chosen target; every crossed enemy takes damage once per throw. |
| Fire Bomb | Bombs | Every explosion victim burns for 10% of normal bomb damage per second for five seconds, with native Fire Arrow burn particles. |
| Bomb Arrow | Bow | Native bomb-arrow model. Each arrow explodes once at its first enemy contact for an additional 50% bow damage in a rank-5 bomb radius (230 before Reach). Piercing continues; each Multishot arrow owns its own explosion. |
| Fire Edge | Starting melee | Burning sword blade. Normal hits, Spin, Shield Attack, Dual Wield secondary hits and Jump Strike ignite enemies for 10% normal melee damage per second for five seconds. |
| Dark Aura | Dark Link | While transformed, ignite visible enemies within a base radius of 300 (scaled by Reach), with black flames. Each second deals 10% of their maximum run HP, including bosses. Burns continue for five seconds after leaving range or losing the form. |
| Shockwave | Shield + Shield Attack | Each bash also releases a straight piercing wave for current melee damage. Its radius is 95 (rank-5 Ball and Chain), scaled by Reach; travels 1,350 units and stops at walls. |
| Chain Spin | Ball and Chain | Each launched ball retracts in a shrinking 360-degree spiral around Link over two active seconds. Each enemy can be hit once outbound and once during the spiraling return. Walls stop the projectile. |

Different burn sources coexist. Reapplying the same source refreshes duration
without stacking it or postponing ticks. Fire Bomb/Fire Edge snapshot normal
weapon damage including active Dark Link, without a new critical roll. Dark
Aura's max-HP damage receives no further damage multiplier. Fractional tenths
carry between ticks, avoiding a minimum-one-damage inflation for weak weapons.
Fire Arrows retain their existing independent burn progression. Bomb Arrow's
explosion belongs to the Bow and does not trigger Fire Bomb.

## Rarities and Luck

Each offered card rolls independently, including **free elite chests**. Rarity
controls ranks/units of the chosen reward: **Common ×1, Uncommon ×2, Rare ×3,
Epic ×4, Legendary ×5**, except Multishot (Epic ×1 / Legendary ×2) and all one-rank Legendary powers (Legendary ×1). Names and icon borders show the rarity. Units exceeding
a reward's cap become 20 rupees each, subject to wallet capacity.

Luck has **20 ranks** and resets each run. Rarity weighting gains +5% per rank;
crit chance gains +2.5% per rank. Higher-rarity Luck cards grant multiple ranks.
With `k = 1 + 0.05 × Luck rank`, rarity weights are **60 / 25k / 10k² / 4k³ / k⁴**,
normalized by their sum. There is no player-level or chest-source bonus.

| Luck rank | Common | Uncommon | Rare | Epic | Legendary |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 60.00% | 25.00% | 10.00% | 4.00% | 1.00% |
| 1 | 58.18% | 25.46% | 10.69% | 4.49% | 1.18% |
| 3 | 54.64% | 26.18% | 12.04% | 5.54% | 1.59% |
| 5 | 51.23% | 26.68% | 13.34% | 6.67% | 2.08% |
| 10 | 43.30% | 27.06% | 16.24% | 9.74% | 3.65% |
| 15 | 36.32% | 26.48% | 18.54% | 12.98% | 5.68% |
| 20 | 30.30% | 25.25% | 20.20% | 16.16% | 8.08% |

Rounded chances **per card**, not per three-card selection. At rank 20, a
selection has about **22.34%** chance to contain at least one Legendary card.
Luck never rerolls existing offers.

Heart, heart-piece and rupee bonuses use a separate multiplier capped at **+50%
Luck** (+10% per rank, capped at rank 5): enemy drops start at 2.5% heart / 45% rupee / 52.5% nothing and reach
3.75% heart / 67.5% rupee / 28.75% nothing; pot heart pieces
reach 3%. Better rupee-color chances also stop improving above +50%.

[Technical details and testing notes](docs/features/hyrule-survivors.md)

Player projectiles have no fixed simultaneous-count cap. Full bow salvos,
boomerangs, bombs, ball-and-chain throws and sword waves allocate additional
reusable slots as needed. Cooldowns, lifetimes and damage rules are unchanged.
This increases possible memory/rendering load and needs in-game stress testing.

Sword Wave fires one projectile before Multishot bonuses. At rank 15 it becomes a Great Spin
projectile (base radius 65 → 100). Spin Attack becomes **Great Spin Attack**
at rank 15 (base radius 260 → 400), including its displayed name. Both use the
six native Great Spin effects, scaled to the hitbox with Reach.

Field/Desert guards are distributed away from the original entrance and Link,
on open land validated locally rather than along a straight path from Link.
Sword Waves again fire from a visible normal sword swing, including at ranged
distance; other automatic skills continue during its wind-up.


## Development Debug tab

The development build adds **Debug** after Settings in the start menu. Changes
apply to the next run and are not written to save files.

| Setting | Values | Default |
| --- | --- | --- |
| Minimum Rarity | Common / Uncommon / Rare / Epic / Legendary | Common |
| Player damage | x1–x10 outgoing damage, including burns | x1 |
| Infinite rerolls | Off / On; free, without consuming charges | Off |
| Infinite banishes | Off / On; no charge consumption | Off |
| Run timer | 20 / 10 / 5 minutes | 20 minutes |
| Max Equipment Drop Rate | Off / On; equipment from every boss | Off |
| Always Legendary Equipment | Off / On; new equipment has Legendary rarity and maximum base stats | Off |

Minimum Rarity raises lower rarity rolls to the floor for both level-ups and
all treasure offers, including the starting item. Higher rolls remain higher.
Infinite rerolls preserve chest purchase costs. Banished choices remain excluded
for the rest of the run. The damage multiplier applies once after ordinary,
critical, Dark Link and fixed burn damage calculations, with a safe native HP cap.

| Run timer | Boss 1 | Boss 2 | Boss 3 | Final boss |
| --- | --- | --- | --- | --- |
| 20 min | 5:00 | 10:00 | 15:00 | 20:00 |
| 10 min | 2:30 | 5:00 | 7:30 | 10:00 |
| 5 min | 1:15 | 2:30 | 3:45 | 5:00 |

At the selected limit, normal spawns/hordes stop; victory still requires all four
bosses defeated. XP, enemy HP curves, cooldowns and regeneration continue to use
actual elapsed time. The HUD displays the chosen timer.

**Before merging PR #140:** set `DAWNLIGHT_SURVIVORS_DEBUG` to `0` in
`src/survivors_debug.hpp`. This is the single enable/disable switch. It removes
the tab and forces every debug setting back to the defaults above, including
stale run/menu values. The switch is currently `1` for development testing.


## Endless Mode and Twilit Rank

After clearing a map, the Victory results offer **Continue in Endless Mode**.
The normal victory record is kept. Continue with the same weapons, upgrades,
inventory, level and run clock. The run ends on defeat or End Run, not at a time
limit. Each complete minute after continuing compounds enemy HP, incoming damage
and movement speed by **x1.10** (two minutes: x1.21). New spawns also continue the
map HP curve beyond 20 minutes using its last segment; existing wounded enemies
receive each minute bonus once, preserving their remaining-health fraction.
Native HP/damage representation remains limited to 32,767; extreme movement
multipliers are bounded to prevent invalid physics values.

The map's four timed bosses repeat in order every five minutes (also when the
normal run used a shortened debug timer). An occupied boss slot waits for its
previous boss to die. Repeated bosses give fresh reward chests/EXP magnets; normal
spawns, Horde rolls, pots and eligible map guards continue.

Each map shows **Endless Score** beside both its Ranked and Cheated normal score.
Click it for the saved run's damage breakdown, Twilit Rank and Delete Score action.
Endless score includes the normal segment and its victory bonus. Talent XP paid
at the normal victory is subtracted from the later award; one continued run still
counts as one run/win. Reset High Scores clears all four categories without
locking maps. Existing v8/v9 normal records migrate with no score changes.

**Twilit Rank** is Enemy HP + Boss HP + Enemy Damage + Enemy Speed + Horde
(5–25). Five x1 settings give Rank 5 and x1 rewards; five x5 settings give Rank
25 and x5 score/meta XP. Each half step adds 0.5 rank. Horde still divides
normal-kill points and normal-enemy run-XP/loot opportunities; elite run XP
stays x1. Results and historical score snapshots retain their original rank
and reward formula.

Equipment drop chance rises linearly from the talent-adjusted baseline (20–70%)
at Rank 5 to **100% at Rank 25**, for timed and stationary bosses. Rarity also
improves: `(Rank - 5) / 20` of equipment rolls use **75% Epic / 25% Legendary**
instead of their normal stat rolls. At Rank 25 nothing below Epic drops.
Equipment Rarity still improves the stat distribution within those rarities.
The Weapon/Tunic split stays 80%/20%; existing items are unchanged.


### Session-only Debug gate

When the development Debug tab is compiled in, its first control is **Enable Debug**
(default Off). A notice appears on either transition. The switch is kept only in
memory, never in config/export/save data, and returns to Off after a game restart.
All other Debug controls are disabled until it is On. Infinite rerolls and infinite
banishes use ordinary toggles. Turning Debug Off resets the debug options.

A run captures Debug at startup even when every individual option is at its default.
Debug runs do not update any normal/Endless Ranked/Cheated record, award permanent
Survivors XP, unlock maps, or update lifetime record statistics. This remains true
through Endless continuation and regardless of the current session switch. Results
show a Debug notice and zero XP earned. In-run XP, level-ups, rewards and combat
still work for testing. New runs started with Debug Off progress normally. The
compile-time release switch still hides the tab and neutralizes all Debug features.

### Endless escalation at 40:00

In Endless Mode, every normal and shadow archer volley contains three native
arrows using Hard Mode's horizontal fan (center and +/-0x900 yaw).
At **40:00 total run time**, heart and heart-piece drops stop and remaining
healing pickups retire. Life Steal stops healing and no longer appears as a new
reward. Passive Life Regen (including talents and upgrade rewards), level-up full
healing and owned Red Potion bottles remain available. Potion Healing ranks
still apply, and difficulty/Endless HP, damage and movement scaling continue.


### Survivors HUD editor

Open **Settings → HUD-Editor** from either Survivors menu or Dawnlight's
Survivors tab. The transparent window previews the HUD while you adjust each
of these ten elements independently: Timer, Twilit Rank, EXP-Bar, Info-Texts,
Level-Text, Score text, EXP-text, Boss-Name, Boss-Bar and Item Banner. Choose an element,
then change X Position, Y Position and Scale; Reset selected element restores
its defaults. X is relative to the screen center, Y to the top, in native HUD
units. Info-Texts share a position and continue to leave space for a visible boss.
The preview always includes sample equipment, a boss and a nonzero rank.
Item Banner uses its center as the X/Y anchor; its background, icon, name, stats
and Midna prompt scale together (10–400%). The preview stays visible without a
drop and does not consume queued rewards or their eight-second timer.

Changes apply immediately, persist automatically, and participate in Dawnlight's
global settings export/import. They do not affect difficulty, scores or run
integrity. Boss bar ends retain their proportions: only the center is extended.
All Survivors overlays, including chest prices, Crit/Block/Dodge and hub labels,
are hidden during native Collection, map/ring menus and pause transitions.


## Persistent boss equipment

Every scheduled boss and stationary guard boss, including repeated Endless bosses and respawned guards, has a base 20% chance to award permanent equipment directly to the inventory. A successful drop is 80% Weapon / 20% Tunic. Weapons have five equally likely families (Sword, Bow, Boomerang, Bombs, Ball and Chain), each with two equally likely variants: each named weapon is 8% of equipment drops. Luck does not change these odds. Equipment Drop Rate adds 5 percentage points per rank (maximum 10), reaching 70% at Twilit Rank 5. Difficulty scales this toward guaranteed equipment at Rank 25, with 75% Epic and 25% Legendary. Ordinary enemies and the victory cleanup wave do not award equipment. Debug runs award none unless **Max Equipment Drop Rate** or **Always Legendary Equipment** is enabled before the run. This session-only toggle defaults to Off and guarantees an equipment award per timed or stationary boss, with normal type rolls and the run's Equipment Rarity talent applied to stat rolls. **Always Legendary Equipment** also defaults to Off and applies from the next run: all new drops have maximum base stats (tier 5 or +2 for count stats), guaranteeing Legendary rarity. Weapons receive one random bonus from their full Legendary pool; Tunics have no perk. Used alone it preserves the normal talent-adjusted drop chance; combined with Max Equipment Drop Rate, every boss awards Legendary equipment. Existing items are unchanged. These items are saved; Debug score, permanent XP and map-unlock exclusions still apply. A successful award displays a small rarity-colored banner, initially centered vertically near the right screen edge; its position and size can be changed in the HUD-Editor. It slides in from the right toward the left, shows the equipment icon on the left and its name/stats on the right, and slides out after eight active gameplay seconds (including the short entrance/exit animation). Below an acquired-item banner, **Call Midna to Equip** offers immediate equipment replacement via the configured Midna input (including touch). This equips exactly the displayed drop in its Weapon/Tunic slot, preserving the replaced item in inventory. On successful storage the banner disappears immediately and the next queued drop appears. A failed equip save leaves the banner available to retry. Simultaneous awards queue in order until the current banner expires or is equipped. Menus pause the timer and hide the banner. Inventory-full and save-failure notices use the same banner without a misleading acquired item. A full inventory displays only **Inventory Full!** and never replaces an existing item.

Open the large equipment chest in the Darknut Arena hub with A. It is an independent hub actor at the side wall on the +X side of the Master Sword pedestal, with its front facing the center of the room. It does not depend on story chest flags, native appearance events, Link's draw or run-loot resources. The inventory contains **40 shared slots across two tabs**, **Equipment 1 (20 slots)** and **Equipment 2 (20 slots)**. Each page has four rows of five. New items occupy the first empty slot in page order, from top left to bottom right, filling page one before page two. Controller/keyboard navigation and touch/mouse activation use native UI controls. Confirm equips **one Weapon and one Tunic**, replacing the previous item in that slot. Both equipped items remain in the inventory and are marked **Equipped**, with one Weapon and one Tunic shared across both pages. **Tap/click an equipped item again, or select it again with A/Confirm, to unequip it without deleting it.** Toggle Banish, then select an item to open **Delete Equipment?**, showing the exact item and stats. **Cancel** and B/Escape keep it; only **Delete** permanently removes it and unequips it if necessary. A failed save keeps the confirmation open for retry. Closing the inventory, switching pages or leaving the editable hub clears pending confirmations; delayed or stale confirmations cannot delete a changed slot. Closing the inventory saves progress through the existing native background-save flow. A failed storage write leaves the previous inventory intact.

The toolbar at the top of each inventory page provides **Sort by Rarity**, **Sort by Type** and **Move Items**, using the same controller/keyboard, mouse and touch controls as the item cards. Sorting covers all 40 slots, places empty slots last and saves the order. Rarity sorts highest first, then by family/variant; Type groups Sword, Bow, Boomerang, Bombs, Ball and Chain, then Tunic, with variants together and highest rarity first within each variant. Equal items retain their order. To move, enable **Move Items**, select an item, then choose any destination, including on the other page. An occupied destination swaps both items. Selecting the source again cancels that selection; selecting **Move Items** again exits move mode. Sorting, Banish or closing the inventory also clears move mode. Equipped items remain equipped after either operation, including identical duplicates. Failed saves preserve the original order and allow retry.

**Equipment Rarity** improves each independent stat roll, linearly per rank. These are **stat-tier probabilities**, not final item-rarity probabilities:

| Stat tier | Talent rank 0 | Talent rank 5 | Talent rank 10 |
| --- | ---: | ---: | ---: |
| 1 | 50% | 35% | 20% |
| 2 | 25% | 25% | 25% |
| 3 | 15% | 20% | 25% |
| 4 | 8% | 14% | 20% |
| 5 | 2% | 6% | 10% |

Piercing and hop-count stats remain within +1–2: the chance of +2 rises by 2 percentage points per talent rank, from 10% to 30%. Gear type/family probabilities and maximum stats are unchanged. Item rarity continues to follow its rolled stats. These equipment talents are independent of Luck and Legendary Chance, use the talent snapshot taken at run start (also in Endless), and never reroll existing inventory items. Each rank costs one Unlock Point; all talents now cost 119 points in total. Records v13 preserves existing progress and initializes both new talents at rank 0 when loading older saves.

New **Epic weapons receive one random +1 upgrade level** from their family pool. New **Legendary weapons receive one random +2-level bonus or one Legendary effect**. Swords have four Epic choices (25% each) and five Legendary choices (20% each). Other families have three equally likely Epic choices and four Legendary choices (25% each). Both variants of a family share this pool. Tunics and Common/Uncommon/Rare weapons have no additional perk.

| Family | Ordinary bonus pool | Legendary effect option |
| --- | --- | --- |
| Sword | Sword Wave, Spin Attack, Multi Strike, Lightning Strike | Fire Edge |
| Bow | Fire Arrows, Piercing, Multishot | Bomb Arrow |
| Boomerang | Multishot, Reach, Haste | Hurricane |
| Bombs | Multishot, Reach, Haste | Fire Bomb |
| Ball and Chain | Multishot, Reach, Haste | Chain Spin |

The bonus is rolled once, saved with the item, and shown in its banner, inventory details/tooltips and Collection description. It does not alter the item's rarity or base stats. Effective bonus levels are separate from earned run ranks; switching equipment removes only the equipment contribution. Ordinary levels can still be earned while a bonus is equipped. Existing upgrade caps remain in effect: Multishot/Multi Strike 3, Piercing 10 and other ordinary upgrades 30. The original numeric equipment affixes remain separate and retain their existing beyond-cap behavior. Multishot, Reach and Haste equipment bonuses apply only to the matching owned weapon; they never affect other weapons or grant a chest weapon. Multi Strike still requires Dual Wield. Sword Wave, Spin and Lightning Strike bonus levels can activate those sword abilities before they are selected at level-up. Legendary effects are Boolean and never double-stack; an already effective Legendary power is excluded from level-up offers until it is no longer active.

Chain Spin is also a one-rank Legendary level-up after acquiring Ball and Chain. Each launched ball snapshots whether it has Chain Spin. It reaches its normal throw distance, then retracts while making one full turn around Link with a continuously shrinking radius. The existing ball model, chain and damaging collider follow the orbit together; movement, pause and wall checks still apply. Switching equipment affects subsequent launches; a ball already in flight finishes its captured behavior. Learned Chain Spin remains active after equipment changes. Records v13 appends its earned rank for each high-score snapshot without moving older score or talent fields.

Equipment details and tooltips show the English name, rarity, rolled stats and bonus. Variants share their native weapon-family icon and do not replace Link's model or story equipment.

| Weapon | Stat 1 | Stat 2 | Stat 3 |
| --- | --- | --- | --- |
| Hero's Sword | +1–5 sword damage | +1–5 percentage points melee critical chance | +1–5% melee attack speed |
| Greatsword | +1–5 sword damage | +2–10% melee reach | +5–25% sword-family knockback |
| Hero's Bow | +1–5 bow damage | +1–5 percentage points bow critical chance | +1–5% bow attack speed |
| Sharpshooter Bow | +1–5 bow damage | +5–25% arrow range | +1–2 additional pierced targets |
| Storm Boomerang | +1–5 boomerang damage | +1–5% boomerang attack speed | +5–25% hop range |
| Hurricane Boomerang | +1–5 boomerang damage | +1–2 additional target hops | +5–25% outbound and return flight speed |
| Bomb Bag | +1–5 bomb damage | +2–10% explosion radius | +1–5% bomb throw rate |
| Siege Bombs | +1–5 bomb damage | +2–10% explosion radius | +5–25% bomb knockback |
| Iron Ball | +1–5 ball and chain damage | +1–5% attack speed | +5–25% range |
| Breaker Ball | +1–5 ball and chain damage | +5–25% knockback | +5–25% outbound and return flight speed |

Tunics retain **+1–5 maximum hearts** and **+1–5 percentage points dodge chance**. The combined maximum remains **20 hearts**. No additional Tunic variants are introduced in this patch.

Weapon and Tunic stats add to existing talent, level-up and item bonuses. Equipment does not consume upgrade levels or prevent earning normal upgrades. For the original numeric affixes (not the new bonus levels), except for the 20-heart maximum and probability's natural 100% bound, equipment bonuses apply beyond ordinary upgrade caps: Sharpshooter Bow permits up to **12 additional pierced targets / 13 total targets per arrow**, and Hurricane Boomerang permits **12 hops / 13 visited targets**. Equipment attack speed remains effective after the normal item cooldown floor, with intervals rounded down to simulation ticks and a minimum of one tick. Range and blast-radius multipliers apply to the already upgraded range/radius; knockback scales the matching family's existing push, preserving boss/elite immunity. A percentage knockback bonus does not create a push when the original push is zero.

Sword damage retains the existing scope: melee/dual-wield splash, Spin, Sword Wave, Jump Strike and Lightning Strike. Sword critical chance retains the existing melee family, including Shield Attack and Lightning Strike. Greatsword reach affects melee, dual-wield splash, Spin, Jump Strike and Lightning Strike. Other equipment strengthens only its corresponding run weapon, **after that weapon is acquired from a chest**. Equipping a ranged item does not grant that weapon, occupy a run item slot or disable sword attacks. The Hurricane Boomerang name alone does not grant Hurricane; the Legendary effect must be rolled as its bonus or earned at level-up. Bomb Arrow blasts retain their existing 50% arrow damage and fixed level-5 bomb radius; equipment bomb radius applies to ordinary bombs. Fire Bomb burn follows the boosted bomb damage.

Each ordinary stat independently rolls tiers **1 / 2 / 3 / 4 / 5** with **50 / 25 / 15 / 8 / 2%** probability. Multiply the tier by the table's step: e.g. range rolls +5/+10/+15/+20/+25%. Additional piercing and hop counts instead roll **+1 with 90% / +2 with 10%**. Rarity is derived from the mean normalized tiers, not raw units: Common below 2, Uncommon from 2, Rare from 3, Epic from 4, Legendary from 4.5. Count affixes normalize +1 to tier 1 and +2 to tier 5. Rarity is descriptive and does not multiply the rolled stats again.

Bonuses are captured at run start and retained in Endless, with explicit live replacement through the drop banner's Midna action. The inventory chest remains hub-only. Swapping a Tunic preserves earned Heart Containers and capacity hidden by the 20-heart cap; additional capacity does not heal, while lower capacity clamps current health. With no equip banner, Midna keeps the usual Cancel / End Run / Restart Run menu. Collection displays the equipped Weapon in the Poe Souls cell and the Tunic in the Letters cell, with the matching family icon, English name, rarity and stats. Story inventory flags and other modes are unchanged. The separate checksummed equipment blob is per save slot. Format v4 uses 212 bytes and accepts both 172-byte v3 and original 92-byte v1/v2 data. Older items retain their stats and have no extra perk; they are never rerolled on load. When loading v1/v2, existing items and equipped indices remain on page one, page two starts empty, and the old unequipped sentinel is translated to the new capacity. V3 retains both pages. Original Swords become Hero's Swords with identical rolls; Tunics are unchanged. High-score resets do not delete equipment, and corrupt or unsupported data remains protected from overwrites. Save records/native saving flushes boss rewards like other run progression.
