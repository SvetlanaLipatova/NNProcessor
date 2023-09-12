#include "InterruptController.h"

#include "config.h"

Sim::InterruptController::InterruptController(sc_module_name nm, uint32_t addrStart)
    : sc_module(nm)
    , addrStart_(addrStart) {
    SC_THREAD(mainThread);
    sensitive << this->clk_i.pos();
    this->IR_o(this->IR);
}

void Sim::InterruptController::read(uint32_t addr, uint64_t* data, size_t length) {
    return;
}

void Sim::InterruptController::write(uint32_t addr, const uint64_t* data, size_t length, uint8_t mask) {
    this->next_ |= 1 << (addr - this->addrStart_);
}

uint32_t Sim::InterruptController::beginAddress() const {
    return this->addrStart_;
}

uint32_t Sim::InterruptController::endAddress() const {
    return this->addrStart_ + 16;
}

void Sim::InterruptController::mainThread() {
    while (true) {
        this->IR.write(this->next_);
        this->next_ = 0;
        wait();
    }
}
