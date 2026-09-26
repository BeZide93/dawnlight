# Arrow Modes

[← Dawnlight overview](../../README.md)

Added in Dawnlight 3.1.0. Open
`Mod Manager -> Dawnlight -> Open Dawnlight Settings -> Gameplay -> Combat`
and use the `Arrow Modes` toggle. It is enabled by default and works independently
of Hard Mode.

While actively aiming the Bow, tap **ZR** to cycle
**Normal -> Fire -> Triple Shot -> Normal**. Merely holding the Bow does not
change modes or reserve ZR. Arrow Modes support Vanilla, 3rd Person, and Cinema
aiming.

| Mode | Arrows consumed per shot | Effect |
| --- | --- | --- |
| Normal | 1 | Standard arrow behavior and damage. |
| Fire | 2 | 50% more damage; ignites lantern-compatible torches, wood, and spiderwebs. |
| Triple Shot | 3 | Three normal arrows in a horizontal spread. |

A brief arrow, flame, or spread icon above Link shows the selected mode; dots
indicate its ammunition cost. In first-person aim the indicator appears near
the center of the screen. If ammunition is insufficient, the indicator turns
red and the shot is blocked.

Fire arrows show the original Bulblin flame while aiming and in flight, including
in rooms without Bulblins. The effect is loaded from your game disc, so no extra
asset files are required. Fire arrows extinguish in water. Bomb arrows keep
their normal behavior, and Hawkeye zoom remains available through the native
item-action button.

Switching `Arrow Modes` off takes effect without restarting, returns the Bow to
normal arrows, and releases ZR for other actions. Arrows already fired retain
their effects. See [COMPATIBILITY.md](../../COMPATIBILITY.md#arrow-modes-and-zr-input)
for input considerations.
