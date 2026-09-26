# Dual Wield

[← Dawnlight overview](../../README.md)

Under `Gameplay -> Combat`, turn **Dual Wield** on to add an **Ordon Sword**
option to the Collection menu. Select that icon to replace Link's visible
shield with a second Ordon Sword. The setting defaults to off; enabling it
alone no longer equips the second sword.

The icon sits to the right of the shield row, after other visible shield
options. Move right from the last available shield, or click/tap the sword.
Its highlighted frame indicates that Dual Wield is equipped. Link’s menu
preview carries the second sword, and navigating/equipping uses the normal
menu sounds. The existing menu cursor supplies the selection animation,
including styling applied by other mods. Use the shield
slots to leave Dual Wield; those slots retain their existing equip/unequip
behavior, including behavior added by other mods. Selecting the sword requires
human Link and an equipped shield. The choice is saved per save slot, with
separate choices for normal play and Boss Rush. Turning the setting off removes
the icon and clears the current choice.

The secondary sword and its scabbard rest at the left hip when stowed; the
primary sword keeps its normal equipment slot.

Ordinary sword strikes and their combo finishers alternate hands. The active
blade supplies the hit positions and sword trail. Guarding crosses both swords;
Shield Attack pushes the crossed blades forward and retains the native stun,
timing, and Hidden Skill interactions. Blocking still requires an equipped
shield in the inventory. Other tools and special sword techniques retain their
native actions.

The feature uses the game's Ordon models and Link animations, with native
animation blending and arm IK for the hip draw and cross guard. No replacement
game archives are installed. Turning it off restores normal shield rendering
and sword behavior. Wolf form, events and nonstandard skeletons use native
equipment behavior.

Collection placement follows rendered shield positions rather than fixed grid
columns. It accommodates Twilit Essentials' remapped slots and Twilight HD
HUD's rearranged screen without occupying a native grid cell. See
[Collection compatibility](../../COMPATIBILITY.md#dual-wield-collection-option)
for the inspected versions and remaining in-game checks.

Validation: `python3 tests/collection_dual_wield_test.py` covers placement,
controller repeat input, pointer handling, menu sounds, shared cursor placement,
frame restoration, page handoff, and save isolation.
`python3 tests/dual_wield_test.py` covers pose math, animation
borrowing/restoration, alternation, blade contact history, menu-preview
equipment transforms/materials, and mode boundaries.
Visual transitions still require an in-game check, particularly interrupted
combos, guarding from a holstered stance, and equipment changes.
