#include "memory/BackingStore.h"
#include <fstream>
#include <sstream>
#include <iomanip>

#include <filesystem>

void BackingStore::pageOut(const std::string& processName, int pageNumber, const std::vector<uint8_t>& pageData) {
    std::ofstream outFile("csopesy-backing-store.txt", std::ios::app); // append mode
    if (outFile.is_open()) {
        outFile << processName << " " << pageNumber;
        for (auto byte : pageData) {
            outFile << " " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        outFile << "\n";
        outFile.close();
    }
}

std::vector<uint8_t> BackingStore::pageIn(const std::string& processName, int pageNumber) {
    std::ifstream inFile("csopesy-backing-store.txt");
    std::string line;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string name;
        int page;
        iss >> name >> page;
        if (name == processName && page == pageNumber) {
            std::vector<uint8_t> data;
            std::string hexByte;
            while (iss >> hexByte) {
                data.push_back(static_cast<uint8_t>(std::stoi(hexByte, nullptr, 16)));
            }
            return data;
        }
    }
    return {}; // empty vector = not found
}