/**
 * @file    main.cpp
 * @brief   Entry point for vulkan engine demo
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#include "VulkanEngine.hpp"

#include <stdexcept>
#include <iostream>

int main() {
    VulkanEngine engine;

    try {
        engine.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
