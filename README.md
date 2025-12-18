# Qt Hearts

A classic Hearts card game built with Qt6.

## Features

- Single player against 3 AI opponents
- Three AI difficulty levels (Easy, Medium, Hard)
- Card passing phases (left, right, across, hold)
- Undo support
- Sound effects
- Customizable card themes (compatible with KDE carddecks)
- Statistics tracking

## Building

### Requirements

- Qt 6.x (Widgets, SvgWidgets, Multimedia)
- CMake 3.16+ or QMake

### Build with CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Build with QMake

```bash
qmake6 qt-hearts.pro
make
```

## Installation

### Arch Linux

```bash
makepkg -si
```

### Manual

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

## Rules

- Avoid taking hearts (1 point each) and the Queen of Spades (13 points)
- Lowest score wins when any player reaches 100 points
- "Shoot the Moon": Take all hearts and the Queen of Spades to give 26 points to all opponents

## License

GPL
