#include "IOController.h"

#include <cmath>

#include "config.h"

#include "Modules/Slaves/InterruptController.h"

Sim::IOController::IOController(sc_module_name nm,
                                size_t id,
                                const GetConfigFunction& getConfigFunc,
                                const GetNextInputFunction& getNextInputFunc,
                                const OutputResultFunction& outputResultFunc)
    : sc_module(nm)
    , id_(id)
    , getConfigFunc_(getConfigFunc)
    , getNextInputFunc_(getNextInputFunc)
    , outputResultFunc_(outputResultFunc) {
    SC_THREAD(mainThread);
    sensitive << this->clk_i.pos();
}

void Sim::IOController::mainThread() {
    while (true) {
        if (this->IR_i.read() & Interrupt::IFinish) {
            this->state_ = State::DataOutput;
        }
        // We don't care about aligment of these pointers since SharedMemory will cast it to uint8_t*

        switch (this->state_) {
            case State::Configuring: {
                this->bus_port->write(this->id_, config::addresses::shared_memory_start, 0);
                // First 32bytes of config is the start of last layer output and only reqired in IOController.
                // Thats why we add +4 to a pointer in burst_write function
                auto config = this->getConfigFunc_();
                uint32_t* tmpConfig = reinterpret_cast<uint32_t*>(config.data());
                this->inputStartAddress_ = tmpConfig[3];
                this->outputStartAddress_ = tmpConfig[0];
                // math trickery to get size in bytes devidable by 8 so that we don't get into unallocated area when we use the 64bit pointer
                this->tmpOutput_.resize(std::ceil(tmpConfig[4 + tmpConfig[4]] / 4.0) * 4);
                this->bus_port->burst_write(this->id_, config::addresses::shared_memory_start + config::addresses::area_1_start, reinterpret_cast<const uint64_t*>(config.data() + 4), config.size() / 8);
                auto& data = this->getNextInputFunc_();
                this->bus_port->burst_write(this->id_, this->inputStartAddress_, reinterpret_cast<const uint64_t*>(data.data()), data.size() / 4);
                this->bus_port->write(this->id_, config::addresses::interrupts::IConfig, 1);
                this->state_ = State::Waiting;
            } break;
            case State::DataInput: {
                auto& data = this->getNextInputFunc_();
                this->bus_port->burst_write(this->id_, this->inputStartAddress_, reinterpret_cast<const uint64_t*>(data.data()), data.size() / 4);
                this->bus_port->write(this->id_, config::addresses::interrupts::ILayer, 1);
                this->state_ = State::Waiting;
            } break;
            case State::DataOutput: {
                this->bus_port->burst_read(this->id_, this->outputStartAddress_, reinterpret_cast<uint64_t*>(this->tmpOutput_.data()), this->tmpOutput_.size() / 4);
                this->outputResultFunc_(this->tmpOutput_);
                this->state_ = State::DataInput;
            } break;
        }
        wait();
    }
}
