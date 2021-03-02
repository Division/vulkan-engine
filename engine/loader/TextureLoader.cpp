#include "TextureLoader.h"
#include "system/Logging.h"
#include <filesystem>
#include <lib/ktx/ktx.h>
#include <lib/ktx/ktxvulkan.h>
#include <lib/ktx/ktxint.h>
#include "FileLoader.h"

using namespace Device;

namespace
{
    Device::Format GetNoSRGBFormat(Device::Format src_format)
    {
        switch (src_format)
        {
            default: return src_format;
            case Device::Format::R8_srgb: return Device::Format::R8G8B8A8_unorm;
            case Device::Format::R8G8_srgb: return Device::Format::R8G8_unorm;
            case Device::Format::R8G8B8_srgb: return Device::Format::R8G8B8_unorm;
            case Device::Format::R8G8B8A8_srgb: return Device::Format::R8G8B8A8_unorm;
            case Device::Format::BC1_RGB_srgb: return Device::Format::BC1_RGB_unorm;
            case Device::Format::BC1_RGBA_srgb: return Device::Format::BC1_RGBA_unorm;
            case Device::Format::BC2_srgb: return Device::Format::BC2_unorm;
            case Device::Format::BC3_srgb: return Device::Format::BC3_unorm;
            case Device::Format::BC7_srgb: return Device::Format::BC7_unorm;
            case Device::Format::Undefined: throw std::runtime_error("Unknown format");
        }
    }
}


std::unique_ptr<Texture> loader::LoadTexture(const std::wstring &name, bool sRGB) {

    auto path = std::filesystem::path(name);
    auto extension = path.extension();
    auto file_data = loader::LoadFile(name);
    if (file_data.size() == 0)
    {
        ENGLog("Texture doesn't exist: %s", name.c_str());
        return nullptr;
    }

    // TODO: check magic, not extension
    ENGLog("Loading texture %s", name.c_str());
    if (extension == ".ktx")
    {
        ktxTexture* texture;
        KTX_error_code result = ktxTexture_CreateFromMemory(reinterpret_cast<ktx_uint8_t*>(file_data.data()), file_data.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
        
        if (result == KTX_error_code::KTX_SUCCESS)
        {
            VkFormat vk_format = ktxTexture_GetVkFormat(texture);
            ktx_uint8_t* image;
            uint32_t num_iterations;

            TextureInitializer initializer(texture->baseWidth, texture->baseHeight, texture->baseDepth, texture->numLayers * texture->numFaces, GetNoSRGBFormat((Format)vk_format), texture->numLevels, sRGB);
            initializer.SetData(texture->pData, texture->dataSize)
                       .SetArray(texture->isArray)
                       .SetCube(texture->isCubemap)
                       .SetNumDimensions(texture->numDimensions);

            auto element_size = ktxTexture_GetElementSize(texture);

            for (int miplevel = 0; miplevel < texture->numLevels; miplevel++)
            {
                GLsizei width, height, depth;
                ktx_uint32_t levelSize;
                ktx_size_t offset;

                /* Array textures have the same number of layers at each mip level. */
                width = std::max(1u, texture->baseWidth >> miplevel);
                height = std::max(1u, texture->baseHeight >> miplevel);
                depth = std::max(1u, texture->baseDepth >> miplevel);

                levelSize = (ktx_uint32_t)ktxTexture_levelSize(texture, miplevel);

                /* All array layers are passed in a group because that is how
                * GL & Vulkan need them. Hence no
                *    for (layer = 0; layer < This->numLayers)
                */

                auto faceLodSize = (ktx_uint32_t)ktxTexture_faceLodSize(texture, miplevel);

                /* All array layers are passed in a group because that is how
                * GL & Vulkan need them. Hence no
                *    for (layer = 0; layer < This->numLayers)
                */
                if (texture->isCubemap && !texture->isArray)
                    num_iterations = texture->numFaces;
                else
                    num_iterations = 1;


                for (int face = 0; face < num_iterations; ++face)
                {
                    /* And all z_slices are also passed as a group hence no
                    *    for (slice = 0; slice < This->depth)
                    */
                    ktx_size_t offset;
                    ktxTexture_GetImageOffset(texture, miplevel, 0, face, &offset);

                    initializer.AddCopy(
                        vk::BufferImageCopy(offset, 0, 0,
                            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, miplevel, face, texture->numLayers),
                            vk::Offset3D(0, 0, 0),
                            vk::Extent3D(width, height, depth)
                        ));
                }

            }

            auto result_texture = std::make_unique<Texture>(initializer);
            
            ktxTexture_Destroy(texture);

            return result_texture;
        }
        else
        {
            ktxTexture_Destroy(texture);

            ENGLog("Loading texture failed %s", name.c_str());
            throw std::runtime_error("Error loading texture");
        }

    }
    else
    {
        int32_t w, h, channels;
        auto data = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(file_data.data()), file_data.size(), &w, &h, &channels, 4);

        TextureInitializer initializer(w, h, 4, data, sRGB);
        auto texture = std::make_unique<Texture>(initializer);
        stbi_image_free(data);

        return texture;
    }

    return nullptr;
};
