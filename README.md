# Console Chess ♟️

A two-player chess game that runs entirely in the terminal

## Requirements

- **C Compiler**: You need a C compiler such as `gcc` installed on your system.
- **Nerd Font**: The terminal must support chess piece icons. Install a [Nerd Font](https://www.nerdfonts.com/) (e.g., JetBrains Mono Nerd Font) and set it as the terminal's font.

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
- **Turn order**: White pieces play first, then Black. Turns alternate after each move.
- **Winning**: The game ends when a King is captured.

### Save and Load

- **Saving**: Players are prompted every 5 moves if they want to save the game. The game state is saved to `game_save.bin`.
- **Loading**: Select "Load Game" from the main menu when starting the program. The game will resume from where you left off.

## Notes

- Ensure your terminal supports Unicode and that the Nerd Font is correctly configured to display the chess piece icons.
- Check and checkmate are not enforced — the game ends when a King is actually captured.
