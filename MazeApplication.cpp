#include "MazeApplication.h"

#include "MazeFileManager.h"

#include <filesystem>
#include <iostream>
#include <string>

void MazeApplication::run() {
  printGreeting();
  while (true) {
    printMenu();
    std::cout << "Выберите пункт: ";

    std::string choice;
    std::getline(std::cin, choice);
    if (!std::cin) {
      return;
    }

    if (choice == "1") {
      inputAndGenerate();
    } else if (choice == "2") {
      loadFromFile();
    } else if (choice == "3") {
      saveToFile();
    } else if (choice == "0") {
      return;
    } else {
      std::cout << "Неизвестный пункт меню.\n\n";
    }
  }
}

void MazeApplication::printGreeting() const {
    std::cout << "Лабораторная работа №2, вариант №2.\n";
    std::cout << "Исполнители: Тарасов Артем, Александрычева Алена  гр.433\n";
}

void MazeApplication::printMenu() const {
  std::cout << "============================\n";
  std::cout << "      ГЕНЕРАТОР ЛАБИРИНТА\n";
  std::cout << "============================\n";
  std::cout << "1. Ввести m и n и построить лабиринт\n";
  std::cout << "2. Загрузить лабиринт из файла\n";
  std::cout << "3. Сохранить текущий лабиринт в файл\n";
  std::cout << "0. Выход\n";
  std::cout << "============================\n";
}

void MazeApplication::inputAndGenerate() {
  const std::size_t rows = readDimension("m");
  const std::size_t cols = readDimension("n");

  if (rows * cols > kMaxCells) {
    std::cout << "Ошибка: размер лабиринта слишком велик для консольного вывода. "
                 "Максимум — "
              << kMaxCells << " ячеек.\n\n";
    return;
  }

  maze_ = generator_.generate(rows, cols);
  hasMaze_ = true;

  std::string error;
  if (!maze_.validate(error)) {
    std::cout << "Внутренняя ошибка генерации: " << error << "\n\n";
    hasMaze_ = false;
    return;
  }

  std::cout << '\n' << maze_.render() << '\n';
}

void MazeApplication::loadFromFile() {
  const std::string filePath = readFilePath("Введите полный путь к файлу: ");

  Maze loadedMaze;
  std::string error;
  if (!MazeFileManager::load(filePath, loadedMaze, error)) {
    std::cout << "Ошибка загрузки: " << error << "\n\n";
    return;
  }

  if (loadedMaze.rows() > kMaxDimension ||
      loadedMaze.cols() > kMaxDimension ||
      loadedMaze.rows() * loadedMaze.cols() > kMaxCells) {
    std::cout << "Ошибка: размеры загруженного лабиринта превышают "
                 "допустимые для консольного вывода.\n\n";
    return;
  }

  maze_ = std::move(loadedMaze);
  hasMaze_ = true;

  // После загрузки результат сразу показывается.
  std::cout << '\n' << maze_.render() << '\n';
}

void MazeApplication::saveToFile() {
  if (!hasMaze_) {
    std::cout << "Сначала постройте или загрузите лабиринт.\n\n";
    return;
  }

  std::string filePath = readFilePath("Введите полный путь для сохранения: ");

  while (true) {
    const std::filesystem::path path(filePath);

    if (std::filesystem::exists(path)) {
      std::cout << "Файл уже существует. [o] Перезаписать, [n] Указать новое "
                   "имя, [c] Отмена: ";
      std::string action;
      std::getline(std::cin, action);

      if (action == "o" || action == "O") {
        // Продолжаем к попытке записи.
      } else if (action == "n" || action == "N") {
        filePath = readFilePath("Введите новый полный путь: ");
        continue;
      } else if (action == "c" || action == "C") {
        std::cout << "Сохранение отменено.\n\n";
        return;
      } else {
        std::cout << "Введите o, n или c.\n";
        continue;
      }
    }

    std::string error;
    if (MazeFileManager::save(maze_, filePath, error)) {
      std::cout << "Лабиринт сохранен: " << filePath << "\n\n";
      return;
    }

    std::cout << "Ошибка сохранения: " << error << '\n';
    std::cout << "[n] Указать новый путь, [c] Отмена: ";
    std::string action;
    std::getline(std::cin, action);

    if (action == "n" || action == "N") {
      filePath = readFilePath("Введите новый полный путь: ");
      continue;
    }

    std::cout << "Сохранение отменено.\n\n";
    return;
  }
}

std::size_t MazeApplication::readDimension(const std::string& name) const {
  while (true) {
    std::cout << "Введите " << name << " (целое положительное число, максимум "
              << kMaxDimension << "): ";

    std::string input;
    std::getline(std::cin, input);

    try {
      std::size_t parsedSymbols = 0;
      const unsigned long long value = std::stoull(input, &parsedSymbols);

      if (parsedSymbols != input.size() || value == 0 ||
          value > kMaxDimension) {
        throw std::out_of_range("invalid dimension");
      }

      return static_cast<std::size_t>(value);
    } catch (...) {
      std::cout << "Ошибка: требуется целое число от 1 до " << kMaxDimension
                << ".\n";
    }
  }
}

std::string MazeApplication::readFilePath(const std::string& prompt) const {
  while (true) {
    std::cout << prompt;
    std::string filePath;
    std::getline(std::cin, filePath);

    if (!filePath.empty()) {
      return filePath;
    }

    std::cout << "Путь не должен быть пустым.\n";
  }
}

