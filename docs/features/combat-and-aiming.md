# Combat and aiming

[← Dawnlight overview](../../README.md)

## Aiming

**Aim Movement** allows movement while aiming. **Aim Mode** offers Vanilla,
3rd Person and Cinema options, with touch and gyro support for the modded modes.
The Gale Boomerang can lock onto terrain in both custom camera modes, using its
native lock-on range and target limit. Terrain targets follow the camera reticle,
including the 3rd Person reticle offset, with Aim Movement either on or off.

See [Arrow Modes](arrow-modes.md) for Bow ammunition modes and ZR controls.

## Bullet Time

Bullet Time while aiming the Bow during a manual jump or Gale: **Off** disables
it, **Always** preserves the previous On behavior, and **BOTW** requires a ground
clearance of twice the original 100% jump height when activating. The threshold
is independent of Jump Height/Gale Height. A 100% jump from level ground cannot
trigger BOTW Bullet Time; jumping off a ledge or using Gale can. A 200% jump
reaches the threshold near its apex. Once activated, duration, stamina and
cancellation work as before. Existing On/Off configurations migrate to Always/Off.

## Flurry Rush and Great Spin

**Flurry Rush** rewards a perfectly timed evade. The optional **Great Spin
Projectile** adds a ranged attack to the Great Spin.

During Flurry Rush, each sword strike that connects shows the native impact
effect at the contact point. Damage and the enemy's hit reaction are still
deferred until the rush ends.

Bullet Time, Flurry Rush and the Great Spin projectile share the
[stamina meter](movement-and-abilities.md#shared-stamina) with movement abilities.

## Dark Link

**Dark Link** adds a charge meter, a temporary transformation and doubled sword
damage. **Dark Link Visual**, directly below its toggle, selects the appearance:

| Visual | Appearance during the transformation |
| --- | --- |
| Magic Armor | The existing Magic Armor model swap. |
| Dark (default) | The currently worn outfit/model with dark shading and red eyes. |
| Dark Magic | The Magic Armor model with dark shading and red eyes. |
| White | Current outfit: white texels become black; black and colored texels become white. Amber iris/pupils and black sclera. |
| Gold | Current outfit with warm golden shading/specular highlights and fully white glowing eyes. |

**Dark Link Activation**, below the visual selector, offers **Spin Attack**
(charged), **R+Z**, **R+A** (default), **L3**, or **R3**. Activation requires a full meter. With
either R shortcut, hold R and then press Z or A, just like native R+Y Quick
Transform. The initial R press can still trigger a manual jump; the shortcut also
works during that jump. Keep holding R and press Z or A again to end early. The
remaining charge stops draining immediately and damaging sword hits can refill
it; the next activation requires full power again. Holding the combination does
not toggle repeatedly. Holding Z/A first and then pressing R does not activate it.
Only Z/A is consumed until release; R and its manual jump remain available.
L3/R3 use a fresh left/right stick click without holding R. Click again to end
Dark Link early and preserve the remaining charge. Holding a stick click never
repeats; clicks held through menus, blocked states, or controller reconnection
must be released before they can activate. Physical and touch stick buttons work.
On Android, touch presses are captured directly so short Z taps and the HD HUD
Z mapping can also trigger the shortcut. A pending Gale charge is cancelled when the
shortcut fires, while the jump already in progress continues normally.

Entry and exit use the native flying warp particles at twice the normal speed:
the appearance effect and sound on entry, and the disappearance effect and sound
on exit. Each sound plays once when its visual transition starts. The material
continues to build from top to bottom and recede from bottom to top.

Both selectors are disabled while Dark Link is unavailable/off, including
its Progression lock. Its saved selection is retained. Changing it during a
transformation applies when gameplay resumes, waits for any pending model load,
and preserves the remaining meter. Dark, White and Gold retain the selected outfit;
the Magic variants restore it when the transformation ends. Each warp layer retains
its own appearance when changing visuals during a transformation.

Dark, Dark Magic, White and Gold affect Link's body, face, hair/hat, hands,
worn boots, equipped sword, shield and scabbard, including Dual Wield's second
sword/scabbard, held or stowed. Magic Armor alone retains native equipment colors.
These appearances use the loaded models, including compatible replacements,
and preserve native texture transparency. In White, swords, shields and scabbards
(including Dual Wield) are uniformly white; their texture colors are not inverted.

Dark/Dark Magic eyes glow fully red; Gold eyes glow fully white. White uses
orange-yellow emission for the iris and pupil and turns the sclera black.
Eye recognition uses `eyeballL`/`eyeballR` material names. Eyelids, blinking and
cutout alpha remain native; unknown eye material names use body shading.

On Link's body and eyes, White classifies the raw diffuse texture before scene lighting: near-white means
all three RGB components are at least 224/255 (192/255 on eyes to include shaded
sclera). Those texels become black; black, gray and colored texels become white,
or amber on eyes. This is a deliberate monochrome recolor, not RGB inversion.
Materials without an identifiable diffuse texture or three spare TEV stages use
uniform white/amber; those with no spare stage remain native. Gold and Dark use
specular lighting when the native batch can safely restore the borrowed light.
Other held items retain their normal appearance. Rendering changes are
scoped to the player's draw calls; shared model data and other actors are untouched.

With [Progression System](general-settings.md) enabled, Dark Link unlocks
after freeing Faron; it is available immediately in Boss Rush.

## Shielding and Dual Wield

Manual Shielding provides optional manual guard controls.
[Dual Wield](dual-wield.md) replaces the visible shield with a second sword and
uses a crossed-sword guard and Shield Attack.

For enemy and boss difficulty settings, see [Hard Mode](hard-mode.md).

