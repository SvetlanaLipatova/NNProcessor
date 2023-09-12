#pragma once
#include <functional>
#include <vector>

#include "systemc.h"

#include "Core/Model.h"

#include "Modules/BusMatrix/IBusMatrix.h"

namespace Sim {

    using GetConfigFunction = std::function<std::vector<uint8_t>()>;
    using GetNextInputFunction = std::function<const std::vector<uint16_t>&()>;
    using OutputResultFunction = std::function<void(const std::vector<uint16_t>&)>;

    class IOController : public sc_core::sc_module {
    public:
        sc_core::sc_in<bool> clk_i;
        sc_core::sc_in<uint16_t> IR_i;
        sc_core::sc_port<IBusMatrix> bus_port;

        IOController(sc_module_name nm,
                     size_t id,
                     const GetConfigFunction& getConfigFunc,
                     const GetNextInputFunction& getNextInputFunc,
                     const OutputResultFunction& outputResultFunc);
        SC_HAS_PROCESS(IOController);

    private:
        void mainThread();

        enum class State { Configuring,
                           Waiting,
                           DataInput,
                           DataOutput };

        size_t id_;
        State state_ = State::Configuring;

        uint32_t inputStartAddress_;
        uint32_t outputStartAddress_;
        std::vector<uint16_t> tmpOutput_;

        GetConfigFunction getConfigFunc_;
        GetNextInputFunction getNextInputFunc_;
        OutputResultFunction outputResultFunc_;
    };

}
