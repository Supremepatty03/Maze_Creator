#pragma once

#include "Maze.h"
#include "MazeGenerator.h"

#include <cstddef>
#include <string>

class MazeApplication {
 public:
  void run();

 private:
  static constexpr std::size_t kMaxDimension = 50;
  static constexpr std::size_t kMaxCells = 2500;

  void inputAndGenerate();
  void loadFromFile();
  void saveToFile();
  void printMenu() const;
  void printGreeting() const;

  std::size_t readDimension(const std::string& name) const;
  std::string readFilePath(const std::string& prompt) const;

  Maze maze_;
  MazeGenerator generator_;
  bool hasMaze_ = false;
};
