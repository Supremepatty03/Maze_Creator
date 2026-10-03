#include "MazeFileManager.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {

int wallMask(const Cell& cell) {
  int mask = 0;
  if (cell.topWall) mask |= 1;
  if (cell.rightWall) mask |= 2;
  if (cell.bottomWall) mask |= 4;
  if (cell.leftWall) mask |= 8;
  return mask;
}

void applyWallMask(Cell& cell, int mask) {
  cell.topWall = (mask & 1) != 0;
  cell.rightWall = (mask & 2) != 0;
  cell.bottomWall = (mask & 4) != 0;
  cell.leftWall = (mask & 8) != 0;
}

char sideToChar(Side side) {
  switch (side) {
    case Side::Top: return 'T';
    case Side::Right: return 'R';
    case Side::Bottom: return 'B';
    case Side::Left: return 'L';
  }
  return 'L';
}

bool charToSide(char value, Side& side) {
  switch (value) {
    case 'T': side = Side::Top; return true;
    case 'R': side = Side::Right; return true;
    case 'B': side = Side::Bottom; return true;
    case 'L': side = Side::Left; return true;
    default: return false;
  }
}

bool isReadOnly(const std::filesystem::path& path) {
#ifdef _WIN32
  const DWORD attributes = GetFileAttributesW(path.wstring().c_str());
  if (attributes != INVALID_FILE_ATTRIBUTES &&
      (attributes & FILE_ATTRIBUTE_READONLY) != 0) {
    return true;
  }
#endif

  std::error_code errorCode;
  const auto status = std::filesystem::status(path, errorCode);
  if (errorCode) {
    return false;
  }

  const auto permissions = status.permissions();
  const auto writePermissions = std::filesystem::perms::owner_write |
                                 std::filesystem::perms::group_write |
                                 std::filesystem::perms::others_write;
  return (permissions & writePermissions) == std::filesystem::perms::none;
}

}  // namespace

bool MazeFileManager::save(const Maze& maze, const std::string& filePath,
                           std::string& error) {
  const std::filesystem::path path(filePath);

  if (std::filesystem::exists(path) && isReadOnly(path)) {
    error = "Файл доступен только для чтения.";
    return false;
  }

  std::ofstream output(filePath, std::ios::out | std::ios::trunc);
  if (!output.is_open()) {
    error = "Не удалось открыть файл для записи. Проверьте путь и права доступа.";
    return false;
  }

  output << "MAZE 1\n";
  output << maze.rows() << ' ' << maze.cols() << "\n";
  output << "ENTRANCE " << sideToChar(maze.entranceSide()) << ' '
         << maze.entranceCell().row << ' ' << maze.entranceCell().col << "\n";
  output << "EXIT " << sideToChar(maze.exitSide()) << ' '
         << maze.exitCell().row << ' ' << maze.exitCell().col << "\n";

  output << "WALLS\n";
  for (std::size_t row = 0; row < maze.rows(); ++row) {
    for (std::size_t col = 0; col < maze.cols(); ++col) {
      output << wallMask(maze.cell(row, col));
      if (col + 1 < maze.cols()) {
        output << ' ';
      }
    }
    output << '\n';
  }

  output << "SOLUTION " << maze.solutionPath().size() << "\n";
  for (const Position& position : maze.solutionPath()) {
    output << position.row << ' ' << position.col << '\n';
  }

  output << "END\n";
  output.flush();

  if (!output) {
    error = "Ошибка записи данных в файл.";
    return false;
  }

  return true;
}

bool MazeFileManager::load(const std::string& filePath, Maze& maze,
                           std::string& error) {
  std::ifstream input(filePath);
  if (!input.is_open()) {
    error = "Не удалось открыть файл для чтения.";
    return false;
  }

  std::string header;
  int version = 0;
  if (!(input >> header >> version) || header != "MAZE" || version != 1) {
    error = "Неверный формат файла лабиринта.";
    return false;
  }

  std::size_t rows = 0;
  std::size_t cols = 0;
  if (!(input >> rows >> cols) || rows == 0 || cols == 0) {
    error = "В файле указаны некорректные размеры лабиринта.";
    return false;
  }

  Maze loadedMaze(rows, cols);

  std::string section;
  char entranceSideChar = 0;
  char exitSideChar = 0;
  std::size_t entranceRow = 0;
  std::size_t entranceCol = 0;
  std::size_t exitRow = 0;
  std::size_t exitCol = 0;

  if (!(input >> section >> entranceSideChar >> entranceRow >> entranceCol) ||
      section != "ENTRANCE") {
    error = "Не удалось прочитать данные о входе.";
    return false;
  }

  if (!(input >> section >> exitSideChar >> exitRow >> exitCol) ||
      section != "EXIT") {
    error = "Не удалось прочитать данные о выходе.";
    return false;
  }

  Side entranceSide;
  Side exitSide;
  if (!charToSide(entranceSideChar, entranceSide) ||
      !charToSide(exitSideChar, exitSide)) {
    error = "В файле указана неизвестная сторона входа или выхода.";
    return false;
  }

  if (!(input >> section) || section != "WALLS") {
    error = "В файле отсутствует секция стен.";
    return false;
  }

  for (std::size_t row = 0; row < rows; ++row) {
    for (std::size_t col = 0; col < cols; ++col) {
      int mask = 0;
      if (!(input >> mask) || mask < 0 || mask > 15) {
        error = "Обнаружена некорректная маска стены.";
        return false;
      }
      applyWallMask(loadedMaze.cell(row, col), mask);
    }
  }

  std::size_t solutionSize = 0;
  if (!(input >> section >> solutionSize) || section != "SOLUTION") {
    error = "В файле отсутствует секция решения.";
    return false;
  }

  std::vector<Position> storedSolution;
  storedSolution.reserve(solutionSize);
  for (std::size_t i = 0; i < solutionSize; ++i) {
    Position position;
    if (!(input >> position.row >> position.col) || position.row >= rows ||
        position.col >= cols) {
      error = "В секции решения обнаружены некорректные координаты.";
      return false;
    }
    storedSolution.push_back(position);
  }

  if (!(input >> section) || section != "END") {
    error = "Некорректное завершение файла.";
    return false;
  }

  loadedMaze.setEntranceInfo({entranceRow, entranceCol}, entranceSide);
  loadedMaze.setExitInfo({exitRow, exitCol}, exitSide);

  // После setEntrance/setExit отверстия должны совпадать с данными файла.
  std::string validationError;
  if (!loadedMaze.validate(validationError)) {
    error = "Лабиринт в файле некорректен: " + validationError;
    return false;
  }

  if (!loadedMaze.rebuildSolution()) {
    error = "Не удалось восстановить единственный путь решения.";
    return false;
  }

  if (loadedMaze.solutionPath() != storedSolution) {
    error = "Сохраненное решение не соответствует стенам лабиринта.";
    return false;
  }

  maze = std::move(loadedMaze);
  return true;
}
