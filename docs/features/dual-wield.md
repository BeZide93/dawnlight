# Dual Wield

[← Dawnlight overview](../../README.md)

Under `Gameplay -> Combat`, turn **Dual Wield** on to add a second-sword
option to the Collection menu. Select that icon to replace Link's visible
shield with the chosen sword. Dual Wield defaults to off; enabling it alone
does not equip the second sword.

The **2nd Sword** selector directly below it offers **Wooden Sword**,
**Ordon Sword** (default), and **Master Sword**. It updates the right-hand model,
Collection icon/name and Link's menu preview. The sword type is a global setting;
the equipped/unequipped choice remains saved separately for each save slot.
Changing the type keeps the current combat/arm animation state.

With **Progression System** active (also through Dawnlight Mode), this setting
is locked: off before obtaining the Ordon Sword, on afterwards. Boss Rush
unlocks it immediately. Turning progression off restores your previous setting;
equip the second sword through its Collection icon as usual.

The icon sits to the right of the shield row, after other visible shield
options. Navigate from adjacent slots (including left from the Wooden Sword
in the HD layout), or click/tap the sword.
Its highlighted frame indicates that Dual Wield is equipped. Link’s menu
preview carries the second sword, and navigating/equipping uses the normal
menu sounds. The existing menu cursor supplies the selection animation,
including styling applied by other mods. Use the shield
slots to leave Dual Wield; those slots retain their existing equip/unequip
behavior, including behavior added by other mods. Selecting the sword requires
human Link and an equipped shield. The choice is saved per save slot, with
separate choices for normal play and Boss Rush. Turning the setting off removes
the icon and clears the current choice.

Ordon and Master Sword use their matching scabbards at the left hip when stowed.
Wooden Sword rests at the hip without a scabbard; the
primary sword keeps its normal equipment slot. The Wooden Sword, including
Twilit Essentials' additional starter-equipment slot, can also be used as the
primary weapon.

The Ordon and Master scabbards have an initial visual adjustment of **1.5 game
units outward from Link's left hip**, in gameplay and the Collection preview.
This is an estimate from the reported hilt/scabbard mismatch, pending in-game
comparison with the actual models. It only translates the scabbard; the blade's
180-degree roll, hand placement and draw/stow animation remain unchanged.

Deploying Glide stows the second sword immediately and skips its right-arm
sheathing animation. Both hands retain the native carrying pose for the Glider
or glide Cucco; normal sword drawing and sheathing resume after gliding.

Ordinary sword strikes and their combo finishers alternate hands. The active
blade supplies the hit positions and sword trail. Guarding crosses both swords;
Shield Attack pushes the crossed blades forward and retains the native stun,
timing, and Hidden Skill interactions. Blocking still requires an equipped
shield in the inventory. Other tools and special sword techniques retain their
native actions.

During the victory flourish, the right-hand grip keeps a stable orientation
through sheathing and the empty-hand return instead of following the main
sword's spinning item track. Jump Strike charging uses a separate right-arm
stance: the second sword stays forward and outside the body while the primary
sword remains raised. This also covers holding/moving with the fully charged
attack; attacks, damage, riding and Glide retain their normal transitions.

The feature uses the game's native sword models and Link animations, with native
animation blending and arm IK for the hip draw and cross guard. No replacement
game archives are installed. Turning it off restores normal shield rendering
and sword behavior. Wolf form, events and nonstandard skeletons use native
equipment behavior.

Collection placement follows rendered shield positions rather than fixed grid
columns. It accommodates Twilit Essentials' remapped slots and Twilight HD
HUD's rearranged screen without occupying a native grid cell. See
[Collection compatibility](../../COMPATIBILITY.md#dual-wield-collection-option)
for the inspected versions and remaining in-game checks.

Validation: `python3 tests/second_sword_test.py` covers resource selection, live
model changes, failure recovery and Collection artwork.
`python3 tests/collection_dual_wield_test.py` covers placement,
controller repeat input, pointer handling, menu sounds, shared cursor placement,
frame restoration, page handoff, and save isolation.
`python3 tests/dual_wield_test.py` covers pose math, animation
borrowing/restoration, alternation, blade contact history, menu-preview
equipment transforms/materials, victory-flourish grip stability, Jump Strike
charge/moving-charge arm isolation, and mode boundaries.
Visual transitions still require an in-game check, particularly interrupted
combos, guarding from a holstered stance, and equipment changes.
