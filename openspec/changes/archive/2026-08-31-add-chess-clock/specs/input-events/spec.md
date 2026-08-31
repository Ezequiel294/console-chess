## ADDED Requirements

### Requirement: The wait for input is bounded on request

The caller SHALL be able to ask for the next event with a deadline: either no deadline, in which case the system waits as long as it takes, or a bounded wait after which the system reports that nothing arrived.

Expiry SHALL be reported as its own event type, distinguishable from every other, so that "time passed" and "the user did something" are never confused for one another.

A deadline SHALL NOT corrupt input that is mid-arrival. An escape sequence, a multi-byte character, or a paste that has begun but not finished SHALL be resumed on the next call rather than truncated, discarded, or emitted in pieces — the same guarantee a resize already carries.

A deadline SHALL NOT be exceeded by an amount a person would notice: waiting for input MUST NOT delay an expiry past its deadline while an unrelated partial sequence sits in the buffer.

#### Scenario: No deadline
- **WHEN** the next event is asked for with no deadline
- **THEN** the system waits until an event is available, as it always has

#### Scenario: Deadline expires
- **WHEN** the next event is asked for with a deadline and nothing is typed
- **THEN** an expiry event is delivered once the deadline passes

#### Scenario: Input before the deadline
- **WHEN** a key is pressed before the deadline
- **THEN** the key event is delivered and no expiry event is

#### Scenario: Expiry is not a keystroke
- **WHEN** a deadline expires
- **THEN** no key, mouse, paste, or resize event is produced by it

#### Scenario: A sequence in progress survives the deadline
- **WHEN** a deadline expires while part of an escape sequence has arrived and the rest has not
- **THEN** the bytes already read are kept and the sequence completes on a later call, rather than being emitted as key input or dropped

#### Scenario: Expiry is not delayed by a partial sequence
- **WHEN** a deadline is reached while an incomplete sequence sits unread
- **THEN** the expiry is still delivered at its deadline
