## 1. Enable mouse reporting

- [x] 1.1 Add `?1000h` (button tracking) and `?1006h` (SGR coordinates) to the terminal entry sequence in `term.c`
- [x] 1.2 Add the matching disables to the restore path, and verify they run on all four exit paths including a crash
- [x] 1.3 Confirm SGR is used rather than the legacy encoding, which cannot express columns beyond 223
- [x] 1.4 Verify mouse events reach the event loop on each terminal being targeted

## 2. Hit testing

- [x] 2.1 Create `interaction.c/h`
- [x] 2.2 Expose the board's origin and cell size from the layout the renderer already computes — one source, never recomputed
- [x] 2.3 `point_to_square(layout, col, row)` returning a square or none
- [x] 2.4 Account for board orientation, so a flipped board maps to what is drawn rather than its mirror
- [x] 2.5 Take cell width from the glyph-width probe rather than a constant
- [x] 2.6 Return none for clicks outside the board — never clamp to the nearest square
- [x] 2.7 Test all four corners of a1 and h8, at both glyph widths and both orientations

## 3. Selection state machine

- [x] 3.1 Add selection state to the Game screen's context: nothing selected, piece selected, awaiting promotion
- [x] 3.2 Define one `SelectSquare` event produced by clicks, typed coordinates, and keyboard cursor plus Enter alike
- [x] 3.3 Nothing selected: select only a piece belonging to the side to move; ignore empty and opposing squares
- [x] 3.4 Piece selected + legal destination: make the move, clear the selection
- [x] 3.5 Piece selected + another own piece: move the selection
- [x] 3.6 Piece selected + illegal square: keep the selection, indicate rejection
- [x] 3.7 Escape, or naming the selected square again, cancels the selection
- [x] 3.8 Route a promotion destination through the `Promotion` overlay from `chess-rules-engine` before completing
- [x] 3.9 Verify a move is accepted only if it appears in the generator's legal move list

## 4. Keyboard cursor

- [x] 4.1 A cursor square movable with arrow keys, Enter to name it
- [x] 4.2 Route it through the same `SelectSquare` event as clicks
- [x] 4.3 Keep typed coordinate entry working exactly as before
- [x] 4.4 Draw the cursor distinctly from the selection

## 5. Affordance rendering

- [x] 5.1 Extend the board widget with a per-square tint and a per-square overlay mark
- [x] 5.2 Query `generate_legal_moves_from` each frame while a piece is selected — no caching
- [x] 5.3 Centered dot on empty legal destinations
- [x] 5.4 Distinct tint for legal destinations holding an opposing piece
- [x] 5.5 Mark en passant destinations as captures even though the square is empty
- [x] 5.6 Tint the selected square distinctly
- [x] 5.7 Persist the last-move tint on origin and destination until the next move, surviving flips and resizes
- [x] 5.8 Mark a king's square while its side is in check, and leave it marked in a final checkmate position
- [x] 5.9 Ensure no marking changes square size or alignment, and none hides the piece on the square
- [x] 5.10 Verify castling appears as a king destination with no special input

## 6. Colour and fallback

- [x] 6.1 Define the tint palette in one place
- [x] 6.2 Give every marking a shape or character fallback so it survives without colour
- [x] 6.3 Detect colour support and select the fallback automatically
- [x] 6.4 Verify all five marking states stay distinguishable in monochrome and in ASCII mode

## 7. Wheel handling

- [x] 7.1 Consume and discard wheel events on the Game screen, with a comment naming why
- [x] 7.2 Verify scrolling during play changes nothing on screen and produces no input

## 8. Verification

- [x] 8.1 Play a full game using only the mouse
- [x] 8.2 Play a full game without touching the mouse, confirming castling and promotion are reachable
- [x] 8.3 Play a game mixing clicks and typed coordinates within single moves
- [x] 8.4 Confirm marked destinations exactly match the generator's output for pinned pieces and while in check
- [x] 8.5 Resize and change font size mid-selection; confirm hit testing stays correct afterwards
- [x] 8.6 Update `README.md`: mouse usage, and the note that mouse tracking affects terminal text selection
