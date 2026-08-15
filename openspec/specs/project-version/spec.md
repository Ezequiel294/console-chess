# project-version Specification

## Purpose

Gives every build a single declared identity, so that a running program, a bug report, and a file on disk can each be traced back to the exact release that produced them.

## Requirements

### Requirement: A single declared version

The project SHALL declare its version in exactly one place. Every other place the version appears SHALL derive from that declaration rather than restating it.

The declared version SHALL be a plain text version string of the form `MAJOR.MINOR.PATCH`, optionally followed by a pre-release suffix.

#### Scenario: One source of truth
- **WHEN** the declared version is changed
- **THEN** every surface that reports a version reports the new one
- **AND** no surface has to be edited separately for them to agree

#### Scenario: Version is readable without building
- **WHEN** someone inspects the source tree without compiling it
- **THEN** the declared version is legible as plain text

### Requirement: A build cannot misreport its version

A build SHALL fail when the declared version is missing, empty, or not a well-formed version string. It SHALL NOT produce a program that reports an absent, blank, or placeholder version.

The failure SHALL name what is wrong with the declaration.

#### Scenario: Declaration is missing
- **WHEN** a build is attempted with no version declared
- **THEN** the build fails with a message saying the declaration is missing
- **AND** no program is produced

#### Scenario: Declaration is empty or blank
- **WHEN** a build is attempted and the declaration contains no version string
- **THEN** the build fails with a message saying so
- **AND** no program is produced

#### Scenario: Declaration is malformed
- **WHEN** a build is attempted and the declared version is not a well-formed version string
- **THEN** the build fails, naming the offending text
- **AND** no program is produced

#### Scenario: Declaration is valid
- **WHEN** a build is attempted with a well-formed version declared
- **THEN** the build succeeds and the resulting program reports that version

### Requirement: The version can be asked for and the program exits

The program SHALL accept an option that reports its name and version and then exits successfully, without starting a game, drawing a board, reading a save file, or waiting for input.

Both a long and a short form of the option SHALL be accepted.

#### Scenario: Long form
- **WHEN** the program is run with the long version option
- **THEN** it prints its name and version, exits successfully, and starts no game

#### Scenario: Short form
- **WHEN** the program is run with the short version option
- **THEN** it behaves identically to the long form

#### Scenario: Reported version matches the declaration
- **WHEN** the program reports its version
- **THEN** the version reported is the one declared for the build it was compiled from

#### Scenario: Usable when redirected
- **WHEN** the version option is used with output redirected to a file or a pipe
- **THEN** the version is written there, so it can be captured by a script

### Requirement: Usage is discoverable

The program SHALL accept an option that prints usage — its name, how it is invoked, and every option it accepts including the version option — and then exits successfully without starting a game.

An unrecognised option SHALL print usage and exit with a failure status, so that a mistyped option is not silently ignored.

#### Scenario: Help requested
- **WHEN** the program is run with the help option
- **THEN** it prints usage listing every accepted option, exits successfully, and starts no game

#### Scenario: Help lists the version option
- **WHEN** usage is printed
- **THEN** the version option appears in it

#### Scenario: Unrecognised option
- **WHEN** the program is run with an option it does not accept
- **THEN** it prints usage, exits with a failure status, and starts no game

#### Scenario: No options
- **WHEN** the program is run with no options
- **THEN** it starts a game as before

### Requirement: The version is visible while playing

The program SHALL show its version on the screen a player sees when it starts, so that the version can be reported without knowing that any command-line option exists.

#### Scenario: Version shown at startup
- **WHEN** the program starts normally
- **THEN** the version is visible on the opening screen

#### Scenario: Startup version matches the reported one
- **WHEN** the version shown at startup is compared with the version option's output
- **THEN** they are the same version

### Requirement: A saved game records the build that wrote it

A saved game SHALL record the version of the build that wrote it. The recorded version SHALL be readable from the file when the game is loaded, so that a reported problem with a save can be traced to a build.

A version that does not match the running build SHALL NOT by itself cause a save to be rejected. Whether a save can be loaded is determined by its format, not by which release wrote it.

#### Scenario: Version is recorded
- **WHEN** a game is saved
- **THEN** the file records the version of the build that wrote it

#### Scenario: Save from a different release still loads
- **WHEN** a save written by a different release is loaded, and its format is one the running build understands
- **THEN** it loads normally

#### Scenario: Format compatibility is unaffected
- **WHEN** the declared version changes but the layout of the saved game does not
- **THEN** saves written before the change still load

#### Scenario: Format change still rejects
- **WHEN** the layout of the saved game changes
- **THEN** saves written under the previous layout are refused, regardless of which release wrote them

### Requirement: The release procedure is documented

The project's documentation SHALL state the current version and describe how to make a release, including where the version is declared and what else a release requires.

#### Scenario: Procedure is written down
- **WHEN** someone wants to cut a release
- **THEN** the documentation tells them where to change the version and what else to do

#### Scenario: Documented version is current
- **WHEN** the documented version is compared with the declared one
- **THEN** they agree
