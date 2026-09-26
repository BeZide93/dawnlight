# Dual Wield

[← Dawnlight overview](../../README.md)

Under `Gameplay -> Combat`, turn **Dual Wield** on to replace Link's visible
shield with a second Ordon Sword. The option defaults to off. The secondary
sword and its scabbard rest at the left hip when stowed; the primary sword keeps
its normal equipment slot.

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

Validation: `python3 tests/dual_wield_test.py` covers pose math, animation
borrowing/restoration, alternation, blade contact history, and mode boundaries.
Visual transitions still require an in-game check, particularly interrupted
combos, guarding from a holstered stance, and equipment changes.
