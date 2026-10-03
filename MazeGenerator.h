#pragma once

#include "Maze.h"

#include <cstdint>
#include <random>
#include <vector>

class MazeGenerator {
 public:
  MazeGenerator();

  Maze generate(std::size_t rows, std::size_t cols);

 private:
  Maze generateOnce(std::size_t rows, std::size_t cols, Side entranceSide);
  std::vector<Position> findPath(const Maze& maze, Position start,
                                 Position finish) const;
  int countTurns(const std::vector<Position>& path) const;

  std::mt19937 randomEngine_;
};
