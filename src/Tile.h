#pragma once
#include <SFML/Graphics.hpp>

enum class Shape { Circle, Square, Diamond, Fourpoint, Clover, Astericks };
enum class Color { Red, Orange, Yellow, Green, Blue, Purple };

struct Tile {
    Shape shape;
    Color color;
};