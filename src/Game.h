#pragma once

#include "Board.h"
#include <SFML/Graphics.hpp>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <vector>

class Game {
public:
    Game() = default;
    void run();

private:
    Board board;

    // Textures for drawing tiles
    std::map<std::pair<Shape, Color>, sf::Texture> tileTextures;
    bool loadTextures(const std::string& assetsDir);
    void drawTile(sf::RenderWindow& window, int x, int y, const Tile& tile);

    // Bag & hand
    std::vector<Tile> tileBag;
    std::mt19937 rng{std::random_device{}()};
    void initTileBag();
    Tile drawTileFromBag(); // assumes bag not empty
    void refillHand();
    void resetUnconfirmedTiles();

    // Player hand: 6 slots, optional if empty
    std::vector<std::optional<Tile>> playerHand; // size 6

    // Selection & staged placements
    int selectedHandIndex = -1; // -1 none selected
    std::map<Coord, Tile> stagedTiles; // temporary placements for this turn

    // UI constants
    static constexpr int TILE_SIZE = 64;
    static constexpr int BUTTON_WIDTH = 90;
    static constexpr int BUTTON_HEIGHT = 40;
    static constexpr int HAND_SLOT_PADDING = 8;

    // UI helpers
    bool pointInRect(sf::Vector2f point, sf::RectangleShape& rect);
    std::string getTextureFilename(Shape s, Color c, const std::string& assetsDir);

    // Draw the bottom hand
    void drawHand(sf::RenderWindow& window, const sf::Font& font);

    // Helper: convert world coords to board coords (flooring)
    static Coord worldToBoard(const sf::Vector2f& worldPos);
};
