#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum class Side {
  Top,
  Right,
  Bottom,
  Left
};

struct Position {
  std::size_t row = 0;
  std::size_t col = 0;

  bool operator==(const Position& other) const {
    return row == other.row && col == other.col;
  }

  bool operator!=(const Position& other) const {
    return !(*this == other);
  }
};

struct Cell {
  bool topWall = true;
  bool rightWall = true;
  bool bottomWall = true;
  bool leftWall = true;
};

class Maze {
 public:
  Maze() = default;
  Maze(std::size_t rows, std::size_t cols);

  void reset(std::size_t rows, std::size_t cols);

  std::size_t rows() const;
  std::size_t cols() const;

  Cell& cell(std::size_t row, std::size_t col);
  const Cell& cell(std::size_t row, std::size_t col) const;

  void setEntrance(Position position, Side side);
  void setExit(Position position, Side side);
  void setEntranceInfo(Position position, Side side);
  void setExitInfo(Position position, Side side);

  Position entranceCell() const;
  Position exitCell() const;
  Side entranceSide() const;
  Side exitSide() const;

  void openWall(Position position, Side side);
  void openPassage(Position first, Side side);

  void setSolutionPath(std::vector<Position> path);
  const std::vector<Position>& solutionPath() const;

  bool isSolutionCell(Position position) const;
  bool validate(std::string& error) const;
  bool rebuildSolution();

  std::string render() const;

 private:
  std::size_t index(Position position) const;
  bool isInside(Position position) const;
  bool hasWall(Position position, Side side) const;

  std::size_t rows_ = 0;
  std::size_t cols_ = 0;
  std::vector<std::vector<Cell>> cells_;

  Position entranceCell_{};
  Position exitCell_{};
  Side entranceSide_ = Side::Left;
  Side exitSide_ = Side::Right;

  std::vector<Position> solutionPath_;
};
