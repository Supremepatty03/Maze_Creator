#pragma once

#include "Maze.h"

#include <string>

class MazeFileManager {
 public:
  static bool save(const Maze& maze, const std::string& filePath,
                   std::string& error);
  static bool load(const std::string& filePath, Maze& maze,
                   std::string& error);
};
