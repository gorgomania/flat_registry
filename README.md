# Apartment Registry

A console application for managing an apartment registry with Russian language support. Runs in a Linux / WSL terminal.

## Features

- Store apartment records: owner's full name, street, house and flat numbers, area, number of residents
- Navigate the list with arrow keys and paginated view
- Sort records by owner's surname (ascending and descending)
- Group apartments by street
- Search records by street name
- Save and load the database in binary and text formats
- Full UTF-8 support (Cyrillic)

## Build

```bash
g++ -o flat_registry main.cpp
```

## Run

```bash
./flat_registry
```

For interactive testing via tmux:

```bash
tmux new-session -d -s reg -x 120 -y 40 './flat_registry'
tmux attach -t reg
```

## Controls

| Key          | Action                      |
|--------------|-----------------------------|
| `↑` / `↓`    | Navigate menu and list      |
| `Enter`      | Confirm selection           |
| `Escape`     | Cancel input                |
| `Backspace`  | Delete character            |

## File formats

**Binary** (`.bin`) — fast loading, stores records at a fixed size (104 bytes/record).

**Text** (`.txt`) — human-readable format with aligned columns, supports UTF-8 headers.

Test data: `test.bin` (12 records, 3 streets: Lenina, Mira, Sadovaya).

## Requirements

- Linux or WSL2
- GCC with C++11 or later
- Terminal with UTF-8 and ANSI escape code support
