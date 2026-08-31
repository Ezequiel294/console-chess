## Why

The game is two-player but one-keyboard: both players must share a terminal. Friends on the same network should be able to play from their own machines — one hosts a match and reads out a short ID, the other types it in, and the two programs find each other and carry the game between them. This is the local-network half of networked play; a future relay server for playing across the internet is explicitly anticipated (the connection's rendezvous is designed to be replaceable) but not part of this change.

**Queued behind `add-chess-clock`.** This change builds on three things the clock change introduces: the input wait that can wake without a keystroke, the new-game setup screen, and the save trailer that records clocks. It must be implemented after `add-chess-clock` lands, and its delta specs modify two capabilities (`new-game-setup`, `chess-clock`) that exist today only as that change's deltas.

## What Changes

- **A LAN mode on the new-game setup screen.** A `Mode` row choosing `2-player` or `LAN` sits above the options, and the rows below it follow the choice: 2-player shows the time rows exactly as the clock change built them; LAN adds a `Play as: White / Black / Random` row. Starting a LAN game shows the match ID and a "waiting for the other player…" popup instead of a board.
- **Joining, from a new main-menu entry.** `Join LAN Game`, between `New Game` and `Load Game`, opens a choice: join a *new* LAN game (type the match ID) or join an *ongoing* one (a list, below). Before a new game starts, the joiner is shown the terms the host chose — the color they will play (`Random` shown as random, before the roll) and the time control — above `Agree, continue` and `Cancel`.
- **Discovery without a server.** The joiner broadcasts the match ID on the local network; the host running that match answers with where to connect; the game then runs over one direct connection. The host's waiting popup also shows its address in small print as a typed fallback for networks that block broadcast. The match ID is a short code from an unambiguous alphabet — a filter for finding the right host, not a secret.
- **Play over the wire.** Moves travel as coordinate text, and each side replays them through its own legal-move generator — an illegal or diverging move is a protocol error, caught immediately by comparing position hashes after every move. The board never flips: each player always sees their own side. There is no SPACE handover — completing a move is the clock press, and an arriving move starts your clock whether or not you are looking.
- **Clocks are self-timed.** In a timed game each machine measures its own thinking time and reports it with the move, so network latency costs neither player anything; the opponent's ticking clock is a live estimate that snaps to the reported truth when the move arrives. Each side declares its own flag fall. Clocks pause the moment a disconnect is detected and resume when both players are back.
- **Draw offers and resignations become messages.** A received draw offer opens a modal on the opponent's screen — always, whoever's turn it is — with `Decline` pre-highlighted, replacing the pass-and-play "moving declines" convention, which cannot apply when the answerer is behind a modal. Resignation ends the game on both screens with the standard result screen.
- **Disconnection pauses, a cache resumes.** A dropped connection shows the remaining player a popup — the opponent is gone, the game is waiting — with the option to leave. Every LAN game is automatically written to a local rejoin cache after every move, so both players can quit and later rejoin with the same ID and continue exactly where the game stopped. The cache is not a saved game: it never appears in Load Game, and it is deleted when the game finishes.
- **An ongoing-games list with a way out.** "Join an ongoing LAN game" lists every unfinished LAN game on this machine — hosted or joined, each resuming its original role. Each entry can also be deleted (abandons the match locally, no result recorded) or resigned-and-deleted (records the loss without connecting; the standard save-and-name offer follows, and the absent opponent simply never receives it — their copy stays ongoing until they act on it too).
- **Finished LAN games end like any other.** The standard result screen appears, and each player independently chooses whether to save — the existing save-and-name flow, producing a normal finished save that Load Game opens as a replay. In-game manual save (`s`) does not exist in LAN games: the cache already preserves the game, and a mid-game save on one machine would say nothing true about a match that lives on two.

Out of scope, deliberately: internet play (the relay server is a future change that swaps only the rendezvous — the protocol and everything above it are unchanged), spectators, more than two players, and any anti-cheating beyond move legality — this is playing a friend, and both machines validate every move anyway.

## Capabilities

### New Capabilities
- `lan-match`: making and finding a match — hosting, the match ID, discovery and its typed fallback, the joiner's terms review, color assignment, and who is host and who is joiner.
- `lan-play`: the game as played over the connection — move exchange and validation, divergence detection, turn and clock semantics without a handover, draw offers and resignation as messages, and what a disconnect does.
- `lan-resume`: continuing an interrupted match — the rejoin cache, the ongoing-games list and its roles, rejoining, deleting, resigning-and-deleting, and what finishing does to the cache.

### Modified Capabilities
- `app-shell`: the main menu gains `Join LAN Game` between `New Game` and `Load Game`; the turn-handover requirement is scoped to pass-and-play games; the in-game command set differs in LAN games (no save, no handover).
- `game-persistence`: "Saving is explicit" gains its one carve-out — the LAN rejoin cache, which is not a save, is invisible to Load Game, and exists only while its game is unfinished.
- `input-events`: the single-input-source requirement is amended — a LAN game's connection is a second source, delivered as events through the same queue, waking the same wait.
- `new-game-setup` *(introduced by `add-chess-clock`)*: the `Mode` row, the LAN-only `Play as` row, and starting a LAN game leading to the waiting popup rather than a board.
- `chess-clock` *(introduced by `add-chess-clock`)*: LAN clock semantics — completing a move is the clock press, self-timed moves, own-flag declaration, and pausing while disconnected.

## Impact

- `src/net/` (new) — sockets, discovery, the line protocol, and the match cache: connection state and message framing, kept apart from `core/`, which stays I/O-free and is what both sides use to validate.
- `src/ui/term.c` / `input.c` — the input wait polls a second file descriptor when a connection exists; a new event type carries network happenings into the loop the clock change already taught to wake on deadlines.
- `src/app/game.c` — a LAN mode of the game screen: remote turns, no flip, no handover, the draw/disconnect modals, commands row differences.
- `src/app/newgame.c` — the `Mode` and `Play as` rows and the hosting/waiting flow.
- `src/app/mainmenu.c` + new screens — `Join LAN Game`, the join-new prompt, the terms review, the ongoing-games list.
- `src/app/save.c` — the cache reuses the save format (FEN, moves, clock trailer) with match metadata; finished LAN saves go through the existing naming flow.
- No new dependencies: POSIX sockets only, macOS and Linux, consistent with the project's zero-dependency C17 character.
- `tests/` — protocol parsing/framing and cache round-trips are testable without a terminal or a peer; two-machine behavior is exercised via the e2e driver where feasible.
