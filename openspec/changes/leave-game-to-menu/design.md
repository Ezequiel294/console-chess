## Context

See proposal.md — Why. What shapes the approach is where the picker sits and what the stack under it looks like:

- **The picker is an overlay on top of the game screen** (`quit_screen` in `game.c`), pushed by `q`. It returns a `Cmd_t` and the loop applies it; a screen never touches the stack itself.
- **The stack under a live game is not always one deep.** A game reached from Load Game has the saved-games list beneath it, and a game reached from New Game now has the setup screen beneath it too (see `add-chess-clock`). A plain `CMD_POP` would land on whichever of those happened to be there.
- **`gameover.c` already solves exactly this**: its Return to Menu returns `CMD_RESET` with `mainmenu_screen(state)`, for the same reason. `CMD_RESET` clears the whole stack and pushes one screen as the new root.
- **`main` owns the `GameState`** and frees its lists at exit; the main menu frees them again before starting or loading a game (`free_state_lists`). Nothing else owns them, so leaving a game to the menu need not free anything and must not.
- **The save path is `save_flow`**, shared today by the in-play save command and by "Save and quit". It already handles both "this game has a file" and "ask for a name first", and it returns a caller-supplied `Cmd_t` once the save is done.

## Goals / Non-Goals

**Goals:**

- One destination for leaving a game, reached the same way whatever the stack under it looks like.
- The save question, the name prompt, and everything written to disk are byte-for-byte what they are today. Only where the player lands changes.
- No new ownership of the `GameState`: leaving a game frees nothing and leaks nothing, and the next game starts from the same clean slate it does now.

**Non-Goals:**

- Any new way to exit the program. `Quit` on the main menu and `Ctrl-C` are what there is, and neither changes.
- A confirmation on the main menu's own `Quit`. It is one Enter from a screen with nothing to lose, and adding a second question there is the opposite of this change.
- Warning about an unsaved game anywhere other than this picker.
- Changing what a *finished* game does. The result screen's Return to Menu already goes where this change is sending a live game.

## Decisions

### `CMD_RESET` to a fresh main menu, not `CMD_POP`

Both leaving options return `(Cmd_t){CMD_RESET, mainmenu_screen(state)}`.

*Why:* the stack beneath a live game is one of three shapes — the menu alone, the menu plus the saved-games list, or the menu plus the setup screen — and "leave to the menu" must mean the same thing in all three. `CMD_RESET` says the destination rather than a number of pops, so it cannot be made wrong by a future screen that opens a game from somewhere new.

*Alternative rejected:* popping until the menu is on top. That is the same outcome by a route that has to know what it is popping past, and it puts stack arithmetic inside a screen, which `app.h` deliberately keeps out of them.

### The save-and-leave path is the existing save path with a different `Cmd_t`

`save_flow` already takes "what to return once a plain save is done" as a parameter, and the name-prompt callback returns its own `Cmd_t`. Save-and-leave is that same flow with `CMD_RESET` in place of `CMD_QUIT`, in both the direct case and the prompt's submit callback.

*Why:* the two paths — a game that has a file, and one being named for the first time — already differ, and duplicating the leave destination into a third place is how the two get out of step. There is exactly one save flow and it gains one more caller.

*Consequence worth stating:* the name prompt is an overlay on top of the picker, which is on top of the game. Its submit callback returns `CMD_RESET`, which clears all three at once. That is what `CMD_RESET` is for, and it is why the prompt does not need to know it is being used on the way out.

### The clock is synchronised by the save, as it already is

`do_save` calls `clock_sync` before writing, so a game saved on the way out records the reading at that moment rather than at the start of the turn. Leaving changes nothing here: the save happens first and the stack moves afterwards.

Leaving *without* saving needs no clock handling at all. The clock lives on the `GameState`, which the next New Game or Load Game overwrites wholesale.

### Wording: the picker is about the game, not the program

Title `Leave this game?`; options `Save and leave`, `Leave without saving`, `Cancel`; and `Leave` / `Cancel` in the two-option form when there is nothing worth saving. The hint row is unchanged.

*Why:* the picker's job is to make losing a game deliberate. Saying "quit" invited the reading the current behaviour actually has — that this is how you exit — and the labels are the only place a player learns otherwise.

The in-game command hint and the help screen say `leave` rather than `quit` for the same reason. The key stays `q`: it is what players' hands know, it is in the README's table, and no other letter is free.

### Deliberately keeping four scenario headings that say "Quit"

The `app-shell` delta renames `Quitting offers to save` to `Leaving a game offers to save` and rewrites its text, but keeps the headings `Quit mid-game`, `Quit cancelled`, `Quit with save` and `Quit without saving`. `openspec validate` refuses a `MODIFIED` block that drops a scenario the main spec still carries — the guard that stops content vanishing at archive time — so the headings are pinned even though their text changes.

They are not wrong, only less precise than the requirement's new name: the player is still quitting *the game*. Renaming them would mean removing and re-adding the requirement, which defeats a guardrail to fix a label.

## Risks / Trade-offs

- **Muscle memory.** Someone used to `q`, arrow, Enter meaning "done for the evening" now lands on the menu and needs one more Enter. → Accepted, and it is the safer direction to be wrong in: the extra step costs a keystroke, whereas the current behaviour costs a relaunch. The labels say `Leave`, so the screen states what it does before the player commits.
- **Two unarchived changes already touch these specs, in four of the same places.** `add-chess-clock` (built, unarchived) and `add-lan-multiplayer` (planned only) both modify `app-shell`, and the three changes overlap on `Main menu` (all three), `Turn handover` (clock and LAN), `Commands replace prompting` (LAN and this one), and `Quitting offers to save` (LAN modifies it; this change renames it). `game-persistence`'s `Saving is explicit` is modified by LAN and by this change. A delta is written against the main spec as it stands, and archiving replaces the whole requirement — so whichever change archives second silently overwrites the first's edit to a shared requirement, with no error to notice. → Two of these are not this change's to solve: the clock/LAN overlaps pre-date it. What this change does about its own is below.

- **`Quitting offers to save` is renamed here and modified by name in `add-lan-multiplayer`.** If this change archives first, LAN's delta targets a requirement that no longer exists under that name. → This change is 15 tasks and LAN is 30 not yet started, so this one archives first in any realistic order. LAN's deltas are reconciled against the resulting main spec before it is built — noted in that change's own design.md so it is read at the right moment rather than remembered. Making the two order-independent is not achievable: whichever carries the new name breaks if it archives first, so the ordering is stated instead of pretended away.

- **This change previously dropped LAN's "pass-and-play" qualifier** from `Saving is explicit`, which would have left the spec claiming every game in progress offers a save on the way out — contradicting LAN's rule that a LAN game has no save at all. → The qualifier is now carried in this change's delta. It is accurate today, since every game is a pass-and-play game, and it means archiving this change cannot narrow or widen what LAN needs that requirement to say.

- **`add-lan-multiplayer` will need to decide what leaving means for a LAN game.** Its spec says a LAN game stays open to rejoin, so "leave without saving" reads differently there. → Out of scope; that change owns the answer, and this one leaves it a `Leaving a game offers to save` requirement that is about the pass-and-play game it was always about.
- **A leaked `GameState` would now be reachable.** Leaving to the menu keeps the old game's lists alive until the next game frees them, where quitting used to end the process. → It is the same lifetime a finished game already has after Return to Menu, and `free_state_lists` runs before every New Game and every Load Game. Worth an ASan pass rather than an argument.

## Migration Plan

No migration. Nothing on disk changes, no save file is read or written differently, and a game saved by an older build is unaffected. `VERSION` gets a patch bump at release time under the existing procedure in README.md.

## Open Questions

None.
