#pragma once

#include <memory>
#include "core/Process.h"

class IMemoryAllocator {
public:
    virtual ~IMemoryAllocator() = default;

    virtual void* allocate(std::shared_ptr<Process> process) = 0;
    virtual void deallocate(std::shared_ptr<Process> process) = 0;

    virtual void visualizeMemory() const = 0;
};
