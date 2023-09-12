#include "Core.h"

#include <cmath>

#include "config.h"

#include "Modules/Slaves/InterruptController.h"

Sim::Core::Core(sc_module_name nm, size_t id, size_t memorySize, size_t numberOfCores)
    : sc_module(nm)
    , id_(id)
    , memorySize_(memorySize)
    , numberOfCores_(numberOfCores) {
    this->memory_ = new uint8_t[memorySize];
    SC_THREAD(mainThread);
    sensitive << clk_i.pos();
}

Sim::Core::~Core() {
    delete[] this->memory_;
}

void Sim::Core::mainThread() {
    while (true) {
        if (this->IR_i.read() & Interrupt::IConfig) {
            this->state_ = State::Configuring;
            this->currentLayerNumber_ = 0;
        }
        if (this->IR_i.read() & Interrupt::ILayer) {
            this->state_ = State::LoadingInput;
            this->accumulator_ = 0;
            this->currentInputNumber_ = 0;
            this->currentNeuronNumber_ = 0;
            this->resultAddressOffset_ = 0;
            this->currentLayerNumber_++;
        }
        if (this->IR_i.read() & Interrupt::IFinish) {
            this->currentLayerNumber_ = 0;
        }
        switch (state_) {
            case State::Configuring: {
                uint64_t tmp;
                this->bus_port->read(this->id_, config::addresses::shared_memory_start + config::addresses::area_2_start, &tmp);
                this->numberOfLayers_ = tmp & 0xFFFFFFFF;
                this->layersNeuronCount_localMemPtr_ = reinterpret_cast<uint32_t*>(this->memory_);
                this->bus_port->burst_read(this->id_,
                                           config::addresses::shared_memory_start + config::addresses::area_2_start + 4,
                                           reinterpret_cast<uint64_t*>(this->memory_),
                                           std::ceil(this->numberOfLayers_ / 2.0));  // reading this->numberOfLayers_ number of 32bit values into local memory

                // Reading Layers Info (address of the begining of weights, address of the begining of results, number of neurons to calculate, address of the inputs(output of the previous layer))
                this->layersInfo_localMemPtr_ = reinterpret_cast<LayerInfo*>(this->memory_ + (4 * this->numberOfLayers_));
                for (uint32_t i = 0; i < this->numberOfLayers_ - 1; i++) {
                    this->bus_port->burst_read(this->id_,
                                               config::addresses::shared_memory_start + config::addresses::area_2_start + 4 + (this->numberOfLayers_ * 4) + (((i * this->numberOfCores_) + this->id_) * 4 * 4),
                                               reinterpret_cast<uint64_t*>(this->memory_ + (4 * this->numberOfLayers_) + (4 * 4 * i)),
                                               2);
                }
                this->inputs_localMemPtr_ = reinterpret_cast<uint64_t*>(this->memory_ + (4 * this->numberOfLayers_) + (4 * 4 * this->numberOfLayers_));
                //this->inputsStartLocalAddr_ = (this->numberOfLayers_ + 4 * this->numberOfLayers_) * 4;
                this->bus_port->read(this->id_, config::addresses::shared_memory_start, &tmp, true);
                tmp = (tmp & 0xFFFFFFFF) + 1;
                if (tmp != this->numberOfCores_) {
                    this->bus_port->write(this->id_, config::addresses::shared_memory_start, tmp, 0x0F, false);
                } else {
                    this->bus_port->write(this->id_, config::addresses::shared_memory_start, 0x1'0000'0000, false);  // writing 1 in number of
                    this->bus_port->write(this->id_, config::addresses::shared_memory_start + 8, 1, 0xFFFF'FFFF, false);
                    this->bus_port->write(this->id_, config::addresses::interrupts::ILayer, 1);
                }
                this->state_ = State::Waiting;
            } break;
            case State::Waiting: {
            } break;
            case State::LoadingInput: {
                this->bus_port->burst_read(this->id_,
                                           this->layersInfo_localMemPtr_[this->currentLayerNumber_ - 1].inputAddress,
                                           this->inputs_localMemPtr_,
                                           std::ceil(this->layersNeuronCount_localMemPtr_[this->currentLayerNumber_ - 1] / 4.0));
                this->state_ = State::Calculating;
            } break;
            case State::Calculating: {
                uint64_t weights;
                uint64_t inputs = this->inputs_localMemPtr_[this->currentInputNumber_ / 4];
                //uint64_t inputs = *reinterpret_cast<uint64_t*>(this->memory_ + this->inputsStartLocalAddr_ + (this->currentInputNumber_ * 2));
                this->bus_port->read(this->id_, (this->layersInfo_localMemPtr_[this->currentLayerNumber_ - 1].weightsStartAddress + (this->currentNeuronNumber_ * this->layersNeuronCount_localMemPtr_[this->currentLayerNumber_ - 1] * 2) + (this->currentInputNumber_ * 2)), &weights);
                this->currentInputNumber_ += 4;
                // If number of neurons in a layer is not devidable by 4 and we are on the last pack than zero out missing weights and inputs
                if (this->currentInputNumber_ > this->layersNeuronCount_localMemPtr_[this->currentLayerNumber_]) {
                    uint8_t mod = this->layersNeuronCount_localMemPtr_[this->currentLayerNumber_] % 4;
                    uint64_t mask = 0xFFFF'FFFF'FFFF'FFFF >> (16 * mod);
                    weights &= mask;
                    inputs &= mask;
                }
                // Calculating
                int16_t* weightsPtr = reinterpret_cast<int16_t*>(&weights);
                int16_t* inputsPtr = reinterpret_cast<int16_t*>(&inputs);
                for (auto i = 0; i < 4; i++) {
                    /*int16_t weight = (weights & (0xFFFF << (i * 16))) >> (i * 16);
                    int16_t input = (inputs & (0xFFFF << (i * 16))) >> (i * 16);*/
                    int32_t weight = weightsPtr[i];
                    int32_t input = inputsPtr[i];
                    int32_t res = (weight * input) >> 12;
                    this->accumulator_ += res;
                }
                for (int i = 0; i < 32; i++) {
                    wait();  // move time
                }

                if (this->currentInputNumber_ < this->layersNeuronCount_localMemPtr_[this->currentLayerNumber_ - 1])
                    break;
                // Neuron calculation finished

                // Sigmoid (logistic) activation function calculated using tanh.
                // Number is converted to floating point. Then function applied. And result is converted back to fixed point.
                // This is the easiest way I can think of right now.
                this->result_[this->currentNeuronNumber_ % 4] = (0.5f + 0.5f * std::tanh((this->accumulator_ / 4096.0f) * 0.5f)) * 4096.0f;
                this->currentInputNumber_ = 0;
                this->accumulator_ = 0;

                // Batching result writing
                if (this->currentNeuronNumber_ % 4 == 3) {
                    this->bus_port->write(this->id_, this->layersInfo_localMemPtr_[this->currentLayerNumber_ - 1].resultsStartAddress + this->resultAddressOffset_, *reinterpret_cast<uint64_t*>(this->result_), 0xFF, false);
                    this->resultAddressOffset_ += 4 * 2;
                }

                // Finished all allocated nurons in current layer
                if (this->currentNeuronNumber_ == this->layersInfo_localMemPtr_[this->currentLayerNumber_ - 1].numberOfNeuronsToCalculate) {
                    if (this->currentNeuronNumber_ % 4 != 3) {
                        // Writing remaining result
                        this->bus_port->write(this->id_,
                                              this->layersInfo_localMemPtr_[this->currentLayerNumber_ - 1].resultsStartAddress + this->resultAddressOffset_,
                                              *reinterpret_cast<uint64_t*>(this->result_),
                                              (0xFF >> ((4 - (this->currentNeuronNumber_ % 4)) * 2)),
                                              false);
                    }

                    uint64_t count;
                    this->bus_port->read(this->id_, config::addresses::shared_memory_start + (4 * 1), &count, true);
                    count &= 0xFFFF'FFFF;
                    count += this->layersInfo_localMemPtr_[this->currentLayerNumber_ - 1].numberOfNeuronsToCalculate;
                    if (count != this->layersNeuronCount_localMemPtr_[this->currentLayerNumber_]) {
                        this->bus_port->write(this->id_, config::addresses::shared_memory_start + (4 * 1), count, 0x0F, false);
                    } else {
                        // All neurons in the curent layer have been calculated
                        this->bus_port->write(this->id_, config::addresses::shared_memory_start + (4 * 1), 0, 0x0F, false);
                        this->bus_port->write(this->id_, (((this->currentLayerNumber_ + 1) < this->numberOfLayers_) ? config::addresses::interrupts::ILayer : config::addresses::interrupts::IFinish), 1, 0xFF, false);
                    }
                    this->state_ = State::Waiting;
                }

                this->currentNeuronNumber_++;
            } break;
        }
        wait();
    }
}
