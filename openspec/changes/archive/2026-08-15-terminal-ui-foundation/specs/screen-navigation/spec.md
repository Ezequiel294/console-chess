## Purpose

Organizes the interface into independent screens held on a stack, so that overlays can appear over what is beneath them and dismiss back to exactly where the user was, and so that each screen can be written without knowledge of any other.

## ADDED Requirements

### Requirement: Screens are held on a stack

The interface SHALL be organized as a stack of screens. The screen on top is the active screen.

Pushing a screen SHALL preserve the state of the screens beneath it. Popping SHALL return to the screen below with its state intact.

#### Scenario: Push and pop
- **WHEN** a screen is pushed over the current one and later dismissed
- **THEN** the underlying screen resumes with exactly the state it had before

#### Scenario: State preserved beneath
- **WHEN** a screen is pushed while the game has a piece selected
- **AND** the pushed screen is later dismissed
- **THEN** the same piece is still selected

### Requirement: Only the active screen receives input

Input events SHALL be delivered only to the screen on top of the stack. Screens beneath SHALL NOT receive input while covered.

#### Scenario: Input while covered
- **WHEN** an overlay is active and the user presses a key the underlying screen would act on
- **THEN** only the overlay sees the key
- **AND** the underlying screen's state does not change

### Requirement: Overlays composite over what is beneath

Each screen SHALL declare whether it covers the display entirely or is an overlay. Rendering SHALL begin at the topmost fully covering screen and draw each screen above it in order.

An overlay SHALL be visible together with the screen beneath it.

#### Scenario: Overlay over the board
- **WHEN** an overlay is active over the game
- **THEN** the board remains visible around and behind the overlay

#### Scenario: Full screen hides what is below
- **WHEN** a fully covering screen is active
- **THEN** no part of any screen beneath it is drawn

### Requirement: Screens request transitions rather than performing them

A screen SHALL request a navigation change by returning it from its input handling. A screen SHALL NOT modify the stack directly.

Requested transitions SHALL be applied after the requesting screen has finished handling the event, and before the next frame is drawn.

The available requests are: no change, push a screen, pop the active screen, replace the active screen, and quit.

#### Scenario: Screen dismisses itself
- **WHEN** the active screen returns a pop request
- **THEN** it finishes handling the current event before being removed
- **AND** it receives no further events afterwards

#### Scenario: Quit request
- **WHEN** any screen returns a quit request
- **THEN** the application shuts down cleanly, restoring the terminal

### Requirement: Screens receive their region each frame

Each screen SHALL be given the region it may draw into as part of every render, and SHALL NOT retain size information between frames.

This makes every screen correct across terminal resizes without per-screen handling.

#### Scenario: Resize with a screen active
- **WHEN** the terminal is resized
- **THEN** the active screen is rendered into the new region on the next frame
- **AND** it requires no resize-specific handling of its own

#### Scenario: Resize with an overlay active
- **WHEN** the terminal is resized while an overlay is shown
- **THEN** both the overlay and the screen beneath it are laid out for the new size

### Requirement: Initial screen

On startup the system SHALL push the game screen as the initial screen.

#### Scenario: Launching the game
- **WHEN** the program starts
- **THEN** the game board is displayed and ready for play

### Requirement: Screen lifecycle notifications

A screen SHALL be notified when it becomes part of the stack and when it is removed, so it can acquire and release whatever it owns.

#### Scenario: Screen removed
- **WHEN** a screen is popped
- **THEN** it is notified before removal
- **AND** any resources it acquired are released
