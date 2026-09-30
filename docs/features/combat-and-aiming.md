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

## Fierce Deity

**Fierce Deity** adds a charge meter, a temporary transformation and doubled sword
damage. **Fierce Deity Visual**, directly below its toggle, selects the appearance:

| Visual | Appearance during the transformation |
| --- | --- |
| Magic Armor (default) | The existing Magic Armor model swap. |
| Dark | The currently worn outfit/model with dark shading and red eyes. |
| Dark Magic | The Magic Armor model with dark shading and red eyes. |

The visual selector is disabled while Fierce Deity is unavailable/off, including
its Progression lock. Its saved selection is retained. Changing it during a
transformation applies when gameplay resumes, waits for any pending model load,
and preserves the remaining meter. Dark keeps the current model without a clothes
reload; the Magic variants restore the selected outfit when the transformation ends.

The dark appearance affects Link's body, face, hair/hat, hands and worn boots.
It uses the loaded models, including compatible model replacements, and preserves
native texture transparency. Red eyes use the native `eyeballL`/`eyeballR` material
and `eyeball` texture naming/masks. Unsupported custom eye layouts fall back to
dark shading. Materials already using all 16 TEV stages are left unchanged.
Equipment and held items retain their normal appearance. Rendering changes are
scoped to the player's draw calls; shared model data and other actors are untouched.

With [Progression System](general-settings.md) enabled, Fierce Deity unlocks
after freeing Faron; it is available immediately in Boss Rush.

## Shielding and Dual Wield

Manual Shielding provides optional manual guard controls.
[Dual Wield](dual-wield.md) replaces the visible shield with a second sword and
uses a crossed-sword guard and Shield Attack.

For enemy and boss difficulty settings, see [Hard Mode](hard-mode.md).
