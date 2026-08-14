//
// Created by otrush on 10/27/2025.
//

#include "DescriptorLayoutCache.hpp"

#include <algorithm>
#include <utility>

namespace Shift::VK {
    namespace {
        //! Order-sensitive, non-cancelling combine.
        size_t HashCombine(size_t seed, size_t value) {
            //! I stole this
            constexpr size_t GOLDEN = static_cast<size_t>(0x9e3779b97f4a7c15ULL);
            return seed ^ (std::hash<size_t>()(value) + GOLDEN + (seed << 6) + (seed >> 2));
        }
    }

    void DescriptorLayoutCache::Init(const Device* device) {
        m_device = device;
    }
    void DescriptorLayoutCache::Destroy(){
        //delete every descriptor layout held
        for (auto pair : layoutCache){
            vkDestroyDescriptorSetLayout(m_device->Get(), pair.second, nullptr);
        }
    }

    VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo& info,
                                                                       std::span<const VkDescriptorBindingFlags> bindingFlags){
        DescriptorLayoutInfo layoutinfo{};
        layoutinfo.createFlags = info.flags;
        layoutinfo.bindings.reserve(info.bindingCount);
        layoutinfo.bindingFlags.reserve(info.bindingCount);

        bool isSorted = true;
        int lastBinding = -1;

        //copy from the direct info struct into our own one
        for (uint32_t i = 0; i < info.bindingCount; i++) {
            layoutinfo.bindings.push_back(info.pBindings[i]);
            layoutinfo.bindingFlags.push_back(i < bindingFlags.size() ? bindingFlags[i] : 0u);

            // Check that the bindings are in strict increasing order
            if (static_cast<int>(info.pBindings[i].binding) > lastBinding) {
                lastBinding = static_cast<int>(info.pBindings[i].binding);
            }
            else{
                isSorted = false;
            }
        }
        // Sort the bindings if they aren't in order
        if (!isSorted) {
            std::vector<std::pair<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlags>> zipped;
            zipped.reserve(layoutinfo.bindings.size());
            for (size_t i = 0; i < layoutinfo.bindings.size(); ++i) {
                zipped.emplace_back(layoutinfo.bindings[i], layoutinfo.bindingFlags[i]);
            }

            std::sort(zipped.begin(), zipped.end(), [](const auto& a, const auto& b) {
                return a.first.binding < b.first.binding;
            });

            for (size_t i = 0; i < zipped.size(); ++i) {
                layoutinfo.bindings[i] = zipped[i].first;
                layoutinfo.bindingFlags[i] = zipped[i].second;
            }
        }

        // Try to grab from cache
        auto it = layoutCache.find(layoutinfo);
        if (it != layoutCache.end()) {
            return it->second;
        }
        // Create a new one (not found)
        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(m_device->Get(), &info, nullptr, &layout);

        // Cache dat shi
        layoutCache[layoutinfo] = layout;
        return layout;
    }

    bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const {
        //! Two layouts with the same bindings but different flags are different!!! Vk guide was wrong who would have thought
        if (other.createFlags != createFlags) {
            return false;
        }
        if (other.bindings.size() != bindings.size()) {
            return false;
        }
        if (other.bindingFlags.size() != bindingFlags.size()) {
            return false;
        }
        // Compare each of the bindings is the same. Bindings are sorted so they will match
        for (int i = 0; i < bindings.size(); i++) {
            if (other.bindings[i].binding != bindings[i].binding){
                return false;
            }
            if (other.bindings[i].descriptorType != bindings[i].descriptorType){
                return false;
            }
            if (other.bindings[i].descriptorCount != bindings[i].descriptorCount){
                return false;
            }
            if (other.bindings[i].stageFlags != bindings[i].stageFlags){
                return false;
            }
            if (other.bindingFlags[i] != bindingFlags[i]){
                return false;
            }
        }
        return true;
    }

    size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const{
        using std::size_t;
        using std::hash;

        size_t result = hash<size_t>()(bindings.size());
        result = HashCombine(result, static_cast<size_t>(createFlags));

        for (size_t i = 0; i < bindings.size(); ++i)
        {
            const VkDescriptorSetLayoutBinding& b = bindings[i];

            // Pack the binding data into a single int64. Not fully correct but it's ok
            size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

            result = HashCombine(result, binding_hash);
            result = HashCombine(result, static_cast<size_t>(bindingFlags[i]));
        }

        return result;
    }
} // Shift::VK
