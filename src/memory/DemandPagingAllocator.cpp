#include "memory/DemandPagingAllocator.h"
#include "memory/BackingStore.h"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <filesystem>

DemandPagingAllocator::DemandPagingAllocator(size_t totalMemorySize, size_t frameSize, PageReplacementPolicy policy)
    : frameSize(frameSize),
      totalFrames(totalMemorySize / frameSize),
      policy(policy),
      frameTable(totalFrames, PageInfo{"", -1}) {
    for (int i = 0; i < totalFrames; ++i) {
        freeFrames.insert(i);
    }
}

void* DemandPagingAllocator::allocate(std::shared_ptr<Process> process) {
    std::string pid = process->getPid();
    uint32_t memoryRequired = process->getMemoryRequired();
    uint32_t pagesNeeded = (memoryRequired + frameSize - 1) / frameSize; // Calculate pages from memory requirement
    
    // Check if we have enough free frames
    if (freeFrames.size() < pagesNeeded) {
        //std::cout << "[MEMORY] Allocation failed for process " << pid << " - not enough free frames. Need: " << pagesNeeded << ", Available: " << freeFrames.size() << std::endl;
        return nullptr;
    }
    
    // Allocate frames for this process
    pageTables[pid] = std::unordered_map<int, int>();
    
    auto it = freeFrames.begin();
    for (uint32_t i = 0; i < pagesNeeded; ++i) {
        int frameIndex = *it;
        it = freeFrames.erase(it);
        
        // Map page i to frame frameIndex
        pageTables[pid][i] = frameIndex;
        frameTable[frameIndex] = PageInfo{pid, static_cast<int>(i)};
        
        // Add to FIFO queue if using FIFO policy
        if (policy == PageReplacementPolicy::FIFO) {
            fifoQueue.push_back(PageInfo{pid, static_cast<int>(i)});
        }
    }
    
    // Update the process to reflect actual allocated pages
    process->setMemory(memoryRequired, pagesNeeded);
    return reinterpret_cast<void*>(1); // Return non-null to indicate success
}

void DemandPagingAllocator::deallocate(std::shared_ptr<Process> process) {
    std::string pid = process->getPid();
    if (pageTables.count(pid)) {
        for (auto& entry : pageTables[pid]) {
            int frameIndex = entry.second;
            freeFrames.insert(frameIndex);
            frameTable[frameIndex] = PageInfo{"", -1};
        }
        pageTables.erase(pid);
        fifoQueue.remove_if([&](const PageInfo& p) { return p.pid == pid; });
        lruTimestamps.erase(pid);
        
        uint32_t memoryRequired = process->getMemoryRequired();
        process->setMemory(memoryRequired, 0);
    }
}

void DemandPagingAllocator::visualizeMemory() const {
    std::cout << "Memory Visualization:\n";

    std::cout << "Free Frames: ";
    for (const auto& frame : freeFrames) {
        std::cout << frame << " ";
    }
    std::cout << std::endl;

    std::cout << "Allocated Pages: \n";
    for (const auto& process : pageTables) {
        for (const auto& page : process.second) {
            std::cout << "Process " << process.first << " has page " << page.first
                      << " in frame " << page.second << std::endl;
        }
    }
}

bool DemandPagingAllocator::accessMemory(const std::string& pid, int pageNumber) {
    if (pageTables[pid].count(pageNumber)) {
        
        if (policy == PageReplacementPolicy::LRU) {
            lruTimestamps[pid][pageNumber] = std::chrono::steady_clock::now().time_since_epoch().count();
        }
        return true;
    }

    handlePageFault(pid, pageNumber);
    return false;
}

void DemandPagingAllocator::handlePageFault(const std::string& pid, int pageNumber) {
    int frameIndex;
    
    if (!freeFrames.empty()) {
        frameIndex = *freeFrames.begin();
        freeFrames.erase(freeFrames.begin());
    } else {
        frameIndex = evictPage();
    }

    readPageFromStore(pid, pageNumber);

    frameTable[frameIndex] = PageInfo{pid, pageNumber};
    pageTables[pid][pageNumber] = frameIndex;

    if (policy == PageReplacementPolicy::FIFO) {
        fifoQueue.push_back(PageInfo{pid, pageNumber});
    } else if (policy == PageReplacementPolicy::LRU) {
        lruTimestamps[pid][pageNumber] = std::chrono::steady_clock::now().time_since_epoch().count();
    }
}

int DemandPagingAllocator::evictPage() {
    PageInfo evicted;
    if (policy == PageReplacementPolicy::FIFO) {
        evicted = fifoQueue.front();
        fifoQueue.pop_front();
    } else if (policy == PageReplacementPolicy::LRU) {
        long long oldestTime = LLONG_MAX;
        for (const auto& [pid, pageMap] : lruTimestamps) {
            for (const auto& [page, time] : pageMap) {
                if (time < oldestTime) {
                    oldestTime = time;
                    evicted = {pid, page};
                }
            }
        }
        lruTimestamps[evicted.pid].erase(evicted.pageNumber);
    } else {
        std::cerr << "No page replacement policy set. Cannot evict." << std::endl;
        std::exit(1);
    }

    int frameIndex = pageTables[evicted.pid][evicted.pageNumber];

    writePageToStore(evicted.pid, evicted.pageNumber);

    pageTables[evicted.pid].erase(evicted.pageNumber);
    frameTable[frameIndex] = PageInfo{"", -1};

    return frameIndex;
}


void DemandPagingAllocator::writePageToStore(const std::string& pid, int pageNumber) {
    std::vector<uint8_t> dummyData(frameSize, 0xFF);
    BackingStore::pageOut(pid, pageNumber, dummyData);
}

void DemandPagingAllocator::readPageFromStore(const std::string& pid, int pageNumber) {
    std::vector<uint8_t> data = BackingStore::pageIn(pid, pageNumber);
}
