## 1. Preflight

- [ ] 1.1 Confirm `add-chess-clock` is implemented and archived; diff this change's `new-game-setup` and `chess-clock` MODIFIED blocks against the archived main specs and reconcile any drift, verified by `openspec validate add-lan-multiplayer` passing and the copied blocks matching the main specs word for word
- [ ] 1.2 Verify the clock change's bounded `input_next`, expiry event, and monotonic time source exist and are green under `make` + the test suite, since every group below builds on them

## 2. Protocol and framing (src/net/, no sockets yet)

- [ ] 2.1 Create `src/net/proto.c/.h`: parse and format every protocol line from design D3 (`hello`, `start`, `move`, `draw?`, `draw!`, `draw-no`, `resign`, `flag`, `resume`, `tail`, `ping`, `pong`, `bye`) with bounded field and line lengths, verified by unit tests covering every verb round-trip and rejection of unknown verbs, overlong lines, and malformed fields
- [ ] 2.2 Add a line-framing buffer (bytes in → complete lines out, partial lines held), verified by unit tests feeding messages split at every possible byte boundary and two messages arriving in one read
- [ ] 2.3 Add match-ID generation from the unambiguous alphabet, verified by a unit test asserting length, alphabet membership, and absence of the excluded lookalike characters

## 3. Sockets, discovery, and events

- [ ] 3.1 Create `src/net/session.c/.h`: TCP listen (ephemeral port), connect, non-blocking send with a small pending buffer, orderly close, verified by a unit test running both ends over `socketpair()`/loopback exchanging framed lines
- [ ] 3.2 Implement UDP discovery: joiner-side `who-has <ID>` broadcast repeated ~1/s, host-side listener answering unicast with its TCP port, verified by a loopback test where a searcher finds a host by ID, a wrong ID finds nothing, and cancel stops the broadcast
- [ ] 3.3 Extend `term_read`/`input_next` to poll the session fd when one is registered and deliver connection events (line arrived, connection closed/failed) through the event queue, verified by tests showing a socket line wakes the wait without input, is never delivered as a key event, and never truncates a partial escape sequence (input-events delta scenarios)
- [ ] 3.4 Implement ping/pong liveness and disconnect declaration after the silence threshold, verified by a test where one end goes quiet and the other receives a disconnect event within the threshold

## 4. Match lifecycle (hello, terms, colors)

- [ ] 4.1 Implement the connect handshake: `hello` exchange, protocol-version gate refusing mismatches with the incompatibility message on both ends, verified by a test connecting mismatched versions and asserting no game starts (lan-match "Incompatible programs" scenario)
- [ ] 4.2 Implement terms flow: host sends terms, joiner agree/cancel, `start` carrying the joiner's color (random rolled host-side only after agree) and the FEN + time control, verified by tests for agree, cancel-returns-host-to-waiting, and a random-roll distribution sanity check
- [ ] 4.3 Wire the setup screen: `Mode` row (pass-and-play / LAN) as the first row, LAN-only `Play as` row, time settings preserved across mode switches, verified against the new-game-setup delta scenarios by driving the screen in the e2e harness
- [ ] 4.4 Build the host waiting screen: match ID large, `ip:port` fallback in small print, cancellable, transitions to the board when a joiner agrees, verified by two-instance smoke test reaching a live board on both ends
- [ ] 4.5 Build the join flow screens: `Join LAN Game` menu entry (between New Game and Load Game), the new/ongoing choice, the ID prompt accepting an ID or a typed `ip:port`, the searching state with cancel, and the terms-review screen with `Agree, continue`/`Cancel` (cancel highlighted), verified against the lan-match scenarios via the e2e harness

## 5. The LAN game on the board

- [ ] 5.1 Add the LAN variant of the game screen's context (role, local color, session handle) with fixed orientation, no handover, and an arriving verified move appearing without local input, verified by two-instance test playing moves both ways (lan-play "Each player sees the board from their own side")
- [ ] 5.2 Apply incoming moves through `generate_legal_moves` and compare the carried Zobrist after every move; on mismatch or illegal move, stop the match on both ends with the divergence error, verified by unit tests injecting an illegal move and a hash mismatch
- [ ] 5.3 Implement the LAN command set: no save (`s` inert and not listed), quit confirms leaving and names rejoinability, verified by e2e assertions on the commands row and the quit dialog (lan-play + app-shell delta scenarios)
- [ ] 5.4 Implement draw offers and resignation over the wire: offer modal on the receiver (Decline highlighted, whoever's turn), explicit accept/decline, pending indicator for the offerer, resignation ending both ends on the standard result screen, verified by two-instance tests of accept, decline, and resign
- [ ] 5.5 Implement disconnect handling in-game: pause on disconnect event, the waiting modal with leave, resume when the peer returns, no result recorded by a drop, verified by killing one instance mid-game and asserting the survivor's modal, pause, and clean leave

## 6. LAN clocks

- [ ] 6.1 Make move completion the clock press in LAN games (increment applied at the move, incoming clock started on arrival regardless of attention), verified by clock unit tests against the chess-clock delta scenarios
- [ ] 6.2 Implement self-timed moves: local measurement excluding paused intervals, `elapsed-ms` on the wire, receiver charging the reported time, opponent-clock estimate corrected on arrival, verified by unit tests including an artificial-delay case proving latency costs nothing
- [ ] 6.3 Implement own-flag declaration (`flag` ends the game on both ends as loss on time, insufficient-material exception included) and both-clocks-held while disconnected, verified by unit tests plus a two-instance flag test

## 7. Cache and rejoin

- [ ] 7.1 Extend the save trailer with the match keys (match ID, role, color, terms) and write the cache atomically after every completed move to the cache directory beside `games/`, verified by round-trip unit tests and an assertion that Load Game's scan never lists cache files
- [ ] 7.2 Build the ongoing-games list: scan of the cache directory showing last-played, ID, color, move count, with rejoin / delete (confirmed) / resign-and-delete (confirmed, then the standard save-and-name offer), verified against the lan-resume scenarios via e2e
- [ ] 7.3 Implement rejoin: original role resumed (host listens under the same ID, joiner searches), waiting state until the peer arrives, `resume` exchange, `tail` reconciliation for a one-move gap, refusal with explanation for irreconcilable histories, verified by unit tests for equal/one-off/conflicting histories and a two-instance disconnect-quit-rejoin-continue test
- [ ] 7.4 Clear the cache on every terminal path — finish (any result), delete, resign-and-delete — and confirm a finished LAN game offers the standard independent save flow whose product opens as a replay, verified by e2e walking finish-save-replay and asserting the cache file is gone

## 8. Integration and docs

- [ ] 8.1 Give the e2e driver a two-instance mode (two PTYs on one host over loopback) and add a full smoke test: host, join, terms, timed game, draw offer declined, disconnect, rejoin, checkmate, both players saving independently — verified by the script passing on macOS and Linux
- [ ] 8.2 Verify every delta-spec scenario in this change has a covering test or a noted manual check, and run the full suite plus `make debug` (ASan/UBSan) through a two-instance game with zero findings
- [ ] 8.3 Update README (Join LAN Game, match IDs, the fallback address, no-save-in-LAN, rejoin) and in-game help, verified by proofreading against the shipped behavior
- [ ] 8.4 Run `openspec validate add-lan-multiplayer --strict` and resolve any findings, verified by a clean pass
