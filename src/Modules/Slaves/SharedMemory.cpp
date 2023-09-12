#include "SharedMemory.h"

#include <cstring>

Sim::SharedMemory::SharedMemory(sc_module_name nm, uint32_t addrStart, uint32_t addrEnd)
    : sc_module(nm)
    , addrStart(addrStart)
    , addrEnd(addrEnd) {
    this->memory_ = new uint8_t[addrEnd - addrStart];
};

Sim::SharedMemory::~SharedMemory() {
    delete[] this->memory_;
}

void Sim::SharedMemory::read(uint32_t addr, uint64_t* data, size_t length) {
    std::memcpy(reinterpret_cast<uint8_t*>(data), (this->memory_ + addr), length * 8);
}

void Sim::SharedMemory::write(uint32_t addr, const uint64_t* data, size_t length, uint8_t mask) {
    for (size_t i = 0; i < length; i++) {
        for (auto j = 0; j < 8; j++) {
            if (mask & (1 << j)) {
                this->memory_[addr + (i * 8) + j] = reinterpret_cast<const uint8_t*>(data + i)[j];
            }
        }
    }
}

uint32_t Sim::SharedMemory::beginAddress() const {
    return this->addrStart;
}

uint32_t Sim::SharedMemory::endAddress() const {
    return this->addrEnd;
}
