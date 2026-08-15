## Purpose

Composes each frame of the display into an off-screen grid of character cells and sends only what changed to the terminal, so that output is always positioned, always fits, and never flickers.

## ADDED Requirements

### Requirement: Frames are composed before display

The system SHALL compose a complete frame off-screen and then update the terminal to match it. Output SHALL NOT be produced by printing progressively to the terminal as the display is built.

#### Scenario: A frame is drawn
- **WHEN** the display needs updating
- **THEN** the terminal shows the new frame in full
- **AND** no intermediate or partially drawn state is visible

#### Scenario: Small change between frames
- **WHEN** one square of the board changes between frames
- **THEN** the rest of the display is not redrawn or cleared

#### Scenario: Repeated identical frames
- **WHEN** a frame is composed that is identical to the one on screen
- **THEN** the terminal display is left untouched

### Requirement: Content is clipped, never wrapped

Every element SHALL be drawn within a rectangle assigned to it. Content that does not fit SHALL be truncated at the rectangle's edge.

Content MUST NOT wrap to the following line, and MUST NOT be drawn outside its assigned rectangle.

#### Scenario: Text longer than its region
- **WHEN** a status message is longer than the width available to it
- **THEN** it is cut off at the edge of its region
- **AND** the remainder does not appear on the next line
- **AND** no other element is overwritten

#### Scenario: Narrow terminal
- **WHEN** the terminal is narrower than the content would naturally require
- **THEN** no line of output exceeds the terminal width
- **AND** the terminal never wraps a line on the application's behalf

### Requirement: Layout follows current size

Layout SHALL be computed from the terminal's current size for every frame. No element may retain a size from a previous frame.

#### Scenario: Resize between frames
- **WHEN** the terminal size changes
- **THEN** the next frame is laid out for the new size in full
- **AND** no element retains its previous dimensions

### Requirement: Insufficient space is reported

When the terminal is too small to display the game, the system SHALL replace the display with a message stating the required size and the current size, and SHALL restore the normal display when enough space becomes available.

#### Scenario: Terminal shrunk below minimum
- **WHEN** the terminal is resized smaller than the game requires
- **THEN** the display is replaced by a message giving the required and current sizes
- **AND** no clipped or misaligned fragment of the game remains visible

#### Scenario: Space restored
- **WHEN** the terminal is enlarged back to at least the required size
- **THEN** the game display returns, with game state unchanged

### Requirement: Piece rendering with fallback

Pieces SHALL be rendered using the configured glyph set, and the system SHALL provide a plain-ASCII alternative selectable at launch for terminals without the required font.

Glyph placement SHALL respect the measured glyph width from `terminal-runtime`, so board columns align on terminals that render the icons at either width.

#### Scenario: Default glyphs
- **WHEN** the game runs with the default glyph set on a terminal with the required font
- **THEN** each piece is drawn with its icon
- **AND** all board columns align

#### Scenario: ASCII fallback selected
- **WHEN** the user launches the game in ASCII mode
- **THEN** pieces are drawn as letters, with case or another visible marker distinguishing the two colors
- **AND** all board columns align

#### Scenario: Double-width glyph terminal
- **WHEN** the terminal renders piece icons two cells wide
- **THEN** squares are sized accordingly and the board remains aligned
