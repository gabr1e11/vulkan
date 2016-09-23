/**
 * @class   VDeleter
 * @brief   Utility class to automatically delete Vulkan handles when
 *          they go out of scope
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#pragma once

#include "VulkanApi.hpp"
#include <functional>

template <typename T>
class VDeleter {
public:
    /**
     * Constructor for generic _deleter with empty deletion function
     */
    VDeleter() : VDeleter([](T, VkAllocationCallbacks*) {}) {}

    /**
     * Constructor for deletion functions with 2 parameters
     */
    VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
        this->_deleter = [=](T obj) { deletef(obj, nullptr); };
    }

    /**
     * Specialized constructor for Vulkan instances
     */
    VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
        this->_deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    /**
     * Specialized constructor for Vulkan devices
     */
    VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        this->_deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    /**
     * Destructor
     */
    ~VDeleter() {
        _cleanup();
    }

    T* operator &() {
        _cleanup();
        return &_object;
    }

    T* replace() {
        _cleanup();
        return &_object;
    }

    /**
     * Cast operator
     */
    operator T() const {
        return _object;
    }

    /**
     * Assignment operator
     */
    void operator=(T rhs) {
        _cleanup();
        _object = rhs;
    }

    /**
     * Comparison operator
     */
    template<typename V>
        bool operator==(V rhs) {
            return _object == T(rhs);
        }

private:
    T _object{VK_NULL_HANDLE};       /**< Internal object handle managed by the class */
    std::function<void(T)> _deleter; /**< Lambda function to delete the handle */

    /**
     * Cleanup function to delete the internal object
     */
    void _cleanup() {
        if (_object != VK_NULL_HANDLE) {
            _deleter(_object);
        }
        _object = VK_NULL_HANDLE;
    }
};
