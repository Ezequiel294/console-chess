# Console Chess
## Overview
This project aims to create a console-based chess game that implements basic chess rules and game mechanics. When launched, the program will display a chess board showing all pieces in their starting positions, followed by a simple menu interface below the board for user interaction.
## Menu System
The game starts by presenting players with a numbered menu offering two main options: "Start New Game" and "Load Game". Players make their selection by entering the corresponding menu number. The load game option will only work if there's an existing binary save file in the same directory as the game executable. If no save file exists, the game will display an error message and return to the main menu.

![image](https://github.com/user-attachments/assets/1e768a02-edd8-4d7a-97b8-1ef2fe875103)

If time permits, selecting "New Game" will lead to a secondary menu where players can choose between playing against an AI opponent or entering a two-player mode where both players share the same computer (At the moment, only the 2-player mode is intended). In the case of choosing AI mode, one final prompt will ask the player to select whether they want to play as white or black pieces before beginning the game.
## Game Mechanics
The core gameplay centers around a piece movement system where players specify their moves through text commands. Players identify pieces using a notation system - for example, "kn_c3" represents a knight at position c3 and 'kn' stands for knight. After entering the piece identifier and pressing enter, the game will prompt for the desired destination square. Another option is that with a single console input, the piece and the destination can be specified in the same line using an arrow like this: “kn_c3->d5”, but this would take more string processing.
The game will implement fundamental piece movement validation to ensure all moves follow chess rules. If a player attempts an invalid move, they'll be prompted to try again until they input a valid move. The game will maintain a history of moves, particularly important for special moves like castling that depend on whether pieces have moved previously.
For the initial version, the winning condition will be simplified to capturing the king, rather than implementing the more complex checkmate detection system. While this deviates from official chess rules, it provides a more manageable scope for the project's first iteration.
## Display System
The game board will be displayed using text characters in the terminal, with pieces represented by their text notation (like w_kn for white knight). While using actual chess piece icons in the terminal presents some challenges, there's potential to explore using Nerd Fonts for improved piece visualization. However, piece notation will still be needed internally and for the players to indicate the desired move.

![image](https://github.com/user-attachments/assets/d86e7b46-2c01-4321-aa60-dac096b15d99)

*Chess nerd fonts. would need a console with nerd fonts too.*
