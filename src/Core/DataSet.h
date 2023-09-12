#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Core {

    struct Example {
        std::vector<uint16_t> input;
        uint16_t expectedOutput;
    };

    using Dataset = std::vector<Example>;

    // Loads Dataset from a mnist formated file and applies normalization
    Dataset loadDataset_mnist(const std::string& dataPath, const std::string& labelsPath);

    Dataset loadDataset_figure(const std::string& dataPath, size_t imageSize);

}
