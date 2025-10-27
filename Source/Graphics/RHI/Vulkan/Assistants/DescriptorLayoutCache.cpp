//
// Created by otrush on 10/27/2025.
//

#include "DescriptorLayoutCache.hpp"

namespace Shift::VK {
    void DescriptorLayoutCache::Init(const Device* device) {
        m_device = device;
    }
    void DescriptorLayoutCache::Cleanup(){
        //delete every descriptor layout held
        for (auto pair : layoutCache){
            vkDestroyDescriptorSetLayout(m_device->Get(), pair.second, nullptr);
        }
    }

    VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info){
        DescriptorLayoutInfo layoutinfo{};
        layoutinfo.bindings.reserve(info->bindingCount);
        bool isSorted = true;
        int lastBinding = -1;

        //copy from the direct info struct into our own one
        for (int i = 0; i < info->bindingCount; i++) {
            layoutinfo.bindings.push_back(info->pBindings[i]);

            // Check that the bindings are in strict increasing order
            if (info->pBindings[i].binding > lastBinding) {
                lastBinding = info->pBindings[i].binding;
            }
            else{
                isSorted = false;
            }
        }
        // Sort the bindings if they aren't in order
        if (!isSorted) {
            std::sort(layoutinfo.bindings.begin(), layoutinfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b ){
                    return a.binding < b.binding;
            });
        }

        // Try to grab from cache
        auto it = layoutCache.find(layoutinfo);
        if (it != layoutCache.end()) {
            return it->second;
        }
        // Create a new one (not found)
        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(m_device->Get(), info, nullptr, &layout);

        // Cache dat shi
        layoutCache[layoutinfo] = layout;
        return layout;
    }

    bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const {
        if (other.bindings.size() != bindings.size()) {
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
        }
        return true;
    }

    size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const{
        using std::size_t;
        using std::hash;

        size_t result = hash<size_t>()(bindings.size());

        for (const VkDescriptorSetLayoutBinding& b : bindings)
        {
            // Pack the binding data into a single int64. Not fully correct but it's ok
            size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

            // Shuffle the packed binding data and xor it with the main hash
            result ^= hash<size_t>()(binding_hash);
        }

        return result;
    }
} // Shift::VK
