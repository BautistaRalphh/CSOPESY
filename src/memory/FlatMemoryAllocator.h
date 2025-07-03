#pragma once

#include "IMemoryAllocator.h"
#include "core/Process.h"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <memory>

class FlatMemoryAllocator : public IMemoryAllocator {
public:
    FlatMemoryAllocator(size_t maximumSize)
        : maximumSize(maximumSize), memory(maximumSize, '.'), allocationMap(maximumSize, false) {}

    void* allocate(std::shared_ptr<Process> process) override {
        size_t size = process->getMemoryRequired(); 
        for (size_t i = 0; i <= maximumSize - size; ++i) {
            if (canAllocateAt(i, size)) {
                allocateAt(i, size);
                allocations[process->getProcessName()] = { i, size };
                return &memory[i];
            }
        }
        return nullptr;
    }

    void deallocate(std::shared_ptr<Process> process) override {
        auto it = allocations.find(process->getProcessName());
        if (it != allocations.end()) {
            size_t start = it->second.first;
            size_t size = it->second.second;
            for (size_t i = start; i < start + size; ++i) {
                allocationMap[i] = false;
                memory[i] = '.';
            }
            allocations.erase(it);
        }
    }

    void visualizeMemory() const override {
        for (size_t i = 0; i < memory.size(); ++i) {
            std::cout << memory[i];
            if ((i + 1) % 64 == 0) std::cout << '\n';
        }
        std::cout << '\n';
    }

private:
    size_t maximumSize;
    std::vector<char> memory;
    std::vector<bool> allocationMap;
    std::unordered_map<std::string, std::pair<size_t, size_t>> allocations;

    bool canAllocateAt(size_t index, size_t size) const {
        for (size_t i = index; i < index + size; ++i) {
            if (allocationMap[i]) return false;
        }
        return true;
    }

    void allocateAt(size_t index, size_t size) {
        for (size_t i = index; i < index + size; ++i) {
            allocationMap[i] = true;
            memory[i] = '#';
        }
    }
};
