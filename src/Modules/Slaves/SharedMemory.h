#pragma once

#include "systemc.h"

#include "Modules/BusMatrix/IBusMatrix.h"

namespace Sim {

    class SharedMemory : public sc_core::sc_module,
                         public Sim::IBusMatrixSlave {
    public:
        SharedMemory(sc_module_name nm, uint32_t addrStart, uint32_t addrEnd);
        ~SharedMemory();
        SC_HAS_PROCESS(SharedMemory);

        virtual void read(uint32_t addr, uint64_t* data, size_t length) override;
        virtual void write(uint32_t addr, const uint64_t* data, size_t length, uint8_t mask) override;

        virtual uint32_t beginAddress() const override;
        virtual uint32_t endAddress() const override;

    private:
        uint8_t* memory_;
        uint32_t addrStart;
        uint32_t addrEnd;
    };

}  // namespace Sim
