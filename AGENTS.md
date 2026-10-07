# Agent instructions for Dawnlight

## Start here

- Read [project context](docs/project-context.md), [README](README.md), the relevant
  feature guide, and [COMPATIBILITY.md](COMPATIBILITY.md) for shared hooks/UI work.
- Inspect the working tree, current branch, upstream base and relevant PR before
  editing. Preserve unrelated work. Continue a named PR on its existing branch;
  otherwise use a focused branch from current `main` for a new PR.
- Follow the user's current scope. Analysis-only requests must not change code.
  Do not merge, tag, release, bump versions or change dependency pins unless requested.
- Complete already-authorized work through verification and PR creation when
  requested. Ask only when a missing decision affects correctness or the next
  action exceeds that scope. State concrete blockers instead of promising a push.
- Explain results to the maintainer in German unless asked otherwise; follow the
  existing English style for code, repository documentation and PR descriptions.

## Binary files and conversation size

Large texture/model uploads have repeatedly exhausted the conversation context.

- Process BMDs, PNGs, archives and generated binary data on disk. Never print or
  paste Base64, hex dumps, binary Git patches or large byte arrays into conversation
  output or model-authored tool arguments. Avoid whole-file connector reads of binaries.
- Prefer an authenticated Git checkout and ordinary file-based commit/push.
  Plugin authentication does not establish terminal Git authentication.
- Use an API upload only when a verified file/stream path keeps the payload outside
  both the conversation and tool transcript. Merely hiding stdout or computing
  Base64 inside an orchestration call does not establish that guarantee.
- If the only transfer requires a multi-megabyte string in a tool call, stop before
  sending it and explain why. Do not repeat the call, split it into chat messages,
  shrink textures or simplify models merely to make the upload possible.
- After interruption, inspect remote branches/commits before retrying: the upload
  may have completed. Compare local and remote asset hashes/bytes before reuse.
- Report only paths, sizes, hashes, short validation summaries and commit/PR IDs.
  Bound log output; never expose credentials or embed private chat history in docs.

## Implementation constraints

- Dawnlight is a mod of Dusklight. Implement changes here through services/hooks;
  inspect the pinned host sources in `dusklight/` without relying on uncommitted
  changes to that fetched checkout. Follow `src/service_imports.hpp` conventions.
- Check the actual host revision and platform guards before changing hooks or
  private structures. A desktop Linux build cannot prove Android touch/private ABI
  compatibility. Prefer existing named settings/service access over fixed offsets.
- Preserve other mods' hook chains, resource ownership and native behavior when
  Dawnlight features are off. Track scene/actor lifetime; release resources/hooks
  on disable/unload and avoid retaining invalid actor/material pointers.
- Keep manual settings distinct from effective Dawnlight Mode and Progression
  overrides. Preserve save-slot/Boss-Rush separation and reset transient state on
  save/scene changes. Boss Rush unlocks progression abilities, but Gale capacity
  still depends on maximum heart containers.
- Preserve persistent config keys and defaults unless the task changes them.
  Keep gameplay changes optional where the existing feature is optional.
- For assets, preserve non-target geometry, joints, weights, materials and draw
  hierarchy. Validate converted BMDs directly; editor mesh numbers and a successful
  converter exit are insufficient. Do not regenerate unrelated generated assets.
- Check permissions/provenance before copying code or assets from another mod.
  A reference repository is not blanket permission to reuse its implementation.

## Build and validation

From the repository root, the native build matching the CI configuration is:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
git diff --check
```

Use CMake 3.26+, Ninja and a suitable C++ toolchain. Configuration fetches the host
revision from `CMakeLists.txt` and may need network access. An existing matching host
checkout can be selected with `-DDUSKLIGHT_DIR=/path/to/checkout`.

Tests are primarily focused Python scripts under `tests/`; inspect each script's
requirements and `.github/workflows/build.yml` rather than assuming a CTest suite.
Examples: `python3 tests/dual_wield_test.py`,
`python3 tests/bossrush_hook_lifecycle_test.py`, and
`python3 tests/bullet_time_gyro_compat_test.py`.
`python3 tests/hud_fade_config_test.py build` needs a configured Ninja build;
some touch tests need the fetched SDK. Run tests relevant to the change and a
native build for runtime changes when possible. Add regression coverage for an
actual behavioral risk, not tests that simply repeat an implementation.

Documentation-only changes need factual, link/path and whitespace checks, not a
full rebuild. Do not declare historical tests or another platform's tests passed
in the current run. Report skipped checks and the reason.

The local package is `build/mods/dawnlight_mod.dusk`; CI combines platform packages
into the distributable `mod-combined` artifact. Visual animation, touch, timing,
cross-mod and crash fixes still need device/in-game checks. Distinguish those from
source checks or Linux compilation. Finish with the PR link, changed behavior,
validation and any unresolved limitation.
