#include "Model.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

#include "H5Cpp.h"

constexpr char headerAttributeName[] = "header";
constexpr char layerWeightsBaseName[] = "layer";

struct ModelFileAttribute {
    size_t layersCount;
};

Core::EmnistModel::EmnistModel(const std::string& path) {
    this->loadFromFile(path);
}

void Core::EmnistModel::loadFromFile(const std::string& path) {
    try {
        H5::H5File file(path, H5F_ACC_RDONLY);

        H5::CompType attrType(sizeof(ModelFileAttribute));
        attrType.insertMember("layersCount", HOFFSET(ModelFileAttribute, layersCount), H5::PredType::NATIVE_UINT);
        hsize_t dims[2] = {1};
        H5::DataSpace attrDataspace(1, dims);

        ModelFileAttribute header;
        auto attr = file.openAttribute(headerAttributeName);
        attr.read(attrType, &header);

        this->layersCount = header.layersCount;
        this->layersWeights.resize(this->layersCount);
        this->neuronsCount.resize(this->layersCount + 1);
        std::vector<float> tmp;

        for (size_t i = 0; i < this->layersCount; i++) {
            H5::DataSet dataset = file.openDataSet(std::string(layerWeightsBaseName) + std::to_string(i));
            H5::DataSpace dataspace = dataset.getSpace();

            if (dataspace.getSimpleExtentNdims() != 2)
                throw std::runtime_error("Malformed model file");

            dataspace.getSimpleExtentDims(dims);

            this->neuronsCount[i] = dims[1];
            uint64_t size = dims[0] * dims[1];
            this->layersWeights[i].resize(size);
            tmp.resize(size);
            dataset.read(tmp.data(), H5::PredType::NATIVE_FLOAT);
            std::transform(tmp.cbegin(), tmp.cend(), this->layersWeights[i].begin(), [](float in) { return static_cast<uint16_t>(in * 4096.0f); });
        }
        this->neuronsCount[this->layersCount] = dims[0];
    } catch (H5::Exception& e) {
        throw std::runtime_error(e.getDetailMsg());
    }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------//

Core::FigureModel::FigureModel(const std::string& info, const std::string& weights) {
    this->loadFromFile(info, weights);
}

void Core::FigureModel::loadFromFile(const std::string& info, const std::string& weights) {
    try {
        std::string tmp;
        std::ifstream fin(info);
        fin.exceptions(std::ifstream::failbit);
        fin >> tmp;
        if (tmp != "NetWork")
            throw std::runtime_error("Malformed model info file");

        fin >> this->layersCount;
        this->layersCount--;
        this->neuronsCount.resize(this->layersCount + 1);
        this->layersWeights.resize(this->layersCount);
        for (int i = 0; i < this->layersCount + 1; i++) {
            fin >> this->neuronsCount[i];
        }
        fin.close();
        fin.open(weights);
        if (!fin.is_open()) {
            throw std::runtime_error("Error reading the weights file");
        }
        float tmpWeight;
        for (int i = 0; i < this->layersCount; i++) {
            size_t size = this->neuronsCount[i] * this->neuronsCount[i + 1];
            this->layersWeights[i].resize(size);
            for (size_t j = 0; j < size; j++) {
                fin >> tmpWeight;
                this->layersWeights[i][j] = static_cast<uint16_t>(tmpWeight * 4096.0f);
            }
        }
    } catch (const std::ios_base::failure& fail) {
        throw std::runtime_error(fail.what());
    }
}
