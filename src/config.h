#pragma once
#include <cstdint>

namespace config {

    namespace addresses {
        constexpr uint32_t area_1_start = 2 * 4;
        constexpr uint32_t area_2_start = area_1_start + 3 * 4;
        constexpr uint32_t shared_memory_start = 0;
        constexpr uint32_t interrupt_controller_start = 0xFFFF'FFEF;
        namespace interrupts {
            constexpr uint32_t IConfig = interrupt_controller_start + 0;
            constexpr uint32_t ILayer = interrupt_controller_start + 1;
            constexpr uint32_t IFinish = interrupt_controller_start + 2;
        }
    }

    namespace sizes {
        constexpr uint32_t local_memory = 1024000;
        constexpr uint32_t shared_memory = 41943040;
    }

}
