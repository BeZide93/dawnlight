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

**R Jump** enables manual jumping. It also works as wolf Link while standing, moving or dashing, using
the same jump binding and **Jump Height** setting with native wolf jump physics.

**Jump Height** in Controls sets manual jump height
from 100% (the original height and default) to 500%.

**Disable Auto Jump**, directly below **R Jump** in Controls, is off by default.
It prevents human and wolf Link's automatic running jump at ledges, requiring the manual
jump button instead. Walking off a ledge carries the actual forward speed and
direction into the fall, including sprint/dash speed, without an upward jump
impulse. Normal fall physics and ledge grabbing remain available.
Scripted/forced jumps, Midna's targeted jumps, manual jumps and Revali's Gale are preserved.
Turning **R Jump** off also resets this option to off and greys it out;
re-enabling R Jump does not automatically re-enable it.

## Glide

Optional **Glide**: press ZR in the air to glide during manual jumps or ordinary
falls. **Glide Item**, directly below the toggle, selects **Cucco** (default) or
an original textured **Glider** with a wooden frame and leather grips. Both use
native Cucco glide movement; landing puts the selected item away. The Glider
hides and silences only its internally summoned Cucco, leaving world Cuccos alone.
With **Stamina Bar** enabled, either Glide Item consumes 5 stamina per second
while gliding. Empty stamina ends the glide; exhaustion blocks redeployment
until stamina recovers to 50%. Disabling Stamina Bar removes the cost.

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

Optional **Revali's Gale**: ZR immediately performs a normal jump, including
while running or sprinting. Keep ZR held through landing to stop and crouch;
stay crouched for at least one second, then release to launch with the saved
pre-jump speed/direction and the native Gale
Boomerang tornado. **Gale Height**, below the toggle, adds 100–1000% of the
original jump height (default 500%) to **Jump Height**. For example, 200% Jump
Height plus 500% Gale Height gives 700% total height.
Releasing before landing or before the full second in crouch cancels Gale
without spending a charge. Pressing X, Y, A or B during charging also cancels
Gale and passes the press to its normal action, including native R+Y Quick
Transform and R+X Sun Song where available. This includes touch input; release
ZR before starting another charge. Time spent in the initial jump does not count.
After one second, a flattened Gale tornado loops at Link's feet with wind
audio until release. Cancelling the charge also stops this readiness cue.
Gale also enables the initial ZR jump when R Jump is off; Glide is independent.
Glide and Revali's Gale both default to disabled.

### Charges and recovery

Gale uses three charges by default. **Gale Charges** adjusts capacity (1–12),
and **Gale Recovery Time** adjusts seconds per recovered charge (1–3600,
default 120). Charges recover one at a time; another use never restarts a
pending recharge. Recovery uses elapsed real time, including menus, cutscenes
and scene transitions. The state lasts for the session, without save-file data.

### Gale Counter

**Gale Counter** toggles only the display: native Epona sprint icons beneath
the Fierce Deity bar, with full and spent charges aligned at the bars' left
anchor. **Custom Gale Counter** in
the HUD Editor adjusts X/Y offsets and scale relative to that default anchor.
Empty charges block Gale, while the initial normal ZR jump remains available.

See [HUD editing](controls-and-hud.md#hud-editing) to reposition the counter.

## Shared stamina

Bullet Time, Flurry Rush, Sprint, Glide and the Great Spin projectile share a
stamina meter, including compatibility with Lazy Tweaks stamina actions. Emptying
the meter causes exhaustion until it recovers to 50%. The Stamina Bar setting
can disable both the meter and all Dawnlight stamina costs.

For Bullet Time, Flurry Rush and the Great Spin projectile, see
[combat and aiming](combat-and-aiming.md).
