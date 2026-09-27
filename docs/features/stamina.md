# Stamina settings

[← Movement & abilities](movement-and-abilities.md#shared-stamina)

Open **Dawnlight Settings -> Controls -> Stamina Settings**, directly below
**Stamina Bar**. The button is enabled only while Stamina Bar is On. Turning
Stamina Bar Off disables Dawnlight's stamina costs.

Costs and recovery rates use **stamina points**, not percentages of the current
capacity. Setting an action's cost to **0** makes it free, even during exhaustion.
The exhaustion threshold is a percentage of maximum stamina.

| Setting | Range | Default |
| --- | --- | --- |
| Stamina Amount | 50–500 | 100 |
| Recovery speed | 1–100/sec | 5/sec |
| Exhaust Threshold | 0–100% | 50% |
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
You can open the menu to view the locked values.

**Progression System** adds 10 maximum stamina per complete heart above the three
starting hearts. The bonus follows the loaded save's maximum hearts, including
hearts already collected; incomplete heart pieces and current damage do not affect
it. It is added after the base amount, including Dawnlight Mode's default:
3 hearts = 100, 6 hearts = 130, 20 hearts = 270. Turning Progression Off removes
the bonus. The meter displays current stamina relative to this effective maximum.

Continuous costs run per real second; menus pause their accounting. Glide adds
its cost to Bullet Time. Recovery runs when there is no continuous drain; exhausted
recovery runs until the configured threshold is reached. A threshold of 0 resumes
paid actions as soon as any stamina has regenerated; 100 requires a full refill.

Block is charged once per shielded hit, including Dual Wield. Guard Break uses its
own cost **instead of** Block. These defensive reactions retain native behavior
and drain the remaining stamina if their cost exceeds it. Holding guard alone
has no cost. Paid attack skills require enough stamina and charge on successful
activation; Midna Attack charges when opening the wolf lock-on field. Shield
Attack, Back Slice, Helm Splitter and Midna Attack work with these costs even
without Lazy Tweaks. The existing Lazy Tweaks meter compatibility remains enabled.
