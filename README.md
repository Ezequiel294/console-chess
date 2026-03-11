# Console Chess ♟️

A two-player chess game that runs entirely in the terminal, featuring both keyboard and mouse input.

## Requirements

- **C Compiler**: You need a C compiler such as `gcc` installed on your system.
- **Nerd Font**: The terminal must support chess piece icons. Install a [Nerd Font](https://www.nerdfonts.com/) (e.g., JetBrains Mono Nerd Font) and set it as the terminal's font.
- **Terminal with mouse support**: For mouse click input, your terminal emulator must support xterm SGR mouse reporting (most modern terminals do, such as Alacritty, Kitty, GNOME Terminal, Windows Terminal via WSL, etc.).

## Compile and Run

1. Open a terminal with a Nerd Font set as the default font.
2. Navigate to the directory containing `main.c`.
3. Compile the program:
   ```
   gcc -o chess_game main.c
   ```
4. Run the game:
   ```
   ./chess_game
   ```

## How to Play

- **Keyboard input**: Enter moves using chess coordinates, letter first (e.g., `e2` to select/move to square e2).
- **Mouse input**: Click directly on a board square to select a piece or choose a destination — no typing needed!
- **Turn order**: White pieces play first, then Black. Turns alternate after each move.
- **Winning**: The game ends when a King is captured.

### In-Game Commands

At any move prompt, you can type the following commands instead of a coordinate:

| Command   | Description                                      |
|-----------|--------------------------------------------------|
| `save`    | Save the current game to a file and exit.        |
| `history` | View the full move history, then press Enter to return to the game. |

### Save and Load

- **Saving**: Type `save` at any move prompt. The game state is saved to `game_save.bin`.
- **Loading**: Select "Load Game" from the main menu when starting the program. The game will resume from where you left off.

## Notes

- Ensure your terminal supports Unicode and that the Nerd Font is correctly configured to display the chess piece icons.
- Mouse click selection works by mapping your click position to the board grid. If icons render as double-width in your terminal, click accuracy may vary — use keyboard input as a fallback.
- Special moves such as **castling**, **en passant**, and **pawn promotion** are not supported yet.
