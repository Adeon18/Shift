#ifndef SHIFT_TEXTUREMANAGER_HPP
#define SHIFT_TEXTUREMANAGER_HPP

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
    class TextureManager {
        class UI: public ui::UIWindowComponent {
        public:
            static constexpr uint32_t DEFAULT_UI_TEX_SIZE = 256;
            explicit UI(std::string name, std::string sName, TextureManager& system): ui::UIWindowComponent{std::move(name), std::move(sName)}, m_system{system} {}


            virtual void Item() override { ui::UIWindowComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;

            std::unordered_map<SGUID, SGUID> textureIdToDescriptorIdLUT;
            std::unordered_map<SGUID, float> textureUIScales;
        private:
            TextureManager& m_system;
        };
    public:
        TextureManager(
                const Device& device,
                const SamplerManager& samplerManager,
                CommandPool& graphicsPool,
                CommandPool& transferPool,
                DescriptorManager& descriptorManager
                );

        [[nodiscard]] SGUID LoadTexture(const std::string& path, VkFormat format, const std::string& name, bool generateMips);
        [[nodiscard]] TextureBase* GetTexture(SGUID guid);
        [[nodiscard]] TextureBase* GetTexture(std::string name);

        [[nodiscard]] TextureBase* GetDefaultWhiteTexture() { return GetTexture("WHITE"); }
        [[nodiscard]] TextureBase* GetDefaultBlackTexture() { return GetTexture("BLACK"); }
        [[nodiscard]] TextureBase* GetDefaultGrayTexture() { return GetTexture("GRAY"); }
        [[nodiscard]] TextureBase* GetDefaultRedTexture() { return GetTexture("RED"); }
        [[nodiscard]] TextureBase* GetDefaultGreenTexture() { return GetTexture("GREEN"); }
        [[nodiscard]] TextureBase* GetDefaultBlueTexture() { return GetTexture("BLUE"); }
        [[nodiscard]] TextureBase* GetDefaultOpaqueTexture() { return GetTexture("BLACK_OPAQUE"); }
    private:
        //! Create default 1x1 color textures
        void CreateDefaultTextures();

        //! Utility function for default color textures
        SGUID CreateDefaultColorTexture(const std::array<uint8_t, 4>& color, std::string name);

        //! Generates mips for texture by id and trasfer it to be read in shader
        void GenerateMipsAndTransferToReadState(SGUID textureId);

        //! UI component
        UI m_UI{"Texture Manager", "Tools", *this};

        const Device& m_device;
        const SamplerManager& m_samplerManager;
        CommandPool& m_gfxPool;
        CommandPool& m_transPool;
        DescriptorManager& m_descriptorManager;

        std::unordered_map<std::string, SGUID> m_textureIdByName;
        std::unordered_map<SGUID, std::unique_ptr<TextureBase>> m_textures;
    };
}

#endif //SHIFT_TEXTUREMANAGER_HPP
