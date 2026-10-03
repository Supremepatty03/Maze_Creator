#include "Maze.h"

#include <algorithm>
#include <array>
#include <queue>
#include <sstream>
#include <utility>

namespace {

constexpr std::array<Side, 4> kDirections = {
    Side::Top,
    Side::Right,
    Side::Bottom,
    Side::Left};

Position getNeighbor(Position position, Side side) {
  switch (side) {
    case Side::Top:
      return {position.row - 1, position.col};
    case Side::Right:
      return {position.row, position.col + 1};
    case Side::Bottom:
      return {position.row + 1, position.col};
    case Side::Left:
      return {position.row, position.col - 1};
  }

  return position;
}

Side getOppositeSide(Side side) {
  switch (side) {
    case Side::Top:
      return Side::Bottom;
    case Side::Right:
      return Side::Left;
    case Side::Bottom:
      return Side::Top;
    case Side::Left:
      return Side::Right;
  }

  return Side::Left;
}

bool hasBoundaryPosition(Position position, std::size_t rows,
                         std::size_t cols, Side side) {
  switch (side) {
    case Side::Top:
      return position.row == 0;
    case Side::Right:
      return position.col + 1 == cols;
    case Side::Bottom:
      return position.row + 1 == rows;
    case Side::Left:
      return position.col == 0;
  }

  return false;
}

bool getWall(const Cell& cell, Side side) {
  switch (side) {
    case Side::Top:
      return cell.topWall;
    case Side::Right:
      return cell.rightWall;
    case Side::Bottom:
      return cell.bottomWall;
    case Side::Left:
      return cell.leftWall;
  }

  return true;
}

void setWall(Cell& cell, Side side, bool value) {
  switch (side) {
    case Side::Top:
      cell.topWall = value;
      break;
    case Side::Right:
      cell.rightWall = value;
      break;
    case Side::Bottom:
      cell.bottomWall = value;
      break;
    case Side::Left:
      cell.leftWall = value;
      break;
  }
}


}  // namespace

Maze::Maze(std::size_t rows, std::size_t cols) {
  reset(rows, cols);
}

void Maze::reset(std::size_t rows, std::size_t cols) {
  rows_ = rows;
  cols_ = cols;
  cells_.assign(rows_, std::vector<Cell>(cols_));
  entranceCell_ = {};
  exitCell_ = {rows_ == 0 ? 0 : rows_ - 1, cols_ == 0 ? 0 : cols_ - 1};
  entranceSide_ = Side::Left;
  exitSide_ = Side::Right;
  solutionPath_.clear();
}

std::size_t Maze::rows() const {
  return rows_;
}

std::size_t Maze::cols() const {
  return cols_;
}

Cell& Maze::cell(std::size_t row, std::size_t col) {
  return cells_.at(row).at(col);
}

const Cell& Maze::cell(std::size_t row, std::size_t col) const {
  return cells_.at(row).at(col);
}

void Maze::setEntrance(Position position, Side side) {
  setEntranceInfo(position, side);
  openWall(position, side);
}

void Maze::setExit(Position position, Side side) {
  setExitInfo(position, side);
  openWall(position, side);
}

void Maze::setEntranceInfo(Position position, Side side) {
  entranceCell_ = position;
  entranceSide_ = side;
}

void Maze::setExitInfo(Position position, Side side) {
  exitCell_ = position;
  exitSide_ = side;
}

Position Maze::entranceCell() const {
  return entranceCell_;
}

Position Maze::exitCell() const {
  return exitCell_;
}

Side Maze::entranceSide() const {
  return entranceSide_;
}

Side Maze::exitSide() const {
  return exitSide_;
}

void Maze::openWall(Position position, Side side) {
  if (!isInside(position)) {
    return;
  }

  setWall(cells_[position.row][position.col], side, false);
}

void Maze::openPassage(Position first, Side side) {
  if (!isInside(first)) {
    return;
  }

  const Position second = getNeighbor(first, side);
  if (!isInside(second)) {
    return;
  }

  openWall(first, side);
  openWall(second, getOppositeSide(side));
}

void Maze::setSolutionPath(std::vector<Position> path) {
  solutionPath_ = std::move(path);
}

const std::vector<Position>& Maze::solutionPath() const {
  return solutionPath_;
}

bool Maze::isSolutionCell(Position position) const {
  for (const Position& solutionPosition : solutionPath_) {
    if (solutionPosition == position) {
      return true;
    }
  }

  return false;
}

std::size_t Maze::index(Position position) const {
  return position.row * cols_ + position.col;
}

bool Maze::isInside(Position position) const {
  return rows_ > 0 && cols_ > 0 && position.row < rows_ && position.col < cols_;
}

bool Maze::hasWall(Position position, Side side) const {
  return getWall(cells_[position.row][position.col], side);
}

bool Maze::rebuildSolution() {
  if (!isInside(entranceCell_) || !isInside(exitCell_) || rows_ == 0 || cols_ == 0) {
    return false;
  }

  const std::size_t cellCount = rows_ * cols_;
  std::vector<bool> visited(cellCount, false);
  std::vector<Position> parent(cellCount, {rows_, cols_});
  std::queue<Position> queue;

  queue.push(entranceCell_);
  visited[index(entranceCell_)] = true;
  parent[index(entranceCell_)] = entranceCell_;

  while (!queue.empty()) {
    const Position current = queue.front();
    queue.pop();

    if (current == exitCell_) {
      break;
    }

    for (const Side side : kDirections) {
      const Position next = getNeighbor(current, side);
      if (!isInside(next) || hasWall(current, side) ||
          visited[index(next)]) {
        continue;
      }

      visited[index(next)] = true;
      parent[index(next)] = current;
      queue.push(next);
    }
  }

  if (!visited[index(exitCell_)]) {
    solutionPath_.clear();
    return false;
  }

  std::vector<Position> path;
  Position current = exitCell_;
  path.push_back(current);

  while (current != entranceCell_) {
    current = parent[index(current)];
    path.push_back(current);
  }

  std::reverse(path.begin(), path.end());
  solutionPath_ = std::move(path);
  return true;
}

bool Maze::validate(std::string& error) const {
  if (rows_ == 0 || cols_ == 0) {
    error = "Размеры лабиринта должны быть положительными.";
    return false;
  }

  if (!isInside(entranceCell_) || !isInside(exitCell_)) {
    error = "Вход или выход указывает на несуществующую ячейку.";
    return false;
  }

  if (!hasBoundaryPosition(entranceCell_, rows_, cols_, entranceSide_) ||
      !hasBoundaryPosition(exitCell_, rows_, cols_, exitSide_)) {
    error = "Вход и выход должны находиться на границах лабиринта.";
    return false;
  }

  if (getWall(cells_[entranceCell_.row][entranceCell_.col], entranceSide_)) {
    error = "На стороне входа нет отверстия.";
    return false;
  }

  if (getWall(cells_[exitCell_.row][exitCell_.col], exitSide_)) {
    error = "На стороне выхода нет отверстия.";
    return false;
  }

  const Side expectedExitSide = getOppositeSide(entranceSide_);
  if (exitSide_ != expectedExitSide) {
    error = "Вход и выход должны располагаться на противоположных сторонах.";
    return false;
  }

  std::size_t boundaryOpenings = 0;

  for (std::size_t row = 0; row < rows_; ++row) {
    for (std::size_t col = 0; col < cols_; ++col) {
      const Position position{row, col};
      const Cell& currentCell = cells_[row][col];

      if (row == 0 && !currentCell.topWall) {
        ++boundaryOpenings;
      }
      if (col + 1 == cols_ && !currentCell.rightWall) {
        ++boundaryOpenings;
      }
      if (row + 1 == rows_ && !currentCell.bottomWall) {
        ++boundaryOpenings;
      }
      if (col == 0 && !currentCell.leftWall) {
        ++boundaryOpenings;
      }

      if (row + 1 < rows_) {
        const Cell& below = cells_[row + 1][col];
        if (currentCell.bottomWall != below.topWall) {
          error = "Файл содержит несовпадающие стены между соседними ячейками.";
          return false;
        }
      } else if (!currentCell.bottomWall &&
                 !(position == exitCell_ && exitSide_ == Side::Bottom) &&
                 !(position == entranceCell_ && entranceSide_ == Side::Bottom)) {
        error = "Обнаружено лишнее отверстие по внешней границе.";
        return false;
      }

      if (col + 1 < cols_) {
        const Cell& right = cells_[row][col + 1];
        if (currentCell.rightWall != right.leftWall) {
          error = "Файл содержит несовпадающие стены между соседними ячейками.";
          return false;
        }
      } else if (!currentCell.rightWall &&
                 !(position == exitCell_ && exitSide_ == Side::Right) &&
                 !(position == entranceCell_ && entranceSide_ == Side::Right)) {
        error = "Обнаружено лишнее отверстие по внешней границе.";
        return false;
      }

      if (row == 0 && !currentCell.topWall &&
          !(position == entranceCell_ && entranceSide_ == Side::Top) &&
          !(position == exitCell_ && exitSide_ == Side::Top)) {
        error = "Обнаружено лишнее отверстие по внешней границе.";
        return false;
      }

      if (col == 0 && !currentCell.leftWall &&
          !(position == entranceCell_ && entranceSide_ == Side::Left) &&
          !(position == exitCell_ && exitSide_ == Side::Left)) {
        error = "Обнаружено лишнее отверстие по внешней границе.";
        return false;
      }
    }
  }

  if (boundaryOpenings != 2) {
    error = "Лабиринт должен иметь ровно два отверстия: вход и выход.";
    return false;
  }

  std::size_t passageCount = 0;
  for (std::size_t row = 0; row < rows_; ++row) {
    for (std::size_t col = 0; col < cols_; ++col) {
      if (row + 1 < rows_ && !cells_[row][col].bottomWall) {
        ++passageCount;
      }
      if (col + 1 < cols_ && !cells_[row][col].rightWall) {
        ++passageCount;
      }
    }
  }

  if (passageCount != rows_ * cols_ - 1) {
    error = "Лабиринт не является деревом: число внутренних проходов должно быть равно числу ячеек минус один.";
    return false;
  }

  std::vector<bool> visited(rows_ * cols_, false);
  std::queue<Position> queue;
  queue.push(entranceCell_);
  visited[index(entranceCell_)] = true;
  std::size_t visitedCount = 0;

  while (!queue.empty()) {
    const Position current = queue.front();
    queue.pop();
    ++visitedCount;

    for (const Side side : kDirections) {
      const Position next = getNeighbor(current, side);
      if (!isInside(next) || hasWall(current, side) || visited[index(next)]) {
        continue;
      }

      visited[index(next)] = true;
      queue.push(next);
    }
  }

  if (visitedCount != rows_ * cols_) {
    error = "Не все ячейки лабиринта связаны в одну компоненту.";
    return false;
  }

  return true;
}

std::string Maze::render() const {
  std::ostringstream output;

  output << "Лабиринт " << rows_ << " x " << cols_ << "\n";
  output << "S — вход, E — выход, . — клетки единственного решения\n\n";

  for (std::size_t row = 0; row < rows_; ++row) {
    for (std::size_t col = 0; col < cols_; ++col) {
      output << '+' << (cells_[row][col].topWall ? "---" : "   ");
    }
    output << "+\n";

    for (std::size_t col = 0; col < cols_; ++col) {
      const Cell& currentCell = cells_[row][col];
      const Position position{row, col};

      // Вертикальная стена между двумя соседними ячейками выводится
      // один раз — как левая стена текущей ячейки. Благодаря этому ширина
      // строки совпадает с шириной горизонтальных стен.
      output << (currentCell.leftWall ? '|' : ' ');

      char marker = ' ';
      const bool isEntrance = position == entranceCell_;
      const bool isExit = position == exitCell_;

      if (isEntrance && isExit) {
        marker = 'X';
      } else if (isEntrance) {
        marker = 'S';
      } else if (isExit) {
        marker = 'E';
      } else if (isSolutionCell(position)) {
        marker = '.';
      }

      output << ' ' << marker << ' ';
    }

    const Cell& lastCell = cells_[row][cols_ - 1];
    output << (lastCell.rightWall ? '|' : ' ');
    output << '\n';

    if (row + 1 == rows_) {
      for (std::size_t col = 0; col < cols_; ++col) {
        output << '+' << (cells_[row][col].bottomWall ? "---" : "   ");
      }
      output << "+\n";
    }
  }

  output << "\nКлеток в решении: " << solutionPath_.size() << "\n";
  if (!solutionPath_.empty()) {
    output << "Переходов между клетками: " << solutionPath_.size() - 1 << "\n";
  }
  return output.str();
}
