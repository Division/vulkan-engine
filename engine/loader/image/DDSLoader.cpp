#include "DDSLoader.h"
#include "DDSLoaderHelpers.h"
#include "system/Logging.h"
#include "render/device/Types.h"
#include "loader/TextureLoader.h"

using namespace DirectX;
using namespace Device;

namespace loader
{

    struct SUBRESOURCE_DATA
    {
        size_t width;
        size_t height;
        size_t depth;
        const void* pSysMem;
        UINT SysMemPitch;
        UINT SysMemSlicePitch;
    };

    enum D3D11_RESOURCE_DIMENSION
    {
        D3D11_RESOURCE_DIMENSION_UNKNOWN = 0,
        D3D11_RESOURCE_DIMENSION_BUFFER = 1,
        D3D11_RESOURCE_DIMENSION_TEXTURE1D = 2,
        D3D11_RESOURCE_DIMENSION_TEXTURE2D = 3,
        D3D11_RESOURCE_DIMENSION_TEXTURE3D = 4
    } 	D3D11_RESOURCE_DIMENSION;

    enum D3D11_RESOURCE_MISC_FLAG
    {
        D3D11_RESOURCE_MISC_GENERATE_MIPS = 0x1L,
        D3D11_RESOURCE_MISC_SHARED = 0x2L,
        D3D11_RESOURCE_MISC_TEXTURECUBE = 0x4L,
        D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS = 0x10L,
        D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS = 0x20L,
        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED = 0x40L,
        D3D11_RESOURCE_MISC_RESOURCE_CLAMP = 0x80L,
        D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX = 0x100L,
        D3D11_RESOURCE_MISC_GDI_COMPATIBLE = 0x200L,
        D3D11_RESOURCE_MISC_SHARED_NTHANDLE = 0x800L,
        D3D11_RESOURCE_MISC_RESTRICTED_CONTENT = 0x1000L,
        D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE = 0x2000L,
        D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER = 0x4000L,
        D3D11_RESOURCE_MISC_GUARDED = 0x8000L,
        D3D11_RESOURCE_MISC_TILE_POOL = 0x20000L,
        D3D11_RESOURCE_MISC_TILED = 0x40000L,
        D3D11_RESOURCE_MISC_HW_PROTECTED = 0x80000L
    } 	D3D11_RESOURCE_MISC_FLAG;

    Format DXGIToEngineFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return Format::R32G32B32A32_float;
        case DXGI_FORMAT_R32G32B32A32_UINT: return Format::R32G32B32A32_uint;
        case DXGI_FORMAT_R32G32B32A32_SINT: return Format::R32G32B32A32_int;
        case DXGI_FORMAT_R32G32B32_FLOAT: return Format::R32G32B32_float;
        case DXGI_FORMAT_R32G32B32_UINT: return Format::R32G32B32_uint;
        case DXGI_FORMAT_R32G32B32_SINT: return Format::R32G32B32_int;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return Format::R16G16B16A16_float;
        case DXGI_FORMAT_R16G16B16A16_UNORM: return Format::R16G16B16A16_unorm;
        case DXGI_FORMAT_R16G16B16A16_UINT: return Format::R16G16B16A16_uint;
        case DXGI_FORMAT_R16G16B16A16_SNORM: return Format::R16G16B16A16_norm;
        case DXGI_FORMAT_R16G16B16A16_SINT: return Format::R16G16B16A16_int;
        case DXGI_FORMAT_R32G32_FLOAT: return Format::R32G32_float;
        case DXGI_FORMAT_R32G32_UINT: return Format::R32G32_uint;
        case DXGI_FORMAT_R32G32_SINT: return Format::R32G32_int;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return Format::D24_unorm_S8_uint;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return Format::R32_float;
        case DXGI_FORMAT_R10G10B10A2_UNORM: return Format::A2R10G10B10_unorm;
        case DXGI_FORMAT_R10G10B10A2_UINT: return Format::A2R10G10B10_uint;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return Format::R8G8B8A8_unorm;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return Format::R8G8B8A8_srgb;
        case DXGI_FORMAT_R8G8B8A8_UINT: return Format::R8G8B8A8_uint;
        case DXGI_FORMAT_R8G8B8A8_SNORM: return Format::R8G8B8_norm;
        case DXGI_FORMAT_R8G8B8A8_SINT: return Format::R8G8B8A8_int;
        case DXGI_FORMAT_R16G16_FLOAT: return Format::R16G16_float;
        case DXGI_FORMAT_R16G16_UNORM: return Format::R16G16_unorm;
        case DXGI_FORMAT_R16G16_UINT: return Format::R16G16_uint;
        case DXGI_FORMAT_R16G16_SNORM: return Format::R16G16_norm;
        case DXGI_FORMAT_R16G16_SINT: return Format::R16G16_int;
        case DXGI_FORMAT_D32_FLOAT: return Format::D32_float;
        case DXGI_FORMAT_R32_FLOAT: return Format::R32_float;
        case DXGI_FORMAT_R32_UINT: return Format::R32_uint;
        case DXGI_FORMAT_R32_SINT: return Format::R32_int;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return Format::D24_unorm_S8_uint;
        case DXGI_FORMAT_R8G8_UNORM: return Format::R8G8_unorm;
        case DXGI_FORMAT_R8G8_UINT: return Format::R8G8_uint;
        case DXGI_FORMAT_R8G8_SNORM: return Format::R8G8_norm;
        case DXGI_FORMAT_R8G8_SINT: return Format::R8G8_int;
        case DXGI_FORMAT_R16_FLOAT: return Format::R16_float;
        case DXGI_FORMAT_D16_UNORM: return Format::D16_unorm;
        case DXGI_FORMAT_R16_UNORM: return Format::R16_unorm;
        case DXGI_FORMAT_R16_UINT: return Format::R16_uint;
        case DXGI_FORMAT_R16_SNORM: return Format::R16_norm;
        case DXGI_FORMAT_R16_SINT: return Format::R16_int;
        case DXGI_FORMAT_R8_UNORM: return Format::R8_unorm;
        case DXGI_FORMAT_R8_UINT: return Format::R8_uint;
        case DXGI_FORMAT_R8_SNORM: return Format::R8_norm;
        case DXGI_FORMAT_R8_SINT: return Format::R8_int;
        case DXGI_FORMAT_A8_UNORM: return Format::R8_unorm;
        case DXGI_FORMAT_BC1_UNORM: return Format::BC1_RGBA_unorm;
        case DXGI_FORMAT_BC1_UNORM_SRGB: return Format::BC1_RGBA_srgb;
        case DXGI_FORMAT_BC2_UNORM: return Format::BC2_unorm;
        case DXGI_FORMAT_BC2_UNORM_SRGB: return Format::BC2_srgb;
        case DXGI_FORMAT_BC3_UNORM: return Format::BC3_unorm;
        case DXGI_FORMAT_BC3_UNORM_SRGB: return Format::BC3_srgb;
        case DXGI_FORMAT_BC4_UNORM: return Format::BC4_unorm;
        case DXGI_FORMAT_BC4_SNORM: return Format::BC4_snorm;
        case DXGI_FORMAT_BC5_UNORM: return Format::BC5_unorm;
        case DXGI_FORMAT_BC5_SNORM: return Format::BC5_snorm;
        case DXGI_FORMAT_BC6H_UF16: return Format::BC6H_ufloat;
        case DXGI_FORMAT_BC6H_SF16: return Format::BC6H_sfloat;
        case DXGI_FORMAT_BC7_UNORM: return Format::BC7_unorm;
        case DXGI_FORMAT_BC7_UNORM_SRGB: return Format::BC7_srgb;

        default:
            throw std::runtime_error("unsupported DXGI format");
        }
    }

    HRESULT FillInitData(
        size_t width,
        size_t height,
        size_t depth,
        size_t mipCount,
        size_t arraySize,
        DXGI_FORMAT format,
        size_t maxsize,
        size_t bitSize,
        const uint8_t* bitData,
        size_t& twidth,
        size_t& theight,
        size_t& tdepth,
        size_t& skipMip,
        SUBRESOURCE_DATA* initData) noexcept
    {
        if (!bitData || !initData)
        {
            return E_POINTER;
        }

        skipMip = 0;
        twidth = 0;
        theight = 0;
        tdepth = 0;

        size_t NumBytes = 0;
        size_t RowBytes = 0;
        const uint8_t* pSrcBits = bitData;
        const uint8_t* pEndBits = bitData + bitSize;

        size_t index = 0;
        for (size_t j = 0; j < arraySize; j++)
        {
            size_t w = width;
            size_t h = height;
            size_t d = depth;
            for (size_t i = 0; i < mipCount; i++)
            {
                HRESULT hr = GetSurfaceInfo(w, h, format, &NumBytes, &RowBytes, nullptr);
                if (FAILED(hr))
                    return hr;

                if (NumBytes > UINT32_MAX || RowBytes > UINT32_MAX)
                    return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

                if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
                {
                    if (!twidth)
                    {
                        twidth = w;
                        theight = h;
                        tdepth = d;
                    }

                    assert(index < mipCount * arraySize);
                    initData[index].pSysMem = pSrcBits;
                    initData[index].SysMemPitch = static_cast<UINT>(RowBytes);
                    initData[index].SysMemSlicePitch = static_cast<UINT>(NumBytes);
                    initData[index].width = w;
                    initData[index].height = h;
                    initData[index].depth = d;
                    ++index;
                }
                else if (!j)
                {
                    // Count number of skipped mipmaps (first item only)
                    ++skipMip;
                }

                if (pSrcBits + (NumBytes * d) > pEndBits)
                {
                    return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
                }

                pSrcBits += NumBytes * d;

                w = w >> 1;
                h = h >> 1;
                d = d >> 1;
                if (w == 0)
                {
                    w = 1;
                }
                if (h == 0)
                {
                    h = 1;
                }
                if (d == 0)
                {
                    d = 1;
                }
            }
        }

        return (index > 0) ? S_OK : E_FAIL;
    }

    std::unique_ptr<Device::Texture> CreateTextureFromDDS(
        _In_ const DDS_HEADER* header,
        _In_reads_bytes_(bitSize) const uint8_t* bitData,
        _In_ size_t bitSize,
        _In_ bool sRGB
    )
    {
        HRESULT hr = S_OK;

        UINT width = header->width;
        UINT height = header->height;
        UINT depth = header->depth;

        uint32_t resDim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        UINT arraySize = 1;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        bool isCubeMap = false;

        size_t mipCount = header->mipMapCount;
        if (0 == mipCount)
        {
            mipCount = 1;
        }

        if ((header->ddspf.flags & DDS_FOURCC) &&
            (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
        {
            auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>(reinterpret_cast<const char*>(header) + sizeof(DDS_HEADER));

            arraySize = d3d10ext->arraySize;
            if (arraySize == 0)
            {
                return nullptr;
            }

            switch (d3d10ext->dxgiFormat)
            {
            case DXGI_FORMAT_AI44:
            case DXGI_FORMAT_IA44:
            case DXGI_FORMAT_P8:
            case DXGI_FORMAT_A8P8:
                ENGLog("ERROR: DDSTextureLoader does not support video textures. Consider using DirectXTex instead");
                return nullptr;

            default:
                if (BitsPerPixel(d3d10ext->dxgiFormat) == 0)
                {
                    ENGLog("ERROR: Unknown DXGI format (%u)\n", static_cast<uint32_t>(d3d10ext->dxgiFormat));
                    return nullptr;
                }
            }

            format = d3d10ext->dxgiFormat;

            switch (d3d10ext->resourceDimension)
            {
            case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
                // D3DX writes 1D textures with a fixed Height of 1
                if ((header->flags & DDS_HEIGHT) && height != 1)
                {
                    return nullptr;
                }
                height = depth = 1;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
                if (d3d10ext->miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE)
                {
                    arraySize *= 6;
                    isCubeMap = true;
                }
                depth = 1;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
                if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
                {
                    return nullptr;
                }

                if (arraySize > 1)
                {
                    ENGLog("ERROR: Volume textures are not texture arrays\n");
                    return nullptr;
                }
                break;

            case D3D11_RESOURCE_DIMENSION_BUFFER:
                ENGLog("ERROR: Resource dimension buffer type not supported for textures\n");
                return nullptr;

            case D3D11_RESOURCE_DIMENSION_UNKNOWN:
            default:
                ENGLog("ERROR: Unknown resource dimension (%u)\n", static_cast<uint32_t>(d3d10ext->resourceDimension));
                return nullptr;
            }

            resDim = d3d10ext->resourceDimension;
        }
        else
        {
            format = GetDXGIFormat(header->ddspf);

            if (format == DXGI_FORMAT_UNKNOWN)
            {
                ENGLog("ERROR: DDSTextureLoader does not support all legacy DDS formats. Consider using DirectXTex.\n");
                return nullptr;
            }

            if (header->flags & DDS_HEADER_FLAGS_VOLUME)
            {
                resDim = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
            }
            else
            {
                if (header->caps2 & DDS_CUBEMAP)
                {
                    // We require all six faces to be defined
                    if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
                    {
                        ENGLog("ERROR: DirectX 11 does not support partial cubemaps\n");
                        return nullptr;
                    }

                    arraySize = 6;
                    isCubeMap = true;
                }

                depth = 1;
                resDim = D3D11_RESOURCE_DIMENSION_TEXTURE2D;

                // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
            }

            assert(BitsPerPixel(format) != 0);
        }

        // Bound sizes (for security purposes we don't trust DDS file metadata larger than the Direct3D hardware requirements)
        if (mipCount > 16)
        {
            ENGLog("ERROR: Too many mipmap levels defined for DirectX 11 (%zu).\n", mipCount);
            return nullptr;
        }

        int num_dimensions = 0;

        switch (resDim)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            num_dimensions = 1;

            if ((arraySize > 2048) ||
                (width > 16384))
            {
                ENGLog("ERROR: Resource dimensions too large for DirectX 11 (1D: array %u, size %u)\n", arraySize, width);
                return nullptr;
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            num_dimensions = 2;

            if (isCubeMap)
            {
                // This is the right bound because we set arraySize to (NumCubes*6) above
                if ((arraySize > 2048) ||
                    (width > 16384) ||
                    (height > 16384))
                {
                    ENGLog("ERROR: Resource dimensions too large for DirectX 11 (2D cubemap: array %u, size %u by %u)\n", arraySize, width, height);
                    return nullptr;
                }
            }
            else if ((arraySize > 2048) ||
                (width > 16384) ||
                (height > 16384))
            {
                ENGLog("ERROR: Resource dimensions too large for DirectX 11 (2D: array %u, size %u by %u)\n", arraySize, width, height);
                return nullptr;
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            num_dimensions = 3;

            if ((arraySize > 1) ||
                (width > 2048) ||
                (height > 2048) ||
                (depth > 2048))
            {
                ENGLog("ERROR: Resource dimensions too large for DirectX 11 (3D: array %u, size %u by %u by %u)\n", arraySize, width, height, depth);
                return nullptr;
            }
            break;

        case D3D11_RESOURCE_DIMENSION_BUFFER:
            ENGLog("ERROR: Resource dimension buffer type not supported for textures\n");
            return nullptr;

        case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        default:
            ENGLog("ERROR: Unknown resource dimension (%u)\n", static_cast<uint32_t>(resDim));
            return nullptr;
        }

        if (!num_dimensions)
        {
            ENGLog("ERROR: Texture dimensions unknown");
            return nullptr;
        }

        {
            // Create the texture
            std::unique_ptr<SUBRESOURCE_DATA[]> initData(new (std::nothrow) SUBRESOURCE_DATA[mipCount * arraySize]);
            if (!initData)
            {
                return nullptr;
            }

            size_t skipMip = 0;
            size_t twidth = 0;
            size_t theight = 0;
            size_t tdepth = 0;
            hr = FillInitData(width, height, depth, mipCount, arraySize, format, 0, bitSize, bitData, twidth, theight, tdepth, skipMip, initData.get());

            if (SUCCEEDED(hr))
            {
                const auto actual_mip_count = mipCount - skipMip;

                TextureInitializer initializer(width, height, depth, arraySize, GetNoSRGBFormat(DXGIToEngineFormat(format)), actual_mip_count, sRGB);
                initializer.SetData(bitData, bitSize)
                    .SetArray(!isCubeMap && arraySize > 1)
                    .SetCube(isCubeMap)
                    .SetNumDimensions(num_dimensions);

                size_t index = 0;
                size_t offset = 0;
                for (size_t face = 0; face < arraySize; face++)
                {
                    for (size_t miplevel = 0; miplevel < actual_mip_count; miplevel++)
                    {
                        auto& data = initData[index++];
                        initializer.AddCopy(
                            vk::BufferImageCopy(offset, 0, 0,
                                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, miplevel, face, arraySize),
                                vk::Offset3D(0, 0, 0),
                                vk::Extent3D(data.width, data.height, data.depth)
                            ));
                        offset += data.SysMemSlicePitch;
                    }
                }

                return std::make_unique<Texture>(initializer);
            }
        }

        return nullptr;
    }

	std::unique_ptr<Device::Texture> LoadDDSFromMemory(const std::vector<uint8_t>& data, bool sRGB)
	{
        // Validate DDS file in memory
        const DDS_HEADER* header = nullptr;
        const uint8_t* bitData = nullptr;
        size_t bitSize = 0;

        HRESULT hr = LoadTextureDataFromMemory(data.data(), data.size(),
            &header,
            &bitData,
            &bitSize
        );

        return CreateTextureFromDDS(header, bitData, bitSize, sRGB);
	}

}