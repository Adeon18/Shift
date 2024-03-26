#ifndef SHIFT_TEXTURESYSTEM_HPP
#define SHIFT_TEXTURESYSTEM_HPP

#include <unordered_map>
#include <memory>

#include "Utility/GUIDGenerator/GUIDGenerator.hpp"

#include "Graphics/Abstraction/Commands/CommandPool.hpp"
#include "Graphics/Abstraction/Images/Images.hpp"
#include "Graphics/Abstraction/Descriptors/DescriptorManager.hpp"
#include "Graphics/Abstraction/Images/SamplerManager.hpp"

#include "Graphics/UI/UIWindowComponent.hpp"
#include "Graphics/UI/UIManager.hpp"

namespace shift::gfx {
    class TextureSystem {
        class UI: public ui::UIToolComponent {
        public:
            explicit UI(std::string name, TextureSystem& system): ui::UIToolComponent(std::move(name)), m_system{system} {
                ui::UIManager::GetInstance().RegisterToolComponent(this);
            }

            virtual void Item() override { ui::UIToolComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;

            std::unordered_map<SGUID, SGUID> textureIdToDescriptorIdLUT;
        private:
            TextureSystem& m_system;
        };
    public:
        TextureSystem(
                const Device& device,
                const SamplerManager& samplerManager,
                CommandPool& graphicsPool,
                CommandPool& transferPool,
                DescriptorManager& descriptorManager
                );

        [[nodiscard]] SGUID LoadTexture(const std::string& path, VkFormat format, std::string name = "");
        [[nodiscard]] TextureBase* GetTexture(SGUID guid);
        [[nodiscard]] TextureBase* GetTexture(std::string name);

        [[nodiscard]] TextureBase* GetDefaultWhiteTexture() { return GetTexture("WHITE"); }
        [[nodiscard]] TextureBase* GetDefaultBlackTexture() { return GetTexture("BLACK"); }
        [[nodiscard]] TextureBase* GetDefaultGrayTexture() { return GetTexture("GRAY"); }
        [[nodiscard]] TextureBase* GetDefaultRedTexture() { return GetTexture("RED"); }
        [[nodiscard]] TextureBase* GetDefaultGreenTexture() { return GetTexture("GREEN"); }
        [[nodiscard]] TextureBase* GetDefaultBlueTexture() { return GetTexture("BLUE"); }
    private:
        void CreateDefaultTextures();

        //! Utility function for default color textures
        SGUID CreateDefaultColorTexture(const std::array<uint8_t, 4>& color, std::string name);

        //! UI component
        UI m_UI{"Texture System", *this};

        const Device& m_device;
        const SamplerManager& m_samplerManager;
        CommandPool& m_gfxPool;
        CommandPool& m_transPool;
        DescriptorManager& m_descriptorManager;

        std::unordered_map<std::string, SGUID> m_textureIdByName;
        std::unordered_map<SGUID, std::unique_ptr<TextureBase>> m_textures;
    };
}

#endif //SHIFT_TEXTURESYSTEM_HPP
