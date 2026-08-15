## Why

The project has released twice — tags `v1.0.0` and `v1.0.1` — and nothing inside the project knows it. The version exists only in git, so a built binary cannot say what it is, a player cannot report which build they are running, and a save file on disk gives no clue which build produced it.

That gap is about to get expensive. Four changes are queued that rewrite the terminal layer, the rules engine, the input model, and the save format. Bug reports across that stretch will be worthless without knowing which build they came from, and "it worked before" will be unanswerable.

`modularize-and-build` introduced a `Makefile`, so for the first time there is a build to declare a version *in*. This change declares it once, in a file, and makes the build carry it everywhere it needs to go.

## What Changes

- Add a `VERSION` file at the repository root holding a single version string, starting at `1.1.0`. This is the sole declaration; nothing else states a version number.
- The `Makefile` reads `VERSION` and compiles it in as `CHESS_VERSION`. A build with a missing, empty, or malformed `VERSION` **fails** rather than producing a binary that misreports itself.
- Add `src/version.h` exposing the compiled-in string plus the accessors that surface it, so no other module reaches for the preprocessor macro directly.
- `main` gains argument handling — currently `int main(void)`:
  - `--version` / `-v` prints the name and version and exits successfully.
  - `--help` / `-h` prints usage and exits successfully. Included deliberately: a flag nobody can discover is not much better than no flag, and it is the surface `terminal-ui-foundation` will hang `--ascii` off.
  - An unrecognised argument prints usage and exits with a failure status.
- The existing welcome banner shows the version, so a player can read it off their own screen without knowing any flags exist.
- Saved games record the version of the build that wrote them, alongside the existing format version. **BREAKING** — this adds a field to the save file header, so `SAVE_VERSION` goes from 2 to 3 and saves written by the current build stop loading. The guard added in `modularize-and-build` already refuses them with a clear message, so the failure is clean.
- `README.md` states the current version and documents the release procedure: edit `VERSION`, commit, tag `vX.Y.Z`.

**The project version and `SAVE_VERSION` remain separate numbers, deliberately.** `SAVE_VERSION` identifies the on-disk layout and bumps only when that layout changes. The project version identifies a feature release. Tying them together would mean a typo fix in a prompt string threw away every game in progress.

This is also why the next release is `1.1.0` and not `2.0.0` despite `modularize-and-build` having broken the save format: save compatibility is tracked by `SAVE_VERSION`, so it is not what the project's major number promises. What the project version promises is user-visible features.

## Capabilities

### New Capabilities

- `project-version`: the build's version identity — a single declared source of truth, the requirement that a build cannot be produced without one, and every place the version has to surface (the `--version` flag, the welcome banner, and saved games).

### Modified Capabilities

None. `openspec/specs/` is currently empty — `modularize-and-build` was archived with no deltas, and the four queued changes have not been archived — so there is no existing capability to modify. In particular this change does **not** modify `game-persistence`: that capability exists only as a delta inside `app-shell-and-persistence` and has no baseline yet. The requirement that a save records the writing build's version is stated here as a property of the version, not of the save format.

## Impact

- **Code**: new `VERSION` and `src/version.h`; `Makefile` gains version extraction and a validation guard; `src/main.c` gains an argument parser and a banner line; `src/app/save.c` writes and reads one more header field.
- **Build**: `make` now fails on a missing or malformed `VERSION`. Anyone building from a source archive that omits the file gets a clear error instead of a mystery binary.
- **Data**: existing `game_save.bin` files stop loading. Third breaking save change in this project's history and, like the others, deliberate — see `modularize-and-build`'s Migration Plan for why no converter is written for a format with a known death date.
- **Downstream**: `app-shell-and-persistence` replaces the binary save format with text. It must carry the version field forward into that format rather than dropping it. `terminal-ui-foundation` inherits the argument parser for `--ascii`. Neither is blocked by this change, and this change is not blocked by them.
- **Risk**: low. The only behavioural surface is a new flag, one banner line, and a header field; everything else is build plumbing. No chess logic is touched.
