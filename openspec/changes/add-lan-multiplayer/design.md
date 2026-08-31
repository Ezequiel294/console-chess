## Context

See proposal.md for motivation. Constraints that shape the approach:

- **Queued behind `add-chess-clock`.** This change assumes the clock change has landed: the bounded input wait and its expiry event, the new-game setup screen, the monotonic time source, and the save trailer's clock keys all exist. Two delta specs here (`new-game-setup`, `chess-clock`) modify requirement blocks copied from that change's deltas; if those blocks are edited before it archives, the copies here must be reconciled.
- **Zero dependencies, C17, macOS and Linux.** POSIX sockets only — no TLS, no mDNS library, no serialization library. Everything below follows from taking that seriously.
- **`core/` stays I/O-free.** The game itself must not learn what a socket is. Networking is a peer of `ui/`, not of `board.c`.
- **The app is a single-threaded event loop** (`app_run` → `input_next` → `term_read` → `poll`). The clock change already teaches this loop to wake on a deadline; this change adds a file descriptor to the same `poll`, not a thread.
- **A future relay server (option C) is anticipated.** The design must keep "how the two sockets found each other" separate from "what travels over them," so the relay change swaps only the first part.

## Goals / Non-Goals

**Goals:**
- One direct TCP connection per match, found by broadcast on the LAN, with a typed-address fallback.
- A line-based text protocol a person can read in a packet capture, rendezvous-agnostic above the connection.
- Both ends validate every move with the same `core/` code; divergence is detected by Zobrist hash on every move.
- Single-threaded throughout; the connection is an event source, never a second thread.
- The rejoin cache reuses the save format and its clock trailer, plus match-metadata keys.

**Non-Goals:**
- No internet traversal, no relay, no encryption, no authentication beyond the protocol-version check. The trust model is "a friend on my network"; move legality is enforced, clock honesty is not.
- No spectators, no more than two machines, no simultaneous matches in one process.
- No attempt to keep playing through a network change (new IP mid-game is a disconnect + rejoin).

## Decisions

### D1: Rendezvous is a replaceable step with a narrow output

The connection is established by a *rendezvous* step whose only output is a connected socket. Three implementations share that contract: LAN discovery (this change), a typed `ip:port` (this change, the fallback), and a relay connection (future). Everything above — hello, terms, moves — is identical across all three, which is what makes option C "a different way to make the same connection" rather than a protocol fork.

*Alternative considered:* baking discovery into the protocol (host beacons carrying game state). Rejected — it entangles finding a match with playing one, exactly the seam the relay needs to cut along.

### D2: Discovery is joiner-initiated UDP query/response

The joiner broadcasts `who-has <ID>` to a fixed UDP port (one constant in `src/net/`), about once a second while the search screen is up. The host listens on that port and answers — unicast, to the asker — with its TCP port; the joiner then connects to the sender's address. The host's TCP listener uses an ephemeral port: only the UDP port is well-known, so two hosts on one machine never collide on TCP, and the OS hands out the rest.

*Alternatives considered:* mDNS/Bonjour (a dependency or a protocol implementation many times this feature's size); host-side beaconing (chattier — it broadcasts for as long as a match waits, where a querying joiner broadcasts only while someone is actively trying to join, and retry-until-found falls out naturally).

### D3: The protocol is newline-delimited text, one message per line

Space-separated fields, first field is the verb, bounded line length, unknown verbs are a protocol error. Over TCP, which already provides ordering and delivery, framing by newline is sufficient.

```
hello <proto-version> <app-version>          both sides, immediately on connect
start <color> <fen> <time> <inc>             host → joiner after terms agreed; color is the joiner's
move  <coord> <elapsed-ms> <zobrist-hex>     either side; elapsed-ms is 0 in untimed games
draw?                                        offer
draw! | draw-no                              answer
resign
flag                                         own clock reached zero on its own machine
resume <move-count> <zobrist-hex>            both sides on rejoin, before play restarts
tail  <n> <moves...>                         rejoin reconciliation: the missing final move(s)
ping | pong                                  liveness, a few seconds apart
bye                                          orderly leave (quit, cancel at terms)
```

The vocabulary is the save format's: coordinate moves and FEN. `core/notation.c` already speaks it, and a wire transcript is nearly a save file — deliberate, since the cache and the protocol then share parsing and validation code.

*Alternative considered:* a binary format. Nothing here needs the bytes saved, and text is debuggable with `nc`.

### D4: The connection is a poll'd fd delivering events — no threads

`term_read`'s `poll` gains an optional second descriptor, registered while a connection exists. Readable socket → bytes are drained into a line buffer in `src/net/`, and each complete line surfaces as one event (per the input-events delta) through the existing queue. Connection close/error surfaces as its own event. Nothing blocks on the socket: sends are small and buffered by the kernel; in the unlikely case a send would block, the bytes queue in `src/net/` and flush when `poll` reports writable.

*Alternative considered:* a reader thread. It buys nothing the fd doesn't and costs synchronization over every piece of game state the app has, in a codebase with zero locks today.

### D5: Move validation and divergence detection ride on `core/`

A received `move` is applied through `generate_legal_moves` exactly as a local one; an illegal move is divergence, full stop. After applying, the local Zobrist hash is compared to the hash carried on the line; any mismatch stops the match with the divergence error on both ends (the detecting side reports it before closing). This is the whole anti-corruption story, and it is cheap because `zobrist.c` already maintains the hash incrementally.

### D6: Clocks are self-timed; pauses are subtracted at the source

Each machine measures its own player's thinking time against the monotonic source the clock change added, and reports it in `move`. Time spent while the match is paused (disconnect detected → resumed) is excluded from that measurement *by the mover's own machine* — the one place that knows both numbers. The opponent's on-screen clock between their moves is a local estimate corrected by each arriving `move`; flags are declared by `flag` from the machine whose clock ran out. Disconnect pause, resume values, and the fairness rationale are spec'd in `chess-clock` and `lan-play`; the design point is that no clock value ever needs two machines to agree in real time — every authoritative number has exactly one author.

### D7: The rejoin cache is a save file with extra trailer keys

Cache files live in their own directory beside `games/` (so the Load Game scan never sees them), one per match, named by match ID. The content is the existing save format — FEN, moves, result-less, clock trailer — plus keys for: match ID, local role (host/joiner), local color, and the terms. Written after every completed move by the same code path that writes saves; deleted on finish, delete, or resign-and-delete. The ongoing-games list is a directory scan of exactly this directory, reusing the saved-games list's loader and screen shape.

*Alternative considered:* a distinct cache format. It would need its own parser, its own validator, and its own bugs, to hold the same information.

### D8: Rejoin reconciliation is "longest verified history wins"

On reconnect, both sides send `resume <move-count> <zobrist>`. Equal counts + equal hash → play on. Counts differing by exactly one → the longer side sends `tail`, the shorter side applies and verifies it like any move. Anything else — equal counts with different hashes, a gap of two or more (impossible if the cache is written per move) — is refused with an explanation, per the lan-resume spec. The host re-rolls nothing on rejoin: color and terms come from both caches and must agree, which the `resume` hash implicitly checks.

### D9: Liveness is application-level pings

`ping` every few seconds of silence, disconnect declared after a small multiple of that with nothing received (TCP keepalive's defaults are minutes and not portable knob-for-knob). Detection triggers the pause + modal on the surviving side; the dropped side usually knows first (its send fails). Reconnection is not automatic in-game: the surviving player waits or leaves; the dropped player rejoins through the ongoing list — one path to resume, not two.

### D10: Screens reuse the existing stack idiom

New screens are ordinary `Screen`s: the join-choice menu, the ID prompt (reusing the save-name prompt's line editor), the terms review, the ongoing list (shaped like savedgames.c), and the waiting/disconnected modals (shaped like confirm.c). The game screen gains a LAN variant of its context — remote color, connection handle, pending-offer state — not a new screen; its render and input paths branch where pass-and-play behavior (flip, handover, save key) doesn't apply.

## Risks / Trade-offs

- [Broadcast blocked by the network (AP isolation, guest wifi)] → the typed `ip:port` fallback is spec'd, always visible on the host's waiting screen, and exercised in tests — it is a first-class path, not an easter egg.
- [Half-open TCP: one side gone, other never notified] → application pings (D9) bound the silence; the modal appears within seconds, never minutes.
- [Two machines running different builds] → `hello` gates on protocol version; per-move Zobrist (D5) catches anything subtler. The failure is loud and immediate by design.
- [Reported elapsed time can be dishonest] → accepted; trust model is a friend. The relay change is where any stronger stance would live, and nothing here precludes it.
- [Cache written after every move wears on disk / partial writes] → ~100-byte file, written atomically (write temp, rename) the way saves already are; a torn cache fails validation on load and is reported, not half-loaded.
- [Two-instance testing is awkward] → protocol framing, discovery packets, reconciliation, and cache round-trips are all testable as pure functions or over `socketpair()` without a terminal; the e2e driver gains a two-PTY mode only for smoke tests.
- [The `new-game-setup`/`chess-clock` MODIFIED blocks are copies of another change's deltas] → before implementing, diff them against what `add-chess-clock` actually archived and reconcile.

## Migration Plan

Nothing migrates: the save format is reused unchanged, existing saves are untouched, and the cache directory is new. Rollback is removing the feature; no data written by it is load-bearing for anything else.

## Open Questions

- The UDP port constant, the ID length/alphabet, and the ping cadence are all single constants chosen at implementation time; nothing above depends on their exact values.
