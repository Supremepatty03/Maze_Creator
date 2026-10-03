#include "MazeGenerator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <queue>

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

bool isInside(Position position, std::size_t rows, std::size_t cols) {
  return position.row < rows && position.col < cols;
}

bool hasWall(const Maze& maze, Position position, Side side) {
  const Cell& cell = maze.cell(position.row, position.col);
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

}  // namespace

MazeGenerator::MazeGenerator() {
  std::random_device randomDevice;
  const auto timeSeed = static_cast<std::uint32_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  const std::uint32_t seed = randomDevice() ^ timeSeed;
  randomEngine_.seed(seed);
}

Maze MazeGenerator::generate(std::size_t rows, std::size_t cols) {
  std::uniform_int_distribution<int> orientationDistribution(0, 1);
  const bool horizontalOrientation = orientationDistribution(randomEngine_) == 0;

  const Side entranceSide = horizontalOrientation ? Side::Left : Side::Top;
  const Side exitSide = getOppositeSide(entranceSide);

  Maze bestMaze;
  std::vector<Position> bestPath;
  long long bestScore = -1;

  constexpr int kAttempts = 8;

  for (int attempt = 0; attempt < kAttempts; ++attempt) {
    const Side currentEntranceSide = entranceSide;
    const Side currentExitSide = exitSide;
    Maze maze = generateOnce(rows, cols, currentEntranceSide);

    const std::size_t oppositeSideLength =
        (currentExitSide == Side::Left || currentExitSide == Side::Right)
            ? rows
            : cols;

    for (std::size_t i = 0; i < oppositeSideLength; ++i) {
      Position candidate;
      switch (currentExitSide) {
        case Side::Top:
          candidate = {0, i};
          break;
        case Side::Right:
          candidate = {i, cols - 1};
          break;
        case Side::Bottom:
          candidate = {rows - 1, i};
          break;
        case Side::Left:
          candidate = {i, 0};
          break;
      }

      const std::vector<Position> path =
          findPath(maze, maze.entranceCell(), candidate);
      if (path.empty()) {
        continue;
      }

      const int turns = countTurns(path);
      const long long score = static_cast<long long>(path.size()) * 10 +
                              static_cast<long long>(turns) * 35;

      if (score > bestScore) {
        bestMaze = maze;
        bestMaze.setExit(candidate, currentExitSide);
        bestMaze.setSolutionPath(path);
        bestScore = score;
        bestPath = path;
      }
    }
  }

  if (bestScore < 0) {
    const Side entranceSideFallback = entranceSide;
    const Side exitSideFallback = exitSide;
    bestMaze = generateOnce(rows, cols, entranceSideFallback);
    bestMaze.setExit(
        exitSideFallback == Side::Right ? Position{rows - 1, cols - 1}
        : exitSideFallback == Side::Left ? Position{rows - 1, 0}
        : exitSideFallback == Side::Bottom ? Position{rows - 1, cols - 1}
                                           : Position{0, cols - 1},
        exitSideFallback);
    bestMaze.rebuildSolution();
  } else {
    bestMaze.setSolutionPath(bestPath);
  }

  return bestMaze;
}

Maze MazeGenerator::generateOnce(std::size_t rows, std::size_t cols,
                                 Side entranceSide) {
  Maze maze(rows, cols);

  Position entrance;
  std::uniform_int_distribution<std::size_t> rowDistribution(0, rows - 1);
  std::uniform_int_distribution<std::size_t> colDistribution(0, cols - 1);

  switch (entranceSide) {
    case Side::Left:
      entrance = {rowDistribution(randomEngine_), 0};
      break;
    case Side::Right:
      entrance = {rowDistribution(randomEngine_), cols - 1};
      break;
    case Side::Top:
      entrance = {0, colDistribution(randomEngine_)};
      break;
    case Side::Bottom:
      entrance = {rows - 1, colDistribution(randomEngine_)};
      break;
  }

  maze.setEntrance(entrance, entranceSide);

  const std::size_t cellCount = rows * cols;
  std::vector<bool> visited(cellCount, false);
  std::vector<Position> stack;
  stack.reserve(cellCount);
  stack.push_back(entrance);
  visited[entrance.row * cols + entrance.col] = true;

  while (!stack.empty()) {
    const Position current = stack.back();
    std::array<Side, 4> directions = kDirections;
    std::shuffle(directions.begin(), directions.end(), randomEngine_);

    bool moved = false;

    for (const Side side : directions) {
      const Position next = getNeighbor(current, side);
      if (!isInside(next, rows, cols) ||
          visited[next.row * cols + next.col]) {
        continue;
      }

      maze.openPassage(current, side);
      visited[next.row * cols + next.col] = true;
      stack.push_back(next);
      moved = true;
      break;
    }

    if (!moved) {
      stack.pop_back();
    }
  }

  return maze;
}

std::vector<Position> MazeGenerator::findPath(const Maze& maze, Position start,
                                               Position finish) const {
  const std::size_t rows = maze.rows();
  const std::size_t cols = maze.cols();
  const std::size_t cellCount = rows * cols;

  std::vector<bool> visited(cellCount, false);
  std::vector<Position> parent(cellCount, {rows, cols});
  std::queue<Position> queue;

  const auto index = [cols](Position position) {
    return position.row * cols + position.col;
  };

  queue.push(start);
  visited[index(start)] = true;
  parent[index(start)] = start;

  while (!queue.empty()) {
    const Position current = queue.front();
    queue.pop();

    if (current == finish) {
      break;
    }

    for (const Side side : kDirections) {
      const Position next = getNeighbor(current, side);
      if (!isInside(next, rows, cols) || hasWall(maze, current, side) ||
          visited[index(next)]) {
        continue;
      }

      visited[index(next)] = true;
      parent[index(next)] = current;
      queue.push(next);
    }
  }

  if (!visited[index(finish)]) {
    return {};
  }

  std::vector<Position> path;
  Position current = finish;
  path.push_back(current);

  while (current != start) {
    current = parent[index(current)];
    path.push_back(current);
  }

  std::reverse(path.begin(), path.end());
  return path;
}

int MazeGenerator::countTurns(const std::vector<Position>& path) const {
  if (path.size() < 3) {
    return 0;
  }

  int turns = 0;

  for (std::size_t i = 1; i + 1 < path.size(); ++i) {
    const Position& previous = path[i - 1];
    const Position& current = path[i];
    const Position& next = path[i + 1];

    const int previousRowDelta = static_cast<int>(current.row) -
                                 static_cast<int>(previous.row);
    const int previousColDelta = static_cast<int>(current.col) -
                                 static_cast<int>(previous.col);
    const int nextRowDelta = static_cast<int>(next.row) -
                             static_cast<int>(current.row);
    const int nextColDelta = static_cast<int>(next.col) -
                             static_cast<int>(current.col);

    if (previousRowDelta != nextRowDelta ||
        previousColDelta != nextColDelta) {
      ++turns;
    }
  }

  return turns;
}
