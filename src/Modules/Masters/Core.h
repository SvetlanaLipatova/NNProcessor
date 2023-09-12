#pragma once

#include "systemc.h"

#include "Modules/BusMatrix/IBusMatrix.h"

namespace Sim {

    class Core : public sc_core::sc_module {
    public:
        sc_core::sc_in<bool> clk_i;
        sc_core::sc_in<uint16_t> IR_i;
        sc_core::sc_port<IBusMatrix> bus_port;

        Core(sc_module_name nm, size_t id, size_t memorySize, size_t numberOfCores);
        ~Core();
        SC_HAS_PROCESS(Core);

    private:
        void mainThread();

        enum class State { Configuring,
                           Waiting,
                           LoadingInput,
                           Calculating };

        size_t id_;
        State state_;

        uint8_t* memory_;
        size_t memorySize_;
        size_t numberOfCores_;
        uint32_t numberOfLayers_;

        //uint32_t inputsStartLocalAddr_;

        uint32_t currentLayerNumber_;        // layer number that is currently being calculated
        uint32_t currentInputNumber_;  // input neuron number (input of the curent layer)
        uint32_t currentNeuronNumber_;       // neuron number that is currently being calculated (local for each core)
        uint32_t resultAddressOffset_;
        int32_t accumulator_;
        int16_t result_[4];

        // Helper pointers
        uint32_t* layersNeuronCount_localMemPtr_;
        struct LayerInfo {
            uint32_t weightsStartAddress;
            uint32_t resultsStartAddress;
            uint32_t numberOfNeuronsToCalculate;
            uint32_t inputAddress;
        };
        // An array in local memory that contains information about shared memory adresses and number of neurons this core is allocated to calculate for each layer
        LayerInfo* layersInfo_localMemPtr_;
        uint64_t* inputs_localMemPtr_;
    };

}
