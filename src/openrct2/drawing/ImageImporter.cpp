/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "ImageImporter.h"

#include "../core/Imaging.h"

#include <cstring>
#include <stdexcept>
#include <string>

using namespace OpenRCT2::Drawing;
using ImportResult = ImageImporter::ImportResult;

constexpr int32_t PALETTE_TRANSPARENT = -1;

ImportResult ImageImporter::Import(
    const Image& image, int32_t offsetX, int32_t offsetY, IMPORT_FLAGS flags, IMPORT_MODE mode) const
{
    if (image.Width > 256 || image.Height > 256)
    {
        throw std::invalid_argument("Only images 256x256 or less are supported.");
    }

    if ((flags & IMPORT_FLAGS::KEEP_PALETTE) && image.Depth != 8)
    {
        throw std::invalid_argument("Image is not palletted, it has bit depth of " + std::to_string(image.Depth));
    }

    const auto width = image.Width;
    const auto height = image.Height;

    auto pixels = GetPixels(image.Pixels.data(), width, height, flags, mode);
    auto [buffer, bufferLength] = flags & IMPORT_FLAGS::RLE ? EncodeRLE(pixels.data(), width, height)
                                                            : EncodeRaw(pixels.data(), width, height);

    rct_g1_element outElement;
    outElement.offset = static_cast<uint8_t*>(buffer);
    outElement.width = width;
    outElement.height = height;
    outElement.flags = (flags & IMPORT_FLAGS::RLE ? G1_FLAG_RLE_COMPRESSION : G1_FLAG_BMP);
    outElement.x_offset = offsetX;
    outElement.y_offset = offsetY;
    outElement.zoomed_offset = 0;

    ImportResult result;
    result.Element = outElement;
    result.Buffer = buffer;
    result.BufferLength = bufferLength;
    return result;
}

std::vector<int32_t> ImageImporter::GetPixels(
    const uint8_t* pixels, uint32_t width, uint32_t height, IMPORT_FLAGS flags, IMPORT_MODE mode)
{
    std::vector<int32_t> buffer;
    buffer.reserve(width * height);

    // A larger range is needed for proper dithering
    auto palettedSrc = pixels;
    std::unique_ptr<int16_t[]> rgbaSrcBuffer;
    if (!(flags & IMPORT_FLAGS::KEEP_PALETTE))
    {
        rgbaSrcBuffer = std::make_unique<int16_t[]>(height * width * 4);
    }

    auto rgbaSrc = rgbaSrcBuffer.get();
    if (!(flags & IMPORT_FLAGS::KEEP_PALETTE))
    {
        for (uint32_t x = 0; x < height * width * 4; x++)
        {
            rgbaSrc[x] = static_cast<int16_t>(pixels[x]);
        }
    }

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            int32_t paletteIndex;
            if (flags & IMPORT_FLAGS::KEEP_PALETTE)
            {
                paletteIndex = *palettedSrc;
                // The 1st index is always transparent
                if (paletteIndex == 0)
                {
                    paletteIndex = PALETTE_TRANSPARENT;
                }
            }
            else
            {
                paletteIndex = CalculatePaletteIndex(mode, rgbaSrc, x, y, width, height);
            }

            rgbaSrc += 4;
            palettedSrc += 1;

            buffer.push_back(paletteIndex);
        }
    }

    return buffer;
}

std::tuple<void*, size_t> ImageImporter::EncodeRaw(const int32_t* pixels, uint32_t width, uint32_t height)
{
    auto bufferLength = width * height;
    auto buffer = static_cast<uint8_t*>(std::malloc(bufferLength));
    for (size_t i = 0; i < bufferLength; i++)
    {
        auto p = pixels[i];
        buffer[i] = (p == PALETTE_TRANSPARENT ? 0 : static_cast<uint8_t>(p));
    }
    return std::make_tuple(buffer, bufferLength);
}

std::tuple<void*, size_t> ImageImporter::EncodeRLE(const int32_t* pixels, uint32_t width, uint32_t height)
{
    struct RLECode
    {
        uint8_t NumPixels{};
        uint8_t OffsetX{};
    };

    auto src = pixels;
    auto buffer = static_cast<uint8_t*>(std::malloc((height * 2) + (width * height * 16)));
    if (buffer == nullptr)
    {
        throw std::bad_alloc();
    }

    std::fill_n(buffer, (height * 2) + (width * height * 16), 0x00);
    auto yOffsets = reinterpret_cast<uint16_t*>(buffer);
    auto dst = buffer + (height * 2);
    for (uint32_t y = 0; y < height; y++)
    {
        yOffsets[y] = static_cast<uint16_t>(dst - buffer);

        auto previousCode = static_cast<RLECode*>(nullptr);
        auto currentCode = reinterpret_cast<RLECode*>(dst);
        dst += 2;

        auto startX = 0;
        auto npixels = 0;
        bool pushRun = false;
        for (uint32_t x = 0; x < width; x++)
        {
            int32_t paletteIndex = *src++;
            if (paletteIndex == PALETTE_TRANSPARENT)
            {
                if (npixels != 0)
                {
                    x--;
                    src--;
                    pushRun = true;
                }
            }
            else
            {
                if (npixels == 0)
                {
                    startX = x;
                }

                npixels++;
                *dst++ = static_cast<uint8_t>(paletteIndex);
            }
            if (npixels == 127 || x == width - 1)
            {
                pushRun = true;
            }

            if (pushRun)
            {
                if (npixels > 0)
                {
                    previousCode = currentCode;
                    currentCode->NumPixels = npixels;
                    currentCode->OffsetX = startX;

                    if (x == width - 1)
                    {
                        currentCode->NumPixels |= 0x80;
                    }

                    currentCode = reinterpret_cast<RLECode*>(dst);
                    dst += 2;
                }
                else
                {
                    if (previousCode == nullptr)
                    {
                        currentCode->NumPixels = 0x80;
                        currentCode->OffsetX = 0;
                    }
                    else
                    {
                        previousCode->NumPixels |= 0x80;
                        dst -= 2;
                    }
                }

                startX = 0;
                npixels = 0;
                pushRun = false;
            }
        }
    }

    auto bufferLength = static_cast<size_t>(dst - buffer);
    buffer = static_cast<uint8_t*>(realloc(buffer, bufferLength));
    if (buffer == nullptr)
    {
        throw std::bad_alloc();
    }
    return std::make_tuple(buffer, bufferLength);
}

int32_t ImageImporter::CalculatePaletteIndex(
    IMPORT_MODE mode, int16_t* rgbaSrc, int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto& palette = StandardPalette;
    auto paletteIndex = GetPaletteIndex(palette, rgbaSrc);
    if (mode == IMPORT_MODE::CLOSEST || mode == IMPORT_MODE::DITHERING)
    {
        if (paletteIndex == PALETTE_TRANSPARENT && !IsTransparentPixel(rgbaSrc))
        {
            paletteIndex = GetClosestPaletteIndex(palette, rgbaSrc);
        }
    }
    if (mode == IMPORT_MODE::DITHERING)
    {
        if (!IsTransparentPixel(rgbaSrc) && IsChangablePixel(GetPaletteIndex(palette, rgbaSrc)))
        {
            auto dr = rgbaSrc[0] - static_cast<int16_t>(palette[paletteIndex].Red);
            auto dg = rgbaSrc[1] - static_cast<int16_t>(palette[paletteIndex].Green);
            auto db = rgbaSrc[2] - static_cast<int16_t>(palette[paletteIndex].Blue);

            if (x + 1 < width)
            {
                if (!IsTransparentPixel(rgbaSrc + 4) && IsChangablePixel(GetPaletteIndex(palette, rgbaSrc + 4)))
                {
                    // Right
                    rgbaSrc[4] += dr * 7 / 16;
                    rgbaSrc[5] += dg * 7 / 16;
                    rgbaSrc[6] += db * 7 / 16;
                }
            }

            if (y + 1 < height)
            {
                if (x > 0)
                {
                    if (!IsTransparentPixel(rgbaSrc + 4 * (width - 1))
                        && IsChangablePixel(GetPaletteIndex(palette, rgbaSrc + 4 * (width - 1))))
                    {
                        // Bottom left
                        rgbaSrc[4 * (width - 1)] += dr * 3 / 16;
                        rgbaSrc[4 * (width - 1) + 1] += dg * 3 / 16;
                        rgbaSrc[4 * (width - 1) + 2] += db * 3 / 16;
                    }
                }

                // Bottom
                if (!IsTransparentPixel(rgbaSrc + 4 * width) && IsChangablePixel(GetPaletteIndex(palette, rgbaSrc + 4 * width)))
                {
                    rgbaSrc[4 * width] += dr * 5 / 16;
                    rgbaSrc[4 * width + 1] += dg * 5 / 16;
                    rgbaSrc[4 * width + 2] += db * 5 / 16;
                }

                if (x + 1 < width)
                {
                    if (!IsTransparentPixel(rgbaSrc + 4 * (width + 1))
                        && IsChangablePixel(GetPaletteIndex(palette, rgbaSrc + 4 * (width + 1))))
                    {
                        // Bottom right
                        rgbaSrc[4 * (width + 1)] += dr * 1 / 16;
                        rgbaSrc[4 * (width + 1) + 1] += dg * 1 / 16;
                        rgbaSrc[4 * (width + 1) + 2] += db * 1 / 16;
                    }
                }
            }
        }
    }
    return paletteIndex;
}

int32_t ImageImporter::GetPaletteIndex(const GamePalette& palette, int16_t* colour)
{
    if (!IsTransparentPixel(colour))
    {
        for (int32_t i = 0; i < PALETTE_SIZE; i++)
        {
            if (static_cast<int16_t>(palette[i].Red) == colour[0] && static_cast<int16_t>(palette[i].Green) == colour[1]
                && static_cast<int16_t>(palette[i].Blue) == colour[2])
            {
                return i;
            }
        }
    }
    return PALETTE_TRANSPARENT;
}

bool ImageImporter::IsTransparentPixel(const int16_t* colour)
{
    return colour[3] < 128;
}

/**
 * @returns true if pixel index is an index not used for remapping.
 */
bool ImageImporter::IsChangablePixel(int32_t paletteIndex)
{
    if (paletteIndex == PALETTE_TRANSPARENT)
        return true;
    if (paletteIndex == 0)
        return false;
    if (paletteIndex >= 203 && paletteIndex < 214)
        return false;
    if (paletteIndex == 226)
        return false;
    if (paletteIndex >= 227 && paletteIndex < 229)
        return false;
    if (paletteIndex >= 243)
        return false;
    return true;
}

int32_t ImageImporter::GetClosestPaletteIndex(const GamePalette& palette, const int16_t* colour)
{
    auto smallestError = static_cast<uint32_t>(-1);
    auto bestMatch = PALETTE_TRANSPARENT;
    for (int32_t x = 0; x < PALETTE_SIZE; x++)
    {
        if (IsChangablePixel(x))
        {
            uint32_t error = (static_cast<int16_t>(palette[x].Red) - colour[0])
                    * (static_cast<int16_t>(palette[x].Red) - colour[0])
                + (static_cast<int16_t>(palette[x].Green) - colour[1]) * (static_cast<int16_t>(palette[x].Green) - colour[1])
                + (static_cast<int16_t>(palette[x].Blue) - colour[2]) * (static_cast<int16_t>(palette[x].Blue) - colour[2]);

            if (smallestError == static_cast<uint32_t>(-1) || smallestError > error)
            {
                bestMatch = x;
                smallestError = error;
            }
        }
    }
    return bestMatch;
}
