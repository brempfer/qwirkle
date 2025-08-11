#pragma once
#include "Tile.h"
#include <map>
#include <utility>

using Coord = std::pair<int, int>;

class Board {
public:
    void placeTile(int x, int y, const Tile& tile);
    const std::map<Coord, Tile>& getTiles() const { return tiles; }
    bool isOccupied(int x, int y) const;

private:
    std::map<Coord, Tile> tiles; // sparse storage
};
