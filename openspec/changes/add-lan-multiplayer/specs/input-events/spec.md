## MODIFIED Requirements

### Requirement: Single input source

Exactly one component SHALL read from standard input. No other part of the system may read it, whether directly or through buffered library calls. The same discipline SHALL apply to a LAN game's connection: exactly one component reads it, and what it reads reaches the application only as typed events through the same queue every other event uses.

Two readers on one descriptor each hold their own buffer, so bytes are intermittently lost or reordered. This is a correctness requirement, not a stylistic one, and it is no less true of a socket than of a terminal.

#### Scenario: Rapid input
- **WHEN** the user types faster than frames are drawn
- **THEN** every keystroke is delivered exactly once, in order

#### Scenario: One reader per source
- **WHEN** a LAN game is in progress
- **THEN** the terminal and the connection each have exactly one reader, and both deliver typed events into the same queue

## ADDED Requirements

### Requirement: The connection wakes the wait

While a LAN game's connection exists, waiting for the next event SHALL return when something happens on the connection — a message arriving, the connection closing or failing — just as it returns for a keystroke, a resize, or an expired deadline. What happened SHALL be delivered as its own event type, never mistaken for input the user produced, and never requiring a keystroke to be noticed.

A connection event SHALL NOT corrupt terminal input mid-arrival, and terminal input SHALL NOT delay a connection event past the point a person would notice — the same guarantees deadline expiry already carries.

#### Scenario: A message arrives while idle
- **WHEN** the opponent's move arrives and the local player is touching nothing
- **THEN** the wait returns with a connection event and the move appears, with no local keystroke involved

#### Scenario: Not confused with input
- **WHEN** a connection event is delivered
- **THEN** no key, mouse, paste, or resize event is produced by it

#### Scenario: The connection fails
- **WHEN** the connection closes or errors while the wait is blocked
- **THEN** the wait returns with an event saying so, rather than blocking on until the user types
