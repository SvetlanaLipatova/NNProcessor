#pragma once
#include "systemc.h"

namespace Sim {

    class IBusMatrix : virtual public sc_core::sc_interface {
    public:
        virtual void read(size_t masterId, uint32_t addr, uint64_t* data, bool lock = false) = 0;
        virtual void write(size_t masterId, uint32_t addr, const uint64_t data, uint8_t mask = 0xFF, bool lock = false) = 0;
        virtual void burst_read(size_t masterId, uint32_t addr, uint64_t* data, size_t length, bool lock = false) = 0;
        virtual void burst_write(size_t masterId, uint32_t addr, const uint64_t* data, size_t length, uint8_t mask = 0xFF, bool lock = false) = 0;

        virtual ~IBusMatrix() = default;
    };

    class IBusMatrixSlave : virtual public sc_core::sc_interface {
    public:
        virtual void read(uint32_t addr, uint64_t* data, size_t length) = 0;
        virtual void write(uint32_t addr, const uint64_t* data, size_t length, uint8_t mask) = 0;

        virtual uint32_t beginAddress() const = 0;
        virtual uint32_t endAddress() const = 0;

        virtual ~IBusMatrixSlave() = default;
    };

}
