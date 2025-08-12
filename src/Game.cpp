#include "Game.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Helper name maps for filename generation - adjust to your naming convention
static const std::map<Shape, std::string> shapeNames = {
    {Shape::Circle, "O"},
    {Shape::Square, "S"},
    {Shape::Diamond, "D"},
    {Shape::Astericks, "A"},
    {Shape::Clover, "C"},
    {Shape::Fourpoint,  "F"}
};

static const std::map<Color, std::string> colorNames = {
    {Color::Red,    "r"},
    {Color::Orange, "o"},
    {Color::Yellow, "y"},
    {Color::Green,  "g"},
    {Color::Blue,   "b"},
    {Color::Purple, "p"}
};

// Game constants (mirrors Game.h)
constexpr int Game::TILE_SIZE;
constexpr int Game::BUTTON_WIDTH;
constexpr int Game::BUTTON_HEIGHT;
constexpr int Game::HAND_SLOT_PADDING;

static std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

std::string Game::getTextureFilename(Shape s, Color c, const std::string& assetsDir) {
    // e.g. assetsDir + "/rO.png" or "rO.png" depending on naming (color+shape)
    std::string filename = colorNames.at(c) + shapeNames.at(s) + ".png";
    return joinPath(assetsDir, filename);
}

bool Game::loadTextures(const std::string& assetsDir) {
    int loaded = 0;
    for (auto const& sn : shapeNames) {
        for (auto const& cn : colorNames) {
            Shape s = sn.first;
            Color c = cn.first;
            std::string fname = getTextureFilename(s, c, assetsDir);
            sf::Texture tex;
            if (!tex.loadFromFile(fname)) {
                std::cerr << "Warning: failed to load texture: " << fname << "\n";
                continue;
            }
            tex.setSmooth(true);
            tileTextures[{s, c}] = std::move(tex);
            loaded+=1;
        }
    }
    if (loaded == 0) {
        std::cerr << "Error: no tile textures loaded from '" << assetsDir << "'.\n";
        return false;
    }
    std::cout << "Loaded " << loaded << " tile textures from '" << assetsDir << "'.\n";
    return true;
}

void Game::initTileBag() {
    tileBag.clear();
    tileBag.reserve(108);
    for (const auto& s : { Shape::Circle, Shape::Square, Shape::Diamond, Shape::Astericks, Shape::Clover, Shape::Fourpoint }) {
        for (const auto& c : { Color::Red, Color::Orange, Color::Yellow, Color::Green, Color::Blue, Color::Purple }) {
            for (int copy = 0; copy < 3; ++copy) {
                tileBag.push_back(Tile{ s, c });
            }
        }
    }
    std::shuffle(tileBag.begin(), tileBag.end(), rng);
}

Tile Game::drawTileFromBag() {
    if (tileBag.empty()) {
        // In a real game, handle empty bag appropriately (return dummy or throw)
        // We'll return a fallback red circle if empty
        return Tile{Shape::Circle, Color::Red};
    }
    Tile t = tileBag.back();
    tileBag.pop_back();
    return t;
}

void Game::resetUnconfirmedTiles() {
    // Move each staged tile back into the first available empty hand slot.
    for (auto const& p : stagedTiles) {
        const Tile &t = p.second;
        bool placedInHand = false;

        // Ensure hand has size 6 (should already, but be safe)
        if (playerHand.size() != 6) playerHand.assign(6, std::nullopt);

        for (size_t i = 0; i < playerHand.size(); ++i) {
            if (!playerHand[i].has_value()) {
                playerHand[i] = t;
                placedInHand = true;
                break;
            }
        }

        if (!placedInHand) {
            // No empty slot found (shouldn't normally happen) — return tile to the bag.
            tileBag.push_back(t);
        }
    }

    // Clear staged tiles and reset selection.
    stagedTiles.clear();
    selectedHandIndex = -1;
}

void Game::refillHand() {
    // Ensure playerHand size is 6
    if (playerHand.size() != 6) playerHand.assign(6, std::nullopt);

    for (size_t i = 0; i < playerHand.size(); ++i) {
        if (!playerHand[i].has_value() && !tileBag.empty()) {
            playerHand[i] = drawTileFromBag();
        }
    }
}

void Game::drawTile(sf::RenderWindow& window, int x, int y, const Tile& tile) {
    auto it = tileTextures.find({tile.shape, tile.color});
    if (it != tileTextures.end()) {
        const sf::Texture& tex = it->second;
        sf::Sprite sprite(tex);
        sprite.setPosition(static_cast<float>(x * TILE_SIZE), static_cast<float>(y * TILE_SIZE));
        float scaleX = static_cast<float>(TILE_SIZE) / static_cast<float>(tex.getSize().x);
        float scaleY = static_cast<float>(TILE_SIZE) / static_cast<float>(tex.getSize().y);
        sprite.setScale(scaleX, scaleY);
        window.draw(sprite);
        return;
    }
}

bool Game::pointInRect(sf::Vector2f point, sf::RectangleShape& rect) {
    return rect.getGlobalBounds().contains(point);
}

Coord Game::worldToBoard(const sf::Vector2f& worldPos) {
    int bx = static_cast<int>(std::floor(worldPos.x / TILE_SIZE));
    int by = static_cast<int>(std::floor(worldPos.y / TILE_SIZE));
    return {bx, by};
}

void Game::drawHand(sf::RenderWindow& window, const sf::Font& font) {
    // Draw playerHand centered at bottom above buttons
    const float screenW = static_cast<float>(window.getSize().x);
    const float screenH = static_cast<float>(window.getSize().y);

    // Each slot width
    float slotW = static_cast<float>(TILE_SIZE) + HAND_SLOT_PADDING;
    float totalW = slotW * 6 - HAND_SLOT_PADDING; // six slots
    float startX = (screenW - totalW) / 2.0f;
    float y = screenH - static_cast<float>(TILE_SIZE) - 10.0f; // 10px top margin from bottom

    for (int i = 0; i < 6; ++i) {
        float x = startX + i * slotW;
        // Draw slot background
        sf::RectangleShape slotBg(sf::Vector2f(static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)));
        slotBg.setPosition(x, y);
        slotBg.setFillColor(sf::Color(230, 230, 230));
        slotBg.setOutlineThickness(2);
        slotBg.setOutlineColor(sf::Color::Black);
        window.draw(slotBg);

        // If this slot is selected, draw highlight
        if (i == selectedHandIndex) {
            sf::RectangleShape highlight(sf::Vector2f(static_cast<float>(TILE_SIZE)+6, static_cast<float>(TILE_SIZE)+6));
            highlight.setPosition(x - 3, y - 3);
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(3);
            highlight.setOutlineColor(sf::Color(50, 200, 50));
            window.draw(highlight);
        }

        // Draw tile if exists
        if (i < static_cast<int>(playerHand.size()) && playerHand[i].has_value()) {
            Tile t = playerHand[i].value();
            // Try to draw texture; we need to draw using screen coords (hand UI)
            auto it = tileTextures.find({t.shape, t.color});
            if (it != tileTextures.end()) {
                const sf::Texture& tex = it->second;
                sf::Sprite sprite(tex);
                sprite.setPosition(x, y);
                float scaleX = static_cast<float>(TILE_SIZE) / static_cast<float>(tex.getSize().x);
                float scaleY = static_cast<float>(TILE_SIZE) / static_cast<float>(tex.getSize().y);
                sprite.setScale(scaleX, scaleY);
                // Draw in default view space; caller must ensure default view is set
                window.draw(sprite);
            }
        } else {
            // empty slot label
            sf::Text label("-", font, 18);
            label.setFillColor(sf::Color(120, 120, 120));
            label.setPosition(x + TILE_SIZE/2 - 6, y + TILE_SIZE/2 - 12);
            window.draw(label);
        }
    }
}

void Game::run() {
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Qwirkle");
    sf::View view = window.getDefaultView();

    // Load font for buttons & hand
    sf::Font font;
    if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        std::cerr << "Failed to load system font; button/hand text may not show.\n";
    }

    // Try to load textures from assets
    if (!loadTextures("assets/tiles")) {
        loadTextures("../assets/tiles"); // fallback when running from build dir
    }

    // Initialize bag and hand
    initTileBag();
    playerHand.assign(6, std::nullopt);
    refillHand();

    // Setup buttons bottom-left (screen coords)

    sf::RectangleShape resetHandBtn(sf::Vector2f(150, 40));
    resetHandBtn.setFillColor(sf::Color(200, 200, 200));
    resetHandBtn.setPosition(200.f, window.getSize().y - BUTTON_HEIGHT - 10.f);

    sf::Text resetHandText("Reset Hand", font, 12);
    resetHandText.setFillColor(sf::Color::Black);
    resetHandText.setPosition(resetHandBtn.getPosition().x + 20.f, resetHandBtn.getPosition().y + 8.f);

    sf::RectangleShape confirmBtn(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
    confirmBtn.setFillColor(sf::Color(100, 200, 100));
    confirmBtn.setPosition(10.f, window.getSize().y - BUTTON_HEIGHT - 10.f);

    sf::Text confirmText("Confirm Move", font, 12);
    confirmText.setFillColor(sf::Color::Black);
    confirmText.setPosition(confirmBtn.getPosition().x + 10.f, confirmBtn.getPosition().y + 8.f);

    sf::RectangleShape exitBtn(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
    exitBtn.setFillColor(sf::Color(200, 100, 100));
    exitBtn.setPosition(20.f + BUTTON_WIDTH, window.getSize().y - BUTTON_HEIGHT - 10.f);

    sf::Text exitText("Exit Game", font, 12);
    exitText.setFillColor(sf::Color::Black);
    exitText.setPosition(exitBtn.getPosition().x + 10.f, exitBtn.getPosition().y + 8.f);

    bool rightMouseDown = false;
    sf::Vector2i lastMousePos;

    while (window.isOpen()) {
        // Set view so mapPixelToCoords uses the current camera
        window.setView(view);

        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;

                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                        sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos); // respects current view
                        sf::Vector2f screenPos(static_cast<float>(pixelPos.x), static_cast<float>(pixelPos.y));

                        // Check UI buttons (use screen coords and default view)
                        // NOTE: UI is drawn in default view; so check in that space
                        window.setView(window.getDefaultView());
                        if (confirmBtn.getGlobalBounds().contains(screenPos)) {
                            // Commit staged tiles
                            for (auto const& p : stagedTiles) {
                                board.placeTile(p.first.first, p.first.second, p.second);
                            }
                            stagedTiles.clear();

                            // Refill hand to 6
                            refillHand();
                            selectedHandIndex = -1;
                            // restore view
                            window.setView(view);
                            break;
                        }
                        if (exitBtn.getGlobalBounds().contains(screenPos)) {
                            window.close();
                            break;
                        }
                        if (resetHandBtn.getGlobalBounds().contains(screenPos)) {
                            resetUnconfirmedTiles();

                            // restore view and stop processing this click (so we don't also interpret it as hand/board click)
                            window.setView(view);
                            break;
                        }
                        // Also check if player clicked on the hand area (screen coords)
                        // We'll use drawHand geometry to find which slot was clicked.
                        // compute hand slot positions same as drawHand
                        const float screenW = static_cast<float>(window.getSize().x);
                        const float screenH = static_cast<float>(window.getSize().y);
                        float slotW = static_cast<float>(TILE_SIZE) + HAND_SLOT_PADDING;
                        float totalW = slotW * 6 - HAND_SLOT_PADDING;
                        float startX = (screenW - totalW) / 2.0f;
                        float y = screenH - static_cast<float>(TILE_SIZE) - 10.0f;

                        // If click falls in hand vertical band
                        if (screenPos.y >= y && screenPos.y <= y + TILE_SIZE) {
                            for (int i = 0; i < 6; ++i) {
                                float x = startX + i * slotW;
                                if (screenPos.x >= x && screenPos.x <= x + TILE_SIZE) {
                                    // clicked slot i
                                    if (playerHand[i].has_value()) {
                                        // select this tile (toggle)
                                        if (selectedHandIndex == i) selectedHandIndex = -1;
                                        else selectedHandIndex = i;
                                    } else {
                                        // clicked empty slot: do nothing
                                    }
                                    break;
                                }
                            }
                            // restore view and continue
                            window.setView(view);
                            break;
                        }

                        // Restore view for board interactions
                        window.setView(view);

                        // If a hand tile is selected, place it to world (board coords) as staged tile
                        if (selectedHandIndex >= 0 && selectedHandIndex < static_cast<int>(playerHand.size())
                            && playerHand[selectedHandIndex].has_value()) {
                            Coord boardCoord = worldToBoard(worldPos);
                            // don't allow placing on occupied board or already staged spot
                            if (!board.isOccupied(boardCoord.first, boardCoord.second)
                                && stagedTiles.find(boardCoord) == stagedTiles.end()) {
                                // place staged tile
                                stagedTiles[boardCoord] = playerHand[selectedHandIndex].value();
                                // remove from hand (slot becomes empty)
                                playerHand[selectedHandIndex] = std::nullopt;
                                // clear selection
                                selectedHandIndex = -1;
                            }
                        }

                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        rightMouseDown = true;
                        lastMousePos = {event.mouseButton.x, event.mouseButton.y};
                    }
                    break;

                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Right) {
                        rightMouseDown = false;
                    }
                    break;

                case sf::Event::MouseMoved:
                    if (rightMouseDown) {
                        sf::Vector2i newPos(event.mouseMove.x, event.mouseMove.y);
                        sf::Vector2f delta = window.mapPixelToCoords(lastMousePos) - window.mapPixelToCoords(newPos);
                        view.move(delta);
                        window.setView(view);
                        lastMousePos = newPos;
                    }
                    break;

                default:
                    break;
            }
        }

        // Draw
        window.clear(sf::Color::White);

        // Board view for tiles (including staged)
        window.setView(view);

        // Draw already-committed tiles
        for (auto const& p : board.getTiles()) {
            drawTile(window, p.first.first, p.first.second, p.second);
        }

        // Draw staged tiles (slightly highlighted with a green outline)
        for (auto const& p : stagedTiles) {
            // draw tile sprite
            drawTile(window, p.first.first, p.first.second, p.second);

            // draw outline rect to indicate staging
            sf::RectangleShape outline(sf::Vector2f(static_cast<float>(TILE_SIZE - 4), static_cast<float>(TILE_SIZE - 4)));
            outline.setPosition(static_cast<float>(p.first.first * TILE_SIZE), static_cast<float>(p.first.second * TILE_SIZE));
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineThickness(3);
            outline.setOutlineColor(sf::Color(50, 200, 50));
            window.draw(outline);
        }

        // UI in default view (hand + buttons)
        window.setView(window.getDefaultView());
        // draw hand (centered bottom)
        drawHand(window, font);

        // draw "Confirm Move" button
        sf::RectangleShape confirmBtnLocal(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
        confirmBtnLocal.setFillColor(sf::Color(100, 200, 100));
        confirmBtnLocal.setPosition(10.f, window.getSize().y - BUTTON_HEIGHT - 10.f);
        window.draw(confirmBtnLocal);
        window.draw(confirmText);

        // draw "Exit Game" button
        sf::RectangleShape exitBtnLocal(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
        exitBtnLocal.setFillColor(sf::Color(200, 100, 100));
        exitBtnLocal.setPosition(20.f + BUTTON_WIDTH, window.getSize().y - BUTTON_HEIGHT - 10.f);
        window.draw(exitBtnLocal);
        window.draw(exitText);

        // draw "Reset Hand" button
        sf::RectangleShape resetHandBtnLocal(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
        resetHandBtnLocal.setFillColor(sf::Color(200, 200, 100));
        resetHandBtnLocal.setPosition(30.f + BUTTON_WIDTH * 2, window.getSize().y - BUTTON_HEIGHT - 10.f);
        window.draw(resetHandBtnLocal);
        window.draw(resetHandText);



        window.display();
    }
}
