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
      frameTable(totalFrames, PageInfo{"", -1}),
      totalPagesPagedIn(0),
      totalPagesPagedOut(0) {
    for (int i = 0; i < totalFrames; ++i) {
        freeFrames.insert(i);
    }
}

void* DemandPagingAllocator::allocate(std::shared_ptr<Process> process) {
    std::string pid = process->getPid();
    uint32_t memoryRequired = process->getMemoryRequired();
    uint32_t pagesNeeded = (memoryRequired + frameSize - 1) / frameSize; 
    
    uint32_t initialPages = std::min(pagesNeeded, static_cast<uint32_t>(1));
    
    if (freeFrames.size() < initialPages) {
        return nullptr;
    }
    
    pageTables[pid] = std::unordered_map<int, int>();
    
    auto it = freeFrames.begin();
    for (uint32_t i = 0; i < initialPages; ++i) {
        int frameIndex = *it;
        it = freeFrames.erase(it);
        
        pageTables[pid][i] = frameIndex;
        frameTable[frameIndex] = PageInfo{pid, static_cast<int>(i)};

        if (policy == PageReplacementPolicy::FIFO) {
            fifoQueue.push_back(PageInfo{pid, static_cast<int>(i)});
        }
    }
    
    for (uint32_t i = initialPages; i < pagesNeeded; ++i) {
        std::vector<uint8_t> dummyData(frameSize, 0xAA + (i % 10)); 
        BackingStore::pageOut(pid, i, dummyData);
        totalPagesPagedOut.fetch_add(1);
        //std::cout << "[DEBUG] Writing page " << i << " of process " << pid << " to backing store" << std::endl;
    }
    
    process->setMemory(memoryRequired, pagesNeeded);
    return reinterpret_cast<void*>(1); 
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
    totalPagesPagedOut.fetch_add(1);
}

void DemandPagingAllocator::readPageFromStore(const std::string& pid, int pageNumber) {
    std::vector<uint8_t> data = BackingStore::pageIn(pid, pageNumber);
    totalPagesPagedIn.fetch_add(1);
}

int DemandPagingAllocator::getPagesInPhysicalMemory(const std::string& pid) const {
    int count = 0;
    auto it = pageTables.find(pid);
    if (it != pageTables.end()) {
        for (const auto& pageEntry : it->second) {
            if (pageEntry.second >= 0) {
                count++;
            }
        }
    }
    return count;
}

int DemandPagingAllocator::getPagesInBackingStore(const std::string& pid) const {
    auto it = pageTables.find(pid);
    if (it != pageTables.end()) {
        int totalPages = it->second.size();
        int pagesInMemory = getPagesInPhysicalMemory(pid);
        return totalPages - pagesInMemory;
    }
    return 0;
}

long long DemandPagingAllocator::getTotalPagesPagedIn() const {
    return totalPagesPagedIn.load();
}

long long DemandPagingAllocator::getTotalPagesPagedOut() const {
    return totalPagesPagedOut.load();
}
