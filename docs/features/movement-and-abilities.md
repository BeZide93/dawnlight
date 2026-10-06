# Movement and abilities

[← Dawnlight overview](../../README.md)

Movement options are configured in Dawnlight's **Controls** settings.
[Dawnlight Mode and progression](general-settings.md) can override and lock
some of these settings; disabling them restores your personal choices.

## Sprint

**Sprint** uses stamina and offers adjustable speed (100–300%, default 150%). Sprint animation uses half the actual movement
speed bonus: 150% movement gives 125% playback, including native indoor slowdown.
Manual jumps during sprinting carry the earned speed bonus into horizontal
speed and distance, independently of the selected jump height. Set **Sprint Speed** beside
**Sprint** in the Controls settings.

## Wolf Sprint

**Wolf Sprint**, directly below **Sprint**, is off by default. Hold the assigned
Dash button (B in the Dawnlight layout) while moving to sustain wolf dash speed.
**Wolf Speed** sets 100–300% of native dash speed (default 100%) and is greyed
out while Wolf Sprint is off. Native slow-area dash limits still apply.
Actual earned horizontal momentum carries into wolf jumps without increasing
their height, and into ledge falls with **Disable Auto Jump** enabled.

## Manual jump and ledges

**Manual Jump** enables manual jumping. **Jump Button**, directly below it, selects
**R** (default), **L (LB)**, **R3** or **L3**. R uses the existing game binding,
including keyboard/touch mappings. L (LB) reads the left bumper (LB/L1), not LT/L2,
which retains camera alignment and targeting. R3/L3 are stick clicks. Previously saved
R2/L2 selections fall back to R; saved R3/L3 selections retain their meaning.
The selection remains editable in Dawnlight Mode and also applies to jump attacks,
Glide and Revali's Gale. Hold it and press B during a human jump
for a jump attack. Existing button actions are not globally remapped.

Manual Jump also works as wolf Link while standing, moving or dashing, using
the same jump binding and **Jump Height** setting with native wolf jump physics.
Human Link can also jump while standing or walking with a carried object, such as
a pot or bomb, using the native jump while keeping the object in his hands.
Wolf Link can jump with an object in his mouth while standing, moving or turning;
the native wolf jump preserves the carried object and uses **Jump Height**.
Picking up, throwing and putting down objects are not interrupted; normal action,
ground and cutscene restrictions still apply.

**Jump Height** in Controls sets manual jump height
from 100% (the original height and default) to 500%.

**Disable Auto Jump**, below **Jump Button** in Controls, is off by default.
It remains independently editable while Dawnlight Mode is on; the preset does not
automatically enable it.
It prevents human and wolf Link's automatic running jump at ledges, requiring the manual
jump button instead. Walking off a ledge carries the actual forward speed and
direction into the fall, including sprint/dash speed, without an upward jump
impulse. Normal fall physics and ledge grabbing remain available.
Scripted/forced jumps, Midna's targeted jumps, manual jumps and Revali's Gale are preserved.
Turning **Manual Jump** off also resets this option to off and greys it out;
re-enabling Manual Jump does not automatically re-enable it.

## Glide

Optional **Glide**: press the selected jump button in the air to glide during manual jumps or ordinary
falls. **Glide Item**, directly below the toggle, selects **Cucco** (default) or
an original textured **Glider** with a wooden frame and leather grips. Both use
native Cucco glide movement; landing puts the selected item away. The Glider
hides and silences only its internally summoned Cucco, leaving world Cuccos alone.
With **Stamina Bar** enabled, either Glide Item consumes the configured Glide cost
(default 5 stamina points per second). Empty stamina ends the glide; exhaustion blocks redeployment
until stamina reaches the configured exhaustion threshold (default 50 points). Disabling Stamina Bar removes the cost unless Twilit Essentials stamina is active.

### Glider model and textures

Dawnlight bundles the BMD from
[DawnlightCustomGlider](https://github.com/BeZide93/DawnlightCustomGlider),
including its canopy, detailed wood, and leather textures. No separate model
pack is needed. An enabled external `DawnlightGlider.bmd` pack takes priority.
To customize the three textures through Dusklight's **Texture Replacements**,
use the source PNGs and filenames in the
[glider texture guide](../../art/glider/README.md#replacing-the-texture-without-rebuilding).

For a custom glider replacement, see
[DawnlightCustomGlider](https://github.com/BeZide93/DawnlightCustomGlider)
for an example of replacing the Glider model and its embedded textures with
a separate `.dusk` overlay mod. Enable it alongside Dawnlight and restart the
game. The repository also includes editable model and texture source files.

## Revali's Gale

Optional **Revali's Gale**: the selected jump button immediately performs a normal jump, including
while running or sprinting. Keep the jump button held through landing to stop and crouch;
stay crouched for at least one second, then release to launch with the saved
pre-jump speed/direction and the native Gale
Boomerang tornado. **Gale Height**, below the toggle, adds 100–1000% of the
original jump height (default 500%) to **Jump Height**. For example, 200% Jump
Height plus 500% Gale Height gives 700% total height.
Releasing before landing or before the full second in crouch cancels Gale
without spending a charge. Pressing X, Y, A or B during charging also cancels
Gale and passes the press to its normal action, including native R+Y Quick
Transform and R+X Sun Song where available. This includes touch input; release
the jump button before starting another charge. Time spent in the initial jump does not count.
After one second, a flattened Gale tornado loops at Link's feet with wind
audio until release. Cancelling the charge also stops this readiness cue.
Manual Jump must be enabled for the initial jump and Gale charge. Turning it off also cancels
a pending Gale charge without spending a charge. This applies to the touch Jump button and
all physical bindings. Glide remains available during falls when its own setting is enabled.
Glide and Revali's Gale both default to disabled.

### Charges and recovery

Gale uses three charges by default. **Gale Charges** adjusts capacity (1–12),
and **Gale Recovery Time** adjusts seconds per recovered charge (1–3600,
default 120). Charges recover one at a time; another use never restarts a
pending recharge. Recovery uses elapsed real time, including menus, cutscenes
and scene transitions. The state lasts for the session, without save-file data.

### Gale Counter

**Gale Counter** toggles only the display: native Epona sprint icons beneath
the Dark Link bar, with full and spent charges aligned at the bars' left
anchor. **Custom Gale Counter** in
the HUD Editor adjusts X/Y offsets and scale relative to that default anchor.
Empty charges block Gale, while the initial normal manual jump remains available.

See [HUD editing](controls-and-hud.md#hud-editing) to reposition the counter.

## Shared stamina

Open **Dawnlight Settings -> Controls -> Stamina Settings**, directly below
**Stamina Bar**. The button is enabled while Dawnlight or Twilit Essentials stamina
is active. Turning Stamina Bar Off disables Dawnlight's stamina costs only when
Twilit Essentials is not supplying stamina.

**Stamina Bar Auto Fade** is also in this submenu. It hides Dawnlight's bar when
full on any HUD preset, retaining the existing saved value. HUD preset changes
do not reset it. This control is disabled when TE manages the stamina bar.

### Twilit Essentials integration

When the optional Twilit Essentials stamina service reports stamina enabled,
Dawnlight automatically uses its pool and hides its own meter. The saved Dawnlight
toggle is preserved. In Controls, its toggle is replaced with **Using the stamina
bar from Twilit Essentials.** This also takes precedence over Dawnlight Mode.

TE owns capacity, regeneration, recovery lockout, and the shared activity settings.
Their Dawnlight controls and preset-unlock buttons are replaced in place with
**controlled by Twilit Essentials stamina settings** notices. This covers Stamina
Amount, Recovery speed, Exhaust Threshold, Exhaust Recovery speed, Sprint, Wolf
Sprint, Bullet Time, Block, Guard Break, Shield Attack, Back Slice and Helm Splitter.
TE applies the native combat costs; Dawnlight does not add a second charge.

Glide, Flurry Rush, Great Spin Projectile and Midna Attack retain their Dawnlight
cost settings, paid from the TE pool. Glide and Bullet Time drain per real gameplay
second, including during slow motion; Flurry Rush pays once on activation and is
blocked when the shared pool cannot cover its cost. TE's normal attack costs still
apply during Flurry Rush. Pauses/events do not accumulate deferred drain.

For Dawnlight Sprint/Wolf Sprint and Bullet Time, TE's source toggles and cost
multipliers take priority over Dawnlight's saved sliders. The adapter uses TE's
base rates of 27 points/sec for sprint and 10.5 points/sec for Bullet Time at 100%.
TE's optional speed-based sprint drain also applies to Dawnlight sprint.
If TE's own Sprint, Wolf Sprint or Bullet Time implementation is enabled, it takes
ownership of that ability so that movement, time scaling and costs are not applied
twice. Disable the corresponding TE ability to use Dawnlight's implementation
with the shared TE stamina pool.

Disabling TE stamina or removing its service restores Dawnlight's saved settings
and meter automatically. An exhausted pool or a temporary service error never
switches an ability to a second pool. The integration targets TE's v1.3 service;
older v1.0 services use the base rates without source-setting queries.

### Dawnlight's local stamina settings

Costs and recovery rates use **stamina points**, not percentages of the current
capacity. Setting an action's cost to **0** makes it free, even during exhaustion.
The exhaustion threshold also uses fixed points: a threshold of 50 ends exhaustion
at 50 points even with a capacity of 200 or more.

| Setting | Range | Default |
| --- | --- | --- |
| Stamina Amount | 50–500 | 100 |
| Recovery speed | 1–100/sec | 5/sec |
| Exhaust Threshold | 0–100 | 50 |
| Exhaust Recovery speed | 1–100/sec | 5/sec |
| Sprint | 0–20/sec | 5/sec |
| Wolf Sprint | 0–20/sec | 5/sec |
| Glide | 0–20/sec | 5/sec |
| Bullet Time | 0–50/sec | 15/sec |
| Flurry Rush | 0–100 | 50 |
| Block | 0–100 | 10 |
| Guard Break | 0–100 | 60 |
| Great Spin Projectile | 0–100 | 40 |
| Shield Attack | 0–50 | 20 |
| Back Slice | 0–50 | 20 |
| Helm Splitter | 0–50 | 20 |
| Midna Attack | 0–100 | 50 |

**Dawnlight Mode** applies and locks these defaults without overwriting your saved
manual values. Turning it Off restores them, including after restarting the app.
Select a gray value to review the edit warning. Confirming turns Dawnlight Mode
Off and adopts its current values as your new saved manual settings before you edit;
Cancel leaves everything unchanged. See [Dawnlight Mode](general-settings.md#dawnlight-mode).

**Progression System** adds 5 maximum stamina per complete heart above the three
starting hearts. The bonus follows the loaded save's maximum hearts, including
hearts already collected; incomplete heart pieces and current damage do not affect
it. It is added after the base amount, including Dawnlight Mode's default:
3 hearts = 100, 6 hearts = 115, 20 hearts = 185. Turning Progression Off removes
the bonus. The meter displays current stamina relative to this effective maximum.

Continuous costs run per real second; menus pause their accounting. Glide adds
its cost to Bullet Time. Recovery runs when there is no continuous drain; exhausted
recovery runs until the configured threshold is reached. A threshold of 0 resumes
paid actions as soon as any stamina has regenerated. A threshold above the current
maximum capacity is treated as that capacity, so full stamina always ends exhaustion.

Block is charged once per shielded hit, including Dual Wield. Guard Break uses its
own cost **instead of** Block. These defensive reactions retain native behavior
and drain the remaining stamina if their cost exceeds it. Holding guard alone
has no cost. Paid attack skills require enough stamina and charge on successful
activation; Midna Attack charges when opening the wolf lock-on field. Shield
Attack, Back Slice, Helm Splitter and Midna Attack work with these costs even
without Lazy Tweaks. The existing Lazy Tweaks meter compatibility remains enabled.

For Bullet Time, Flurry Rush and the Great Spin projectile, see
[combat and aiming](combat-and-aiming.md).
