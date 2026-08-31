## ADDED Requirements

### Requirement: Screens are notified that time has passed

The system SHALL be able to notify screens that time has passed, without any input having occurred. This notification is not input: it says nothing about what the user did, and a screen that ignores it SHALL behave exactly as it does today.

Unlike input, this notification SHALL reach screens beneath an overlay as well as the active screen. A game covered by a confirmation is still a game whose clock is running; delivering time only to the top of the stack would stop time for the very screen that most needs it.

A screen SHALL declare whether it wants this notification. A screen that does not declare it SHALL NOT receive it, so no existing screen acquires new behaviour by the mechanism existing.

The notification SHALL NOT change the stack: a screen may update what it knows and what it draws, but SHALL NOT be moved, pushed, popped or replaced as a result. A screen whose state changed such that it should be left SHALL express that on the next event it handles, the way a screen that discovers its game has ended under an overlay already does.

#### Scenario: Time reaches the active screen
- **WHEN** time passes and the active screen wants the notification
- **THEN** it is notified, and the next frame reflects whatever it changed

#### Scenario: Time reaches a covered screen
- **WHEN** an overlay is active over a screen that wants the notification
- **THEN** the covered screen is notified too, even though it receives no input

#### Scenario: Screens that do not want it
- **WHEN** time passes and a screen has not declared that it wants the notification
- **THEN** it is not notified and nothing about it changes

#### Scenario: Input is still exclusive
- **WHEN** an overlay is active and the user presses a key
- **THEN** only the overlay sees the key, exactly as before — the time notification changes nothing about input delivery

#### Scenario: The stack is not moved
- **WHEN** a screen is notified that time has passed
- **THEN** no screen is pushed, popped or replaced as a direct result

### Requirement: The system wakes when a screen needs it to

A screen SHALL be able to say how soon it needs to be notified next, and the system SHALL wait no longer than the soonest such request before drawing the next frame — even if nothing at all is typed.

When no screen on the stack asks to be woken, the system SHALL wait for input with no deadline, doing no work and consuming no processor time while idle.

#### Scenario: A screen asks to be woken
- **WHEN** a screen on the stack asks to be notified within a given interval
- **THEN** the next frame is drawn no later than that, with or without input

#### Scenario: The soonest request wins
- **WHEN** two screens on the stack ask for different intervals
- **THEN** the shorter is used

#### Scenario: Nothing to wake for
- **WHEN** no screen asks to be woken
- **THEN** the system blocks on input and draws no frame until something happens
