## Purpose

Owns the terminal as a resource: putting it into the mode a full-screen application needs, reporting its size and size changes, measuring how it renders piece glyphs, and guaranteeing it is restored to its original state no matter how the program ends.

## ADDED Requirements

### Requirement: Full-screen terminal mode

On startup the system SHALL place the terminal into a mode suitable for a full-screen application: character-at-a-time input with no echo, the alternate screen buffer active, the cursor hidden, alternate scroll mode disabled, and bracketed paste enabled.

Alternate scroll mode MUST be disabled. While it is active the terminal translates wheel motion into cursor-key sequences, which are indistinguishable from keyboard input.

#### Scenario: Entering the game
- **WHEN** the program starts on an interactive terminal
- **THEN** the screen is replaced by the application's own display
- **AND** no cursor is visible
- **AND** typed characters are delivered to the program without being echoed and without waiting for Enter

#### Scenario: Scrolling does not produce input
- **WHEN** the user scrolls the mouse wheel while the application is running
- **THEN** no keyboard input is delivered to the application as a result

#### Scenario: Pasting does not produce input
- **WHEN** the user pastes text into the terminal
- **THEN** the pasted characters are not interpreted as individual keystrokes

### Requirement: Guaranteed terminal restoration

The system SHALL restore the terminal to its pre-launch state on every exit path: normal termination, interruption, termination signal, and fatal fault. Restoration SHALL undo every mode change: input mode, alternate screen, cursor visibility, scroll mode, paste mode, and mouse reporting.

#### Scenario: Normal exit
- **WHEN** the user quits the game
- **THEN** the terminal returns to the content and state it had before launch
- **AND** the shell behaves normally

#### Scenario: Interrupt
- **WHEN** the user presses Ctrl-C
- **THEN** the terminal is restored before the program exits

#### Scenario: Crash
- **WHEN** the program terminates on a fatal fault
- **THEN** the terminal is still restored, leaving a usable shell

### Requirement: Terminal size awareness

The system SHALL determine the terminal's size in character cells at startup and SHALL detect subsequent changes, including those caused by changing the font size rather than the window dimensions.

A size change SHALL take visible effect without requiring further user input.

#### Scenario: Window resized
- **WHEN** the user resizes the terminal window
- **THEN** the display is laid out for the new size

#### Scenario: Font size changed mid-game
- **WHEN** the user increases the terminal font size during play
- **THEN** the display is laid out for the resulting smaller cell grid
- **AND** no partial or wrapped output from the previous layout remains on screen

#### Scenario: Resize while idle
- **WHEN** the terminal is resized while the program is waiting for input
- **THEN** the display updates immediately, without waiting for a key to be pressed

### Requirement: Glyph width measurement

At startup the system SHALL measure how many cells the terminal uses to render the game's piece glyphs, rather than assuming a width.

The piece icons occupy Private Use Area codepoints, which the C library reports as one cell wide while many terminals render two. An unmeasured assumption misaligns every board row.

#### Scenario: Single-width terminal
- **WHEN** the terminal renders a piece glyph in one cell
- **THEN** the measured width is 1 and board columns align

#### Scenario: Double-width terminal
- **WHEN** the terminal renders a piece glyph in two cells
- **THEN** the measured width is 2 and board columns align

#### Scenario: Measurement unavailable
- **WHEN** the terminal does not answer the measurement query within a bounded time
- **THEN** the system proceeds with a documented default width rather than hanging

### Requirement: Non-interactive terminal refusal

When standard input or standard output is not an interactive terminal, the system SHALL report that a terminal is required and exit without modifying any terminal state.

#### Scenario: Output redirected to a file
- **WHEN** the program is started with its output redirected
- **THEN** it prints an explanatory message and exits non-zero
- **AND** no escape sequences are written to the redirected output
