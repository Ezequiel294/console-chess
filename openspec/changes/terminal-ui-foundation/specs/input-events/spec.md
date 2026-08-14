## Purpose

Turns the raw byte stream arriving from the terminal into typed events the application can reason about, and guarantees that anything it does not understand is discarded rather than leaked into the application as spurious input.

## ADDED Requirements

### Requirement: Single input source

Exactly one component SHALL read from standard input. No other part of the system may read it, whether directly or through buffered library calls.

Two readers on one descriptor each hold their own buffer, so bytes are intermittently lost or reordered. This is a correctness requirement, not a stylistic one.

#### Scenario: Rapid input
- **WHEN** the user types faster than frames are drawn
- **THEN** every keystroke is delivered exactly once, in order

### Requirement: Typed events

Input SHALL be delivered to the application as typed events, never as raw bytes. The event types are: key press, mouse action, terminal resize, paste, and end-of-input.

A mouse event SHALL carry its column, row, button, and whether it is a press, release, motion, or wheel movement. Wheel movement SHALL be reported as a wheel event with a direction, not as a button press.

#### Scenario: Character key
- **WHEN** the user presses `e`
- **THEN** a key event for `e` is delivered

#### Scenario: Named key
- **WHEN** the user presses an arrow key, Enter, Escape, Backspace, Tab, or a function key
- **THEN** a key event identifying that key is delivered, distinct from any character key

#### Scenario: Mouse click
- **WHEN** the user clicks at a position on screen
- **THEN** a mouse event carrying that column and row is delivered

#### Scenario: Wheel movement
- **WHEN** the user scrolls the wheel
- **THEN** a wheel event with a direction is delivered
- **AND** no key event is delivered

### Requirement: Unknown sequences are discarded

The system SHALL consume every escape sequence in full and SHALL discard any it does not recognize. An unrecognized sequence MUST NOT produce an event of any kind.

This is what makes stray terminal output structurally incapable of reaching game logic.

#### Scenario: Unrecognized control sequence
- **WHEN** the terminal emits an escape sequence the system does not handle
- **THEN** the entire sequence is consumed
- **AND** no event is produced
- **AND** no fragment of the sequence appears as key input

#### Scenario: Truncated sequence
- **WHEN** an escape sequence arrives incomplete and no further bytes follow
- **THEN** the system does not block indefinitely and does not emit the partial sequence as key input

### Requirement: Escape key disambiguation

A lone Escape key press SHALL be distinguished from the start of an escape sequence, using a bounded wait for following bytes.

#### Scenario: User presses Escape
- **WHEN** the user presses Escape and types nothing further
- **THEN** an Escape key event is delivered within a short, bounded delay

#### Scenario: Sequence beginning with escape
- **WHEN** an escape byte is immediately followed by the rest of a recognized sequence
- **THEN** the sequence is decoded as its own event and no Escape key event is delivered

### Requirement: Paste handling

Pasted text SHALL be delivered as a single paste event carrying its content, or discarded, but SHALL NOT be delivered as a series of key events.

#### Scenario: User pastes multiple lines
- **WHEN** the user pastes text containing newlines
- **THEN** no key events are produced for the pasted characters
- **AND** the game state is unchanged unless the receiving screen chooses to act on the paste event

### Requirement: Resize delivery

A terminal size change SHALL be delivered as a resize event that interrupts a pending wait for input.

#### Scenario: Resize while blocked
- **WHEN** the terminal is resized while the system is waiting for input
- **THEN** a resize event is delivered without requiring a keystroke
- **AND** any partially read escape sequence is not corrupted by the interruption
