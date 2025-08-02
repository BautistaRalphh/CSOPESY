#pragma once

#include "memory/IMemoryAllocator.h"
#include "core/Process.h"

#include <unordered_map>
#include <list>
#include <set>
#include <string>
#include <vector>

class DemandPagingAllocator : public IMemoryAllocator {
public:
    enum class PageReplacementPolicy {
        NONE,
        FIFO,
        LRU
    };

    DemandPagingAllocator(size_t totalMemorySize, size_t frameSize, PageReplacementPolicy policy);

    void* allocate(std::shared_ptr<Process> process) override;
    void deallocate(std::shared_ptr<Process> process) override;
    void visualizeMemory() const override;
    bool accessMemory(const std::string& pid, int pageNumber);

    // Memory statistics methods
    int getPagesInPhysicalMemory(const std::string& pid) const;
    int getPagesInBackingStore(const std::string& pid) const;

private:
    size_t frameSize;
    size_t totalFrames;
    PageReplacementPolicy policy;

    struct PageInfo {
        std::string pid;
        int pageNumber;
    };


    std::vector<PageInfo> frameTable;
    std::set<int> freeFrames;

    std::unordered_map<std::string, std::unordered_map<int, int>> pageTables;

    std::list<PageInfo> fifoQueue;

    std::unordered_map<std::string, std::unordered_map<int, long long>> lruTimestamps;

    void handlePageFault(const std::string& pid, int pageNumber);

    int evictPage();

    void writePageToStore(const std::string& pid, int pageNumber);
    void readPageFromStore(const std::string& pid, int pageNumber);
};
