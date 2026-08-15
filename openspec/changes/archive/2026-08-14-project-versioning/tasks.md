## 1. Declare the version

- [x] 1.1 Add a `VERSION` file at the repository root containing `1.1.0` and nothing else
- [x] 1.2 Whitelist `VERSION` in `.gitignore` — the repository ignores everything not listed
- [x] 1.3 Read it in the `Makefile` into a `VERSION` variable, stripping trailing whitespace and a trailing newline
- [x] 1.4 Pass it to the compiler as `CHESS_VERSION`, quoted so it arrives as a string literal
- [x] 1.5 Add `src/version.h` exposing `chess_version()` returning the compiled-in string
- [x] 1.6 Have `version.h` fail to compile if `CHESS_VERSION` is not defined, so the macro can never be silently absent

## 2. Refuse to build a mislabelled binary

The build must stop before compiling anything, so the message is about the version and not a strange compiler error.

- [x] 2.1 Fail the build when `VERSION` does not exist, naming the missing file
- [x] 2.2 Fail the build when `VERSION` is empty or contains only whitespace
- [x] 2.3 Fail the build when the contents do not match `MAJOR.MINOR.PATCH` with an optional pre-release suffix, quoting the offending text
- [x] 2.4 Fail the build when the version is longer than the save file's version field, so truncation is impossible at runtime
- [x] 2.5 Ensure the check runs before any compilation and that no binary is left behind on failure
- [x] 2.6 Test all four failure modes and confirm each names what is wrong
- [x] 2.7 Confirm a valid `VERSION` builds and that the binary reports exactly that string

## 3. Command-line options

- [x] 3.1 Change `main(void)` to `main(int argc, char **argv)`
- [x] 3.2 Handle options before locale setup, terminal output, and any file access, so a broken environment cannot affect them
- [x] 3.3 Parse as a loop over `argv` rather than a special case on `argc`, so `terminal-ui-foundation` can add `--ascii` as one more branch
- [x] 3.4 `--version` / `-v`: print name and version, exit successfully, start no game
- [x] 3.5 `--help` / `-h`: print usage listing every accepted option including `--version`, exit successfully, start no game
- [x] 3.6 Unrecognised option: print usage, exit with a failure status, start no game
- [x] 3.7 No options: start a game exactly as before
- [x] 3.8 Verify `--version` works with output redirected to a file and to a pipe
- [x] 3.9 Verify the exit status is zero for `--version` and `--help`, and non-zero for an unrecognised option

## 4. Show the version while playing

- [x] 4.1 Put the version on the existing welcome banner's title line, not on a new line
- [x] 4.2 Confirm it matches `--version` output exactly
- [x] 4.3 Keep it to one derived value — read it through `chess_version()`, never a second literal

## 5. Record the version in saved games

Last, because it is the only breaking step and the rest stands without it.

- [x] 5.1 Extend the save header with a fixed-size version field after the format version
- [x] 5.2 Bump `SAVE_VERSION` from 2 to 3, and update the comment in `save.c` explaining what each version was
- [x] 5.3 Write the running build's version when saving
- [x] 5.4 Read the field on load, checking the read as every other field is checked
- [x] 5.5 Make the loaded version available to the caller without comparing it against the running build
- [x] 5.6 Confirm a save written by a different version string still loads when the format version matches
- [x] 5.7 Confirm a version 2 save is refused with the existing message and left on disk
- [x] 5.8 Confirm a truncated version field is caught as a corrupt file rather than read as garbage

## 6. Documentation

- [x] 6.1 State the current version in `README.md`
- [x] 6.2 Document the release procedure: edit `VERSION`, commit, tag `vX.Y.Z`
- [x] 6.3 Document that the project version and the save format version are separate, and what each one promises
- [x] 6.4 Note that `VERSION` must be present to build, so anyone packaging a source archive includes it

## 7. Verification

- [x] 7.1 Play a full game and confirm nothing about gameplay changed
- [x] 7.2 Save, quit, and resume; confirm the position and history are exact
- [x] 7.3 Change `VERSION`, rebuild, and confirm every surface reports the new value with no other file edited
- [x] 7.4 Confirm a save written before the version change still loads after it
- [x] 7.5 Rebuild clean under `-Wall -Wextra` with no new warnings
- [x] 7.6 Run under a leak checker and confirm no regression from the argument handling
