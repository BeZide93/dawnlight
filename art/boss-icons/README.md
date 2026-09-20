# Boss mirror icons

The 18 PNGs were supplied by the user for Dawnlight in
`Twilight_Princess_Boss_Symbole_Schwarz_PNG.zip` on 2026-09-20.
They are retained byte for byte; the mod embeds their alpha masks, rendered in
black over 30%-opaque blue/red mirror overlays.

Source ZIP SHA-256: `ae70bd6d35c0bab8f3f41bb44a379553ce03da3b4a97ad30af07741a6507e5e9`.

Names match the Boss Rush entry names with spaces replaced by underscores.
Run `python tools/generate_boss_symbols.py` from the repository root to update
`src/generated/boss_portal_art.hpp`. See `docs/boss-portal-symbols.md` for details.
