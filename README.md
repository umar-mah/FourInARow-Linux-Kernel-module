# Connect Four Game as a Linux Kernel Module

## Overview

FourInARow is a Linux kernel module that implements a virtual character device (/dev/fourinarow) to simulate the Connect Four game. This device allows users to play against a computer opponent using read and write operations from userspace. The computer player selects moves randomly, while the kernel module handles game logic, board state, and win/tie detection.

## Project Structure

- **project-base/**: Linux kernel environment.
- **KernelGame/**: Contains the `fourinarow` kernel module source code and related files.

## Features

- Linux Kernel Module named fourinarow.ko
- Character device interface: /dev/fourinarow
- Fully working 8x8 Connect Four game against a computer
- Kernel-space game logic with input/output via read() and write()
- Win and tie detection

## Implementation

- The device node uses chmod 666 to provide read and write permissions for all users.
- All supported module commands are implemented in the `dev_write` function.
- The `DROPC` command allows the user to drop their piece into the specified column. It places the piece in the lowest available row within that column.
- The `CTURN` command makes the computer perform a random move, dropping its piece into a randomly chosen column that has space.
- The `checkWin` function checks for a winning condition by scanning horizontally, vertically, and diagonally. It is invoked after each `DROPC` and `CTURN` move.
- The `checkTie` function verifies if the board is full and no winner has been determined, resulting in a tie.
- The `RESET` command must be used to reset the board and to select the user’s playing color ('Y' for yellow or 'R' for red).
- Module initialization involves setting up the kernel module, registering the character device, and creating the corresponding device class and device node

## Module Commands

- Interact with the game using write() to /dev/fourinarow, and reads responses via read().

### `RESET $`

- Initializes or resets the game.
- `$` is either `R` (Red) or `Y` (Yellow) – the user's color.
- Response: `OK`

### `BOARD`

- Returns the current board state.
- Response: Board grid

### `DROPC $`

- Drops user's chip in column `$` (A–H).
- Responses:
  - `OK`: Move accepted
  - `OOT`: Out of turn
  - `NOGAME`: No active game
  - `WIN`: You won
  - `TIE`: Tie game

### `CTURN`

- Requests the computer to make its move.
- Responses:
  - `OK`: Computer moved
  - `OOT`: Not computer’s turn
  - `NOGAME`: No active game
  - `LOSE`: Computer won
  - `TIE`: Tie game

## File Structure

- `fourinarow.ko`: Compiled kernel module
- `/dev/fourinarow`: Character device interface
- `fourinarow.c`: Source code implementing module functionality

## Building & Loading the Module

```
make
sudo insmod fourinarow.ko
```

## Sample Usage

```
# Start a new game as Yellow
echo "RESET Y" >> /dev/fourinarow
cat /dev/fourinarow  # -> OK

# Drop a chip in column B
echo "DROPC B" >> /dev/fourinarow
cat /dev/fourinarow  # -> OK, WIN, or TIE

# Let computer take a turn
echo "CTURN" >> /dev/fourinarow
cat /dev/fourinarow  # -> OK, LOSE, or TIE

# Check board state
echo "BOARD" >> /dev/fourinarow
cat /dev/fourinarow # -> Board Grid
```

## Removing Module

```
sudo rmmod fourinarow
make clean
```
