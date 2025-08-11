#include "Board.h"

void Board::placeTile(int x, int y, const Tile& tile) {
    tiles[{x, y}] = tile;
}

bool Board::isOccupied(int x, int y) const {
    return tiles.find({x, y}) != tiles.end();
}
