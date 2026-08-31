## Purpose

Makes and finds a match between two machines on the same local network: one player hosts and gets a short match ID, the other types that ID in, and the two programs locate each other and agree the terms — with no server, no account, and no configuration.

## ADDED Requirements

### Requirement: Hosting a match produces a short ID

Confirming a LAN game on the setup screen SHALL create a match and display a short match ID, together with a "waiting for the other player" message that stays until an opponent joins or the host cancels.

The ID SHALL be short enough to read aloud and type from memory, and SHALL avoid characters that are easily mistaken for one another. It identifies the match to other machines on the network; it is not a secret and not a password.

Cancelling the wait SHALL close the match: the ID stops answering, and — no move having been played — nothing is kept.

#### Scenario: Hosting
- **WHEN** the host confirms a LAN game on the setup screen
- **THEN** a match ID is displayed with a waiting message, and no board is shown yet

#### Scenario: The ID is typable
- **WHEN** a match ID is displayed
- **THEN** it is a short code with no characters that read as one another (no `0`/`O`, no `1`/`l`/`I`)

#### Scenario: Cancelling the wait
- **WHEN** the host cancels while waiting for an opponent
- **THEN** they are returned to where they came from, the ID no longer answers on the network, and nothing has been saved or cached

### Requirement: A match is found by its ID on the local network

Entering a match ID SHALL make the program search the local network for the machine hosting that match, and connect to it when found — with no server involved and nothing configured in advance. While searching, the screen SHALL say so, and the search SHALL be cancellable.

Only the match with that ID SHALL answer: two matches on the same network SHALL never be confused, and a mistyped ID SHALL find nothing rather than the wrong game.

Some networks do not let machines find each other this way. The host's waiting screen SHALL therefore also display its own address, and the joining screen SHALL accept a typed address in place of an ID, reaching the same match over the same connection.

#### Scenario: Joining by ID
- **WHEN** the joiner enters the ID of a match being hosted on the same network
- **THEN** the two machines connect and the joiner is shown the match's terms

#### Scenario: A wrong ID
- **WHEN** the joiner enters an ID no host on the network is using
- **THEN** the search finds nothing and says so; it does not connect to a different match

#### Scenario: Cancelling the search
- **WHEN** the joiner cancels while searching
- **THEN** they are returned to the menu and no connection is made

#### Scenario: The typed fallback
- **WHEN** the network does not carry the search and the joiner instead types the address shown on the host's screen
- **THEN** the same connection is made and the match proceeds identically

### Requirement: The joiner reviews the terms before the game starts

Before a new LAN game begins, the joiner SHALL be shown what the host chose: the color the joiner will play — shown as "Random" when the host chose random, before any roll — and the time control, or that the game is untimed. Below the terms, the joiner SHALL choose between agreeing to play and cancelling.

Cancelling SHALL disconnect and return the joiner to the menu; the host SHALL return to waiting, with the match still open under the same ID.

The game SHALL begin on both machines only when the joiner agrees.

#### Scenario: Terms shown
- **WHEN** the joiner's machine connects to a new match
- **THEN** the joiner sees the color they would play and the time control before anything starts

#### Scenario: Random is shown as random
- **WHEN** the host chose to play a random color
- **THEN** the joiner's terms say the colors are random, not a color picked in advance

#### Scenario: Agreeing
- **WHEN** the joiner chooses to agree
- **THEN** the game starts on both machines under exactly the terms shown

#### Scenario: Declining
- **WHEN** the joiner cancels at the terms
- **THEN** the joiner is back at the menu, and the host is waiting again with the same ID

### Requirement: Colors are assigned by the host's choice

The host SHALL play the color they chose on the setup screen; the joiner SHALL play the other. When the host chose random, the color SHALL be decided only after the joiner agrees, and each machine SHALL then tell its player which color they received.

#### Scenario: A chosen color
- **WHEN** the host chose White and the joiner agrees
- **THEN** the host plays White and the joiner plays Black

#### Scenario: A random color
- **WHEN** the host chose Random and the joiner agrees
- **THEN** each side is told which color they play before the first move, and over many games neither assignment is favoured

### Requirement: Incompatible programs refuse to play each other

When the two programs cannot correctly play the same game — because they speak different versions of the match protocol — the match SHALL be refused at connection with a message on both screens saying so, rather than started and allowed to go wrong.

#### Scenario: Version mismatch
- **WHEN** a joiner connects to a host speaking an incompatible protocol version
- **THEN** no game starts, and each side is told the two programs are incompatible and which side is older
