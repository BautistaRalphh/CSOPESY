#include "memory/BackingStore.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>

void BackingStore::pageOut(const std::string& processName, int pageNumber, const std::vector<uint8_t>& pageData) {
   //std::cout << "[DEBUG] BackingStore::pageOut called for process " << processName << " page " << pageNumber << std::endl;
    std::ofstream outFile("csopesy-backing-store.txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << processName << " " << pageNumber;
        for (auto byte : pageData) {
            outFile << " " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        outFile << "\n";
        outFile.close();
        //std::cout << "[DEBUG] Successfully wrote to backing store file" << std::endl;
    } else {
        //std::cout << "[DEBUG] Failed to open backing store file for writing" << std::endl;
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
    return {}; 
}

void BackingStore::displayStatus() {
    std::cout << "\n--- Backing Store Status ---" << std::endl;
    std::ifstream inFile("csopesy-backing-store.txt");
    if (!inFile.is_open()) {
        std::cout << "Backing store file not found or could not be opened." << std::endl;
        return;
    }
    
    std::string line;
    int pageCount = 0;
    std::cout << std::left << std::setw(12) << "Process" 
              << std::setw(8) << "Page#" 
              << "Data (first 8 bytes)" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    while (std::getline(inFile, line) && pageCount < 20) { 
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string processName;
        int pageNumber;
        iss >> processName >> pageNumber;
        
        std::cout << std::left << std::setw(12) << processName 
                  << std::setw(8) << pageNumber;
        
        std::string hexByte;
        for (int i = 0; i < 8 && iss >> hexByte; ++i) {
            std::cout << hexByte << " ";
        }
        std::cout << std::endl;
        pageCount++;
    }
    
    if (pageCount == 0) {
        std::cout << "No pages currently in backing store." << std::endl;
    } else {
        inFile.clear();
        inFile.seekg(0);
        int totalPages = 0;
        while (std::getline(inFile, line)) {
            if (!line.empty()) totalPages++;
        }
        std::cout << "\nTotal pages in backing store: " << totalPages << std::endl;
    }
    inFile.close();
}