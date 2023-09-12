#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <iostream>
#include <vector>

#include <spdlog/fmt/fmt.h>
#include "CLI/CLI.hpp"
#include "spdlog/spdlog.h"

#include "config.h"

#include "Core/DataSet.h"
#include "Core/Model.h"

#include "Modules/BusMatrix/BusMatrix.h"
#include "Modules/Masters/Core.h"
#include "Modules/Masters/IOController.h"
#include "Modules/Slaves/InterruptController.h"
#include "Modules/Slaves/SharedMemory.h"

std::vector<uint8_t> generateConfig(Core::IModel* model, size_t coresCount);

int sc_main(int argc, char* argv[]) {
    spdlog::set_pattern("[%T.%e] %v");

    std::string currentTimeStr = std::format("{0:%Y%m%dT%H%M%S}", std::chrono::system_clock::now());

    size_t coresCount = 7;
    size_t maxTests = -1;
    std::string vcdFilePath;
    std::string resultsFilePath = std::format("test_{}.csv", currentTimeStr);

    std::string figures_modelInfoFilePath;
    std::string figures_modelWeightsFilePath;
    std::string figures_datasetFilePath;

    std::string emnist_imagesFilePath;
    std::string emnist_labelsFilePath;
    std::string emnist_modelInputFilePath;

    CLI::App app("Application used for training neural network.");
    app.allow_extras(false);

    app.add_option("-c,--cores", coresCount, "Number of processing cores")->check(CLI::Range(1, 1000))->capture_default_str();
    app.add_option("--max-tests", maxTests, "Maximum number of tests")->capture_default_str();
    auto vcdFilePathOption = app.add_option("--vcd", vcdFilePath, "Path to save vcd file")->type_name("PATH")->capture_default_str();

    auto resultFilePathOption = app.add_option("-o, --results", resultsFilePath, "Path to save results")->type_name("PATH")->capture_default_str();

    auto figuresCommand = app.add_subcommand("figures", "Figures network")->fallthrough();
    auto emnistCommand = app.add_subcommand("emnist", "Emnist network")->fallthrough();

    figuresCommand->add_option("-i,--info", figures_modelInfoFilePath, "File with model information")->required()->type_name("PATH");
    figuresCommand->add_option("-w,--weights", figures_modelWeightsFilePath, "File with model weights")->required()->type_name("PATH");
    figuresCommand->add_option("-d,--dataset", figures_datasetFilePath, "File containing dataset")->required()->type_name("PATH");

    emnistCommand->add_option("-i,--images", emnist_imagesFilePath, "File containing images")->required()->type_name("PATH");
    emnistCommand->add_option("-l,--labels", emnist_labelsFilePath, "File containing labels for images")->required()->type_name("PATH");
    emnistCommand->add_option("-m,--model", emnist_modelInputFilePath, "File with model to load")->required()->type_name("PATH");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    std::ofstream fout;
    if (resultFilePathOption->count()) {
        fout.open(resultsFilePath);
    }

    Core::IModel* modelPtr;
    Core::Dataset dataset;

    spdlog::info("Loading model");
    if (app.got_subcommand(figuresCommand)) {
        modelPtr = new Core::FigureModel(figures_modelInfoFilePath, figures_modelWeightsFilePath);
    } else {
        modelPtr = new Core::EmnistModel(emnist_modelInputFilePath);
    }
    spdlog::info("Model loaded. Layers: {0}", modelPtr->layersCount);

    spdlog::info("Loading dataset");
    if (app.got_subcommand(figuresCommand)) {
        dataset = Core::loadDataset_figure(figures_datasetFilePath, modelPtr->neuronsCount[0]);
    } else {
        dataset = Core::loadDataset_mnist(emnist_imagesFilePath, emnist_labelsFilePath);
    }
    spdlog::info("{0} images loaded", dataset.size());
    if (dataset.size() > maxTests) {
        dataset.resize(maxTests);
        spdlog::info("Dataset limited to {0}. See --max-tests", maxTests);
    }

    sc_core::sc_clock clk("clk", 10, SC_NS);
    Sim::BusMatrix bus("bus");
    bus.clk_i(clk);

    // Creating Slaves

    Sim::SharedMemory memory("memory", config::addresses::shared_memory_start, config::addresses::shared_memory_start + config::sizes::shared_memory);
    bus.slave_port(memory);

    Sim::InterruptController intController("intController", config::addresses::interrupt_controller_start);
    intController.clk_i(clk);
    bus.slave_port(intController);

    // Creating Masters

    // Cores
    sc_core::sc_vector<Sim::Core> cores("Core_", coresCount, [&](const char* nm, size_t i) {
        return new Sim::Core(nm, i, config::sizes::local_memory, coresCount);
    });
    for (size_t i = 0; i < coresCount; i++) {
        cores[i].clk_i(clk);
        cores[i].bus_port(bus);
        cores[i].IR_i(intController.IR_o);
    }

    size_t num = 0;
    size_t correct = 0;
    float error = 0;
    Sim::IOController ioController(
        "ioController",
        coresCount,
        std::bind(generateConfig, modelPtr, coresCount),
        [&]() -> const std::vector<uint16_t>& {
            if (num == dataset.size()) {
                sc_stop();
                return std::vector<uint16_t>{};
            }
            return dataset[num].input;
        },
        [&](const std::vector<uint16_t>& result) -> void {
            for (size_t i = 0; i < modelPtr->neuronsCount.back(); i++) {
                error += std::abs((dataset[num].expectedOutput == (i - 1) ? 1 : 0) - (result[i] / 4096.0f));
            }
            if (dataset[num].expectedOutput == std::distance(result.begin(), std::max_element(result.begin(), result.begin() + modelPtr->neuronsCount.back())))
                correct++;
            spdlog::info("{0}: {2} {1}", num, error, ((dataset[num].expectedOutput == std::distance(result.begin(), std::max_element(result.begin(), result.begin() + modelPtr->neuronsCount.back()))) ? "Correct" : "Error"));
            if (resultFilePathOption->count()) {
                fout << num << ": " << error << " " << ((dataset[num].expectedOutput == std::distance(result.begin(), std::max_element(result.begin(), result.begin() + modelPtr->neuronsCount.back()))) ? "Correct" : "Error") << ":Expected:";
                for (int i = 0; i < result.size(); i++) {
                    if (i != 0) {
                        fout << ", ";
                    }
                    fout << result[i];
                }
                fout << ", Got: " << dataset[num].expectedOutput << "\n";
            }
            num++;
        });
    ioController.clk_i(clk);
    ioController.bus_port(bus);
    ioController.IR_i(intController.IR_o);

    sc_trace_file* wf;

    if (vcdFilePathOption->count()) {
        wf = sc_create_vcd_trace_file(vcdFilePath.c_str());
        sc_trace(wf, clk, "clk");
        sc_trace(wf, intController.IR, "ir");
    }

    sc_start();

    if (vcdFilePathOption->count()) {
        sc_close_vcd_trace_file(wf);
        spdlog::info("vcd file saved as {0}", vcdFilePath);
    }

    error /= dataset.size() * modelPtr->neuronsCount[modelPtr->layersCount];
    spdlog::info("Average error: {0}", error);
    spdlog::info("Correct: {0}%", ((float)correct) / dataset.size() * 100.0f);
    std::cout << "Total time (ns): " << sc_time_stamp();

    delete modelPtr;

    return 0;
}

std::vector<uint8_t> generateConfig(Core::IModel* model, size_t coresCount) {
    // Calculating reqired size
    size_t area0SizeBytes = 2 * 4;
    size_t area1SizeBytes = 3 * 4;
    size_t area2SizeBytes = (1 + (model->layersCount + 1)) * 4;
    size_t area3SizeBytes = (coresCount * 4 * model->layersCount) * 4;
    size_t area4SizeBytes = 0;
    size_t area5SizeBytes = 0;
    for (size_t i = 0; i < model->layersCount; i++) {
        area4SizeBytes += model->neuronsCount[i] * model->neuronsCount[i + 1];
        area5SizeBytes += model->neuronsCount[i];
    }
    area5SizeBytes += model->neuronsCount[model->layersCount];
    area4SizeBytes *= 2;
    area5SizeBytes *= 2;
    size_t configSize = 4 + area1SizeBytes + area2SizeBytes + area3SizeBytes + area4SizeBytes;  // first element is output pointer

    std::vector<uint8_t> config(configSize, 0);
    // Creating config
    uint32_t* config32 = reinterpret_cast<uint32_t*>(config.data());  // yes, it can get misaligned. I don't care at this point
    config32[0] = config::addresses::shared_memory_start + area0SizeBytes + area1SizeBytes + area2SizeBytes + area3SizeBytes + area4SizeBytes + area5SizeBytes - model->neuronsCount[model->layersCount] * 2;
    // area 1
    config32[1] = config::addresses::shared_memory_start + area0SizeBytes + area1SizeBytes + area2SizeBytes;  // area 3 pointer
    config32[2] = config32[1] + area3SizeBytes;                                                               // area 4 pointer
    config32[3] = config32[2] + area4SizeBytes;                                                               // area 5 pointer
    // area 2
    config32[4] = model->layersCount + 1;
    uint32_t* config32it = config32 + 5;
    for (size_t i = 0; i < model->layersCount + 1; i++) {
        *config32it = model->neuronsCount[i];
        config32it += 1;
    }
    // area 3
    uint32_t weightsAddress = config32[2];
    uint32_t resultAddress = config32[3] + (model->neuronsCount[0] * 2);
    uint32_t inputAddress = config32[3];
    for (size_t i = 1; i < model->layersCount + 1; i++) {
        for (size_t j = 0; j < coresCount; j++) {
            config32it[0] = weightsAddress;
            config32it[1] = resultAddress;
            config32it[2] = (model->neuronsCount[i] / coresCount) + ((model->neuronsCount[i] % coresCount) > j ? 1 : 0);
            config32it[3] = inputAddress;
            weightsAddress += model->neuronsCount[i - 1] * config32it[2] * 2;
            resultAddress += config32it[2] * 2;
            config32it += 4;
        }
        inputAddress += model->neuronsCount[i - 1] * 2;
    }
    // area 4
    uint16_t* config16 = reinterpret_cast<uint16_t*>(config32it);
    for (size_t i = 0; i < model->layersCount; i++) {
        std::memcpy(config16, model->layersWeights[i].data(), model->layersWeights[i].size() * 2);
        config16 += model->layersWeights[i].size();
    }

    return config;
}