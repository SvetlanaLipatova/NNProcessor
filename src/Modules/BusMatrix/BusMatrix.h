#pragma once
#include <deque>
#include <unordered_map>

#include "IBusMatrix.h"

namespace Sim {

    class BusMatrix : public sc_core::sc_module,
                      public IBusMatrix {
    public:
        sc_core::sc_in<bool> clk_i;
        sc_core::sc_port<IBusMatrixSlave, 0, SC_ZERO_OR_MORE_BOUND> slave_port;

        SC_CTOR(BusMatrix);

        void main_action();

        // IBusMatrix interface functions
        virtual void read(size_t masterId, uint32_t addr, uint64_t* data, bool lock = false) override;
        virtual void write(size_t masterId, uint32_t addr, const uint64_t data, uint8_t mask = 0xFF, bool lock = false) override;
        virtual void burst_read(size_t masterId, uint32_t addr, uint64_t* data, size_t length, bool lock = false) override;
        virtual void burst_write(size_t masterId, uint32_t addr, const uint64_t* data, size_t length, uint8_t mask = 0xFF, bool lock = false) override;

    private:
        struct Request {
            size_t masterId;
            uint32_t addr;
            uint64_t* data;
            size_t length;
            uint8_t mask;
            bool lock;
            bool isRead;
            sc_core::sc_event doneEvent;
        };

        std::deque<size_t> requestsQueue_;
        std::unordered_map<size_t, Request*> requests_;
        bool isLocked = false;
        size_t lockedMasterId;

        Request* getRequestElement(size_t);
    };

}  // namespace Sim
