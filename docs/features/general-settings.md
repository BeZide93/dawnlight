# Dawnlight Mode and progression

[← Dawnlight overview](../../README.md)

The first tab in `Mod Manager -> Dawnlight -> Open Dawnlight Settings` is **General**.
Dawnlight Mode and Progression System default to **Off**, preserving existing
configurations. **New Save Mode** is also in General; it selects how new empty
save slots are initialized.

## Dawnlight Mode

**Dawnlight Mode** applies the intended preset: Sprint at 150%, Wolf Sprint On at 100%, R Jump at 110%,
Disable Auto Jump On (human and wolf),
Flurry Rush, BOTW Bullet Time, Enemy Hard Mode, Boss Hard Mode, 300% HP Scaling,
Manual Shielding, 500% Gale Height, 60-second Gale recovery, Stamina Bar On,
Arrow Modes, Great Spin Projectile,
No Normal-Hit Invulnerability, and the Progression System.
Controlled settings display their effective values and are grayed out.
Your personal Off-mode settings remain saved separately: switching Dawnlight Mode
Off restores them, including your previous Progression System choice. This also
works after restarting Dusklight. Settings outside the preset remain editable.

## Progression System

**Progression System** can also be enabled independently. Unlocks follow the
currently loaded save's story flags:

| Milestone | Unlock |
| --- | --- |
| Start of the game | Sprint |
| Give Talo the Wooden Sword in Ordon | Glide, with Glider selected |
| Free Ordona | Revali's Gale and its counter |
| Free Faron | Fierce Deity |
| Each three full heart containers | One Gale charge: 3 hearts = 1, 6 = 2, 9 = 3, etc. |

In Boss Rush, all progression-controlled abilities are available immediately,
including Glide/Glider, Revali's Gale and Fierce Deity. Gale charge capacity still
follows the current maximum heart containers. Story flags remain unchanged.

Charge capacity uses maximum heart containers, not current health; damage and
partial heart containers do not lower or prematurely increase it. Progression
controls Sprint, Glide, Glide Item, Revali's Gale, Gale Counter, Gale Charges,
and Fierce Deity, locking these settings until progression is disabled. A short
Dusklight toast announces newly reached unlocks and charge-capacity increases
when Notifications is On.
After handing over the Wooden Sword, the end of Talo's dialogue is followed by
Link's native chest-item lifting/holding animation, showing a compact copy of
Dawnlight's Glider in his hands and a Glider acquisition message with its own
transparent 2D item icon. The icon replaces the native fallback only in this
Glider message; ordinary item dialogs keep their original icons. This is a
presentation-only reward: it does not add bombs or change inventory. Existing
saves and toggling progression on do not replay this scene.
Loading a save or enabling progression applies existing progress silently;
changing saves does not carry unlocks or spent Gale charges into the next save.
Disabling progression restores the corresponding personal settings.

## Notifications

**Notifications**, directly below Progression System, defaults to **Off** and
controls only Progression System unlock and Gale capacity notifications. Other
Dawnlight messages, including update, restart, HUD and spawner feedback, are
unaffected. It remains editable independently of Dawnlight Mode. Muting these
notifications does not pause progression or the Glider acquisition sequence;
missed progression toasts are not replayed when notifications are enabled.

## New saves and save compatibility

**New Save Mode** includes Intro Skip and Boss Rush. Dawnlight also includes
save compatibility and item integrity repairs. See [Boss Rush](boss-rush.md)
for its hub, portals and save/resume features.
