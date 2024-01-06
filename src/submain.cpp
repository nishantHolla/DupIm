
#include <pHash.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#define DEFAULT_THRESHOLD 15

const std::vector<std::string> IMAGE_EXTS = {".png", ".jpg", ".jpeg"};
int THRESHOLD;
bool isImage(const std::filesystem::path &_path);
std::string shiftArgs(int *argc, char **argv[]);

int main(int argc, char *argv[]) {
  if (argc < 2) return 1;

  const std::string PROGRAM = shiftArgs(&argc, &argv);
  const std::string TARGET = shiftArgs(&argc, &argv);
  const std::string THRESHOLD_INPUT = shiftArgs(&argc, &argv);
  std::fstream OUTPUT_FILE("./duplicatesOutput.txt", std::ios::out);
  std::fstream LOG_FILE("./duplicatesLog.txt", std::ios::out);

  if (!OUTPUT_FILE) return 2;

  if (std::filesystem::is_directory(TARGET) == false) return 3;

  if (THRESHOLD_INPUT.empty() == false) {
    try {
      THRESHOLD = std::stoi(THRESHOLD_INPUT);
    } catch (std::invalid_argument e) {
      THRESHOLD = DEFAULT_THRESHOLD;
    }
  }

  std::unordered_map<ulong64, std::filesystem::path> imageHashes;
  for (auto &entry : std::filesystem::recursive_directory_iterator(TARGET)) {
    if (isImage(entry.path()) == false) continue;

    const std::filesystem::path IMAGE_PATH =
        std::filesystem::canonical(entry.path());
    ulong64 IMAGE_HASH;
    ph_dct_imagehash(IMAGE_PATH.c_str(), IMAGE_HASH);
    std::cout << "Working on " << IMAGE_PATH.string() << "\n";

    if (imageHashes.find(IMAGE_HASH) != imageHashes.end()) {
      OUTPUT_FILE << "Found duplicates:\n"
                  << IMAGE_PATH << "\n"
                  << imageHashes[IMAGE_HASH] << "\n\n";
      continue;
    }

    for (auto &pair : imageHashes) {
      int hammingDistance;
      if ((hammingDistance = ph_hamming_distance(pair.first, IMAGE_HASH)) >
          THRESHOLD)
        continue;

      OUTPUT_FILE << "Found match of " << hammingDistance << "\n"
                  << IMAGE_PATH << "\n"
                  << pair.second << "\n\n";
    }

    imageHashes[IMAGE_HASH] = IMAGE_PATH;
  }

  for (auto &pair : imageHashes) {
    LOG_FILE << pair.first << ": " << pair.second << "\n";
  }

  OUTPUT_FILE.close();
  return 0;
}

bool isImage(const std::filesystem::path &_path) {
  if (std::filesystem::is_regular_file(_path) == false) return false;

  const std::string FILE_EXT = _path.extension().string();
  return (std::find(IMAGE_EXTS.begin(), IMAGE_EXTS.end(), FILE_EXT) !=
          IMAGE_EXTS.end());
}

std::string shiftArgs(int *argc, char **argv[]) {
  if (*argc == 0) return "";

  const std::string ARGUMENT = (*argv)[0];
  (*argc)--;
  (*argv)++;
  return ARGUMENT;
}
