
#include "./include/sisIO.hpp"
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <pHash.h>
#include <string>
#include <unordered_map>
#include <vector>

#define DEFAULT_THRESHOLD 15

const std::vector<std::string> IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".webp"};
int threshold = DEFAULT_THRESHOLD;
const std::string OUTPUT_FILE = "./Dupim.output.txt";
const std::string LOG_FILE = "./Dupim.log.txt";
const std::string TEMP_FILE = "./Dupim.temp.jpg";

SisIO io = SisIO("");
SisIO OutputIO = SisIO(OUTPUT_FILE);
SisIO LogIO = SisIO(LOG_FILE);

bool isImageFile(const std::filesystem::path &filePath);
std::string shiftArgs(int *argc, char **argv[]);
bool convert(const std::filesystem::path &filePath);
void cleanup();
void compute(const std::filesystem::path &filePath,
             std::unordered_map<ulong64, std::filesystem::path> &hashMap);
void logResult(const int hammingDistace, const std::filesystem::path &a,
               const std::filesystem::path &b);

int main(int argc, char *argv[]) {

    if (argc < 2) {

        io.output(SisIO::messageType::error, "Invalid arguments.");
        io.output(SisIO::messageType::info,
                  "Usage: <program_name> <target_directory> <threshold_value>");
        return 1;
    }

    const std::string PROGRAM = shiftArgs(&argc, &argv);
    const std::string TARGET = shiftArgs(&argc, &argv);
    const std::string THRESHOLD_INPUT = shiftArgs(&argc, &argv);

    if (std::filesystem::is_directory(TARGET) == false) {
        io.output(SisIO::messageType::error,
                  "Could not find target directory.");
        return 4;
    }

    if (THRESHOLD_INPUT.empty() == false) {
        try {
            threshold = std::stoi(THRESHOLD_INPUT);
        } catch (std::invalid_argument e) {
            io.output(SisIO::messageType::error,
                      "Could not convert given threshold number");
            return 5;
        }
    }

    if (std::filesystem::exists(LOG_FILE)) {
        io.output(SisIO::messageType::error,
                  "Found an existing log file at " + LOG_FILE +
                      ". Please remove it to run the program.");
        return 6;
    }

    if (std::filesystem::exists(OUTPUT_FILE)) {
        io.output(SisIO::messageType::error,
                  "Found an existing output file at " + OUTPUT_FILE +
                      ". Please remove it to run the program.");
        return 7;
    }

    std::unordered_map<ulong64, std::filesystem::path> imageHashes;
    compute(TARGET, imageHashes);

    cleanup();
    return 0;
}

bool isImageFile(const std::filesystem::path &filePath) {
    if (std::filesystem::is_regular_file(filePath) == false)
        return false;

    const std::string FILE_EXT = filePath.extension().string();
    return (std::find(IMAGE_EXTS.begin(), IMAGE_EXTS.end(), FILE_EXT) !=
            IMAGE_EXTS.end());
}

bool convert(const std::filesystem::path &filePath) {
    const std::string EXT = filePath.extension().string();
    if (EXT == "jpg" || EXT == "jpeg") {
        return true;
    }

    cv::Mat image = cv::imread(filePath.string(), cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        return false;
    }

    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);

    if (!cv::imwrite(TEMP_FILE, image, compression_params)) {
        return false;
    }

    return true;
}

void compute(const std::filesystem::path &filePath,
             std::unordered_map<ulong64, std::filesystem::path> &hashMap) {

    for (auto &entry :
         std::filesystem::recursive_directory_iterator(filePath)) {
        if (isImageFile(entry.path()) == false)
            continue;

        const bool validFile = convert(entry.path());
        if (!validFile)
            continue;

        const std::filesystem::path IMAGE_PATH =
            std::filesystem::canonical(entry.path());

        ulong64 imageHash;
        const std::string EXT = entry.path().extension().string();

        if (EXT == "jpg" || EXT == "jpeg")
            ph_dct_imagehash(IMAGE_PATH.c_str(), imageHash);
        else
            ph_dct_imagehash(TEMP_FILE.c_str(), imageHash);

        LogIO.log(SisIO::messageType::info,
                  entry.path().string() + " => " + std::to_string(imageHash),
                  "Dupim");

        for (auto &pair : hashMap) {
            const int hammingDistance =
                ph_hamming_distance(pair.first, imageHash);

            if (hammingDistance < threshold)
                logResult(hammingDistance, pair.second, entry.path());
        }

        hashMap[imageHash] = entry.path();
    }
}

void logResult(const int hammingDistace, const std::filesystem::path &a,
               const std::filesystem::path &b) {
    const std::string title =
        hammingDistace != 0
            ? "Found similarity of " + std::to_string(hammingDistace)
            : "Found duplicates";

    const std::string message =
        title + "\n\t" + a.string() + "\n\t" + b.string();
    OutputIO.log(SisIO::messageType::warn, message, "Dupim");
    io.output(SisIO::messageType::warn, message);
}

void cleanup() {
    if (std::filesystem::exists(TEMP_FILE))
        std::filesystem::remove(TEMP_FILE);
}

std::string shiftArgs(int *argc, char **argv[]) {
    if (*argc == 0)
        return "";

    const std::string ARGUMENT = (*argv)[0];
    (*argc)--;
    (*argv)++;
    return ARGUMENT;
}
