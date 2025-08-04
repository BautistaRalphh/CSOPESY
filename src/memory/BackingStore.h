#pragma once

#include <string>
#include <vector>
#include <cstdint>

class BackingStore {
public:
    static void pageOut(const std::string& processName, int pageNumber, const std::vector<uint8_t>& pageData);
    static std::vector<uint8_t> pageIn(const std::string& processName, int pageNumber);
    static void displayStatus();
};