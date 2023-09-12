#pragma once
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace Core {

    struct IModel {
        IModel() : layersCount(0){};
        virtual ~IModel() = default;

        size_t layersCount;                                // Number of layers not including the input layer
        std::vector<size_t> neuronsCount;                  // Number of neurons in each layer. Has the size of `layersCount + 1`
        std::vector<std::vector<uint16_t>> layersWeights;  // layersWeights[i] is a weight matrix (row-major) between (i)th and (i+1)th layer
    };

    struct EmnistModel : public IModel {
        EmnistModel() : IModel(){};
        EmnistModel(const std::string& path);  // Loads model from a file
        virtual ~EmnistModel() = default;

        void loadFromFile(const std::string& path);
    };

    struct FigureModel : public IModel {
        FigureModel() : IModel(){};
        FigureModel(const std::string& info, const std::string& weights);  // Loads model from a file
        virtual ~FigureModel() = default;

        void loadFromFile(const std::string& info, const std::string& weights);
    };

}
