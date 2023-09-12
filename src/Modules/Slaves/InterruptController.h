#pragma once

#include "Modules/BusMatrix/IBusMatrix.h"

namespace Sim {

    namespace Interrupt {
        enum {
            IConfig = 0b0000'0000'0000'0001,
            ILayer = 0b0000'0000'0000'0010,
            IFinish = 0b0000'0000'0000'0100,
        };
    }

    class InterruptController : public sc_core::sc_module,
                                public Sim::IBusMatrixSlave {
    public:
        sc_core::sc_in<bool> clk_i;
        sc_export<sc_signal<uint16_t>> IR_o;
        sc_signal<uint16_t> IR;

        InterruptController(sc_module_name nm, uint32_t addrStart);
        ~InterruptController() = default;
        SC_HAS_PROCESS(InterruptController);

        virtual void read(uint32_t addr, uint64_t* data, size_t length) override;
        virtual void write(uint32_t addr, const uint64_t* data, size_t length, uint8_t mask) override;

        virtual uint32_t beginAddress() const override;
        virtual uint32_t endAddress() const override;

    private:
        void mainThread();

        uint32_t addrStart_;

        uint16_t next_ = 0;
    };

}  // namespace Sim
