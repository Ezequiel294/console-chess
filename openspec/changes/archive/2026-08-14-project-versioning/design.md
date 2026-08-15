## Context

See `proposal.md` — Why. Three facts about the current tree shape this design:

- **There is a build now.** `modularize-and-build` replaced `gcc -o chess_game main.c` with a `Makefile`, which is what makes compiling a value in possible at all.
- **`main` takes no arguments.** It is `int main(void)`, so `--version` is not a matter of adding a branch; the signature changes and an argument path appears where there was none.
- **The save file already has a versioned header.** `save.c` writes a magic number and a `uint32_t` format version and validates both. Recording a second, different kind of version means extending a header that already exists rather than inventing one.

The constraint is that this change must not become a release-engineering project. It declares a number and carries it to four places.

## Goals / Non-Goals

**Goals:**

- One declaration, derived everywhere, with disagreement made impossible rather than merely discouraged.
- A build that refuses to produce a mislabelled binary.
- An argument surface that the next change can extend without redesigning it.

**Non-Goals:**

- Automating releases. No tag creation, changelog generation, or CI publishing. The procedure is written down and run by hand.
- Deriving the version from git. Rejected below.
- A build-time configuration system. One value does not need one.
- Migrating existing save files. Consistent with every other save break in this sequence; see Migration Plan.

## Decisions

### `VERSION` is a file, not a Makefile variable

Both put the number in one place. A separate file is chosen because it can be read by anything without parsing a Makefile — a release script, a packaging tool, a human, or a future CI job — and because a version bump then shows up as a one-line diff to a file whose entire purpose is that number, rather than buried in build logic.

The Makefile reads it and passes it to the compiler as a macro. `version.h` wraps the macro so that no other translation unit references it directly; the macro is a build detail, and modules should depend on the accessor.

Alternative considered: `#define CHESS_VERSION "1.1.0"` in a header, no file, no Makefile involvement. Simpler, and it makes the version invisible to every tool that is not a C compiler. Rejected.

### The version is not derived from `git describe`

Tempting, since `v1.0.0` and `v1.0.1` already exist and describing them is free.

Rejected because it makes the build depend on the presence of git history. A source archive, a shallow clone, or an exported tree produces `unknown`, and the one moment the version matters most — someone reporting a bug from a build they got as a tarball — is exactly when it is absent. A declared version is always right because it is always there.

The git tags remain the record of *what was released*; `VERSION` is the record of *what this tree is*. A release makes them agree, and the documented procedure is what keeps them in step.

### The build fails rather than defaulting

A missing or malformed `VERSION` stops the build with a message. The alternative — substituting `0.0.0` or `unknown` — produces a binary that lies quietly, which is worse than no binary, because the lie is only discovered when someone is already debugging something else.

Validation is a pattern check in the Makefile before any compilation happens, so the failure arrives immediately and names the offending text rather than surfacing as a strange compiler error.

### Argument parsing stays hand-rolled and tiny

Three cases: version, help, unrecognised. A `getopt` loop or an options table would be more machinery than the thing it parses.

`terminal-ui-foundation` adds `--ascii`, so this is written as a loop over `argv` with a chain of comparisons rather than a special case for `argc == 2` — adding a fourth option should be adding a branch, not restructuring. Beyond roughly six options this should become a table; it will not reach six.

Options are handled before any terminal setup, locale work, or file access, so `--version` cannot be affected by a broken terminal or an unreadable save. This also matters for the next change, which takes over the terminal on startup: `--version` must never enter the alternate screen.

### The save file gains a version string, and the format version bumps

The header becomes magic, format version, then the writing build's version. Adding a field changes the layout, so `SAVE_VERSION` goes 2 → 3. That bump is caused by the layout change, not by the release — which is exactly the separation the proposal argues for, demonstrated on its first outing.

Stored as a fixed-size character array rather than a length-prefixed string, so the header stays a fixed size and the reader needs no allocation. Sixteen bytes is far more than `MAJOR.MINOR.PATCH` with a pre-release suffix needs, and a version longer than the field is a build-time failure, not a runtime one.

The recorded version is read and made available on load, but never compared against the running build. A save is accepted or refused on its format alone. Comparing versions would recreate the coupling the whole change exists to avoid, and would make every release throw away games in progress.

### The banner line is the smallest possible change

The welcome text is a run of `wprintf` calls in `main.c`. The version goes on the existing title line rather than as a new line, because `terminal-ui-foundation` replaces this output wholesale and rebuilding it here would be work thrown away twice.

## Risks / Trade-offs

- **`VERSION` and the git tags drift apart** → They are different records and cannot be made to validate each other without the git dependency this design rejects. Mitigated by writing the procedure down and keeping it to two steps. A CI check that a tag matches `VERSION` is the natural later fix and is out of scope.
- **A third breaking save format change** → Accepted, and cheap: the guard from `modularize-and-build` already refuses old files with a clear message and leaves them on disk. This is the case that guard was built for.
- **`app-shell-and-persistence` drops the version field when it replaces the format** → Called out in this change's Impact and worth restating in that change's tasks when it is applied. The text format should carry it as a line, which is easier than the binary field being replaced.
- **`main` gaining arguments collides with the next change's startup rewrite** → Small collision, and in the useful direction: `terminal-ui-foundation` shrinks `main` to roughly thirty lines and needs somewhere for `--ascii`. Option handling before terminal setup is the order that change wants anyway.
- **Fixed-size version field silently truncates a long version** → Prevented at build time by the same length check that validates the format, so the failure is a build error and not a corrupted header.

## Migration Plan

No migration. Existing `game_save.bin` files are refused by the existing version guard, with the message it already produces.

This is the third breaking save format change in the sequence and the reasoning has not changed: the format is replaced entirely in `app-shell-and-persistence`, so a converter written now would be written for a format with a known death date.

Deployment is `make`. Rollback is `git revert`; a save written by this change will not load into the previous build, so a rollback loses a game in progress.

Order of work: declare and validate first, then surface it, then record it in saves. Each step is independently useful, and the save change — the only breaking one — comes last so it can be dropped without losing the rest.

## Open Questions

- Whether to add a check that a `vX.Y.Z` tag matches `VERSION` at that commit. It would catch the drift named above, needs a CI job or a hook to be worth anything, and changes no requirement here. Revisit if the two ever actually diverge.
- Whether the version belongs anywhere else in the UI once `terminal-ui-foundation` lands — a status bar corner, or an "about" entry on the main menu that `app-shell-and-persistence` adds. Both are cosmetic and better decided against a real layout than in the abstract.
