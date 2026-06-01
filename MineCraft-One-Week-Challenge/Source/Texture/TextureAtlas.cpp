#include "TextureAtlas.h"
#include <array>

TextureAtlas::TextureAtlas(const std::string &textureFileName)
{
    sf::Image i;
    if (!i.loadFromFile("Res/Textures/" + textureFileName + ".png")) {
        throw std::runtime_error("Unable to open image: " + textureFileName);
    }
    addHeartTextures(i);

    loadFromImage(i);

    m_imageSize = 256;
    m_individualTextureSize = 16;
}

std::array<GLfloat, 8> TextureAtlas::getTexture(const sf::Vector2i &coords)
{
    static const GLfloat TEX_PER_ROW =
        (GLfloat)m_imageSize / (GLfloat)m_individualTextureSize;
    static const GLfloat INDV_TEX_SIZE = 1.0f / TEX_PER_ROW;
    static const GLfloat PIXEL_SIZE = 1.0f / (float)m_imageSize;

    GLfloat xMin = (coords.x * INDV_TEX_SIZE) + 0.5f * PIXEL_SIZE;
    GLfloat yMin = (coords.y * INDV_TEX_SIZE) + 0.5f * PIXEL_SIZE;

    GLfloat xMax = (xMin + INDV_TEX_SIZE) - PIXEL_SIZE;
    GLfloat yMax = (yMin + INDV_TEX_SIZE) - PIXEL_SIZE;

    return {xMax, yMax, xMin, yMax, xMin, yMin, xMax, yMin};
}

void TextureAtlas::addHeartTextures(sf::Image &image)
{
    using sf::Vector2u;

    // Helper: set a horizontal run of pixels within a tile
    auto setLine = [&](int baseX, int baseY, int pxY, int pxXStart, int pxXEnd,
                       sf::Color color) {
        for (int x = pxXStart; x <= pxXEnd; ++x) {
            image.setPixel(Vector2u(baseX + x, baseY + pxY), color);
        }
    };

    auto setPixel = [&](int baseX, int baseY, int pxX, int pxY,
                        sf::Color color) {
        image.setPixel(Vector2u(baseX + pxX, baseY + pxY), color);
    };

    const sf::Color red(220, 30, 30, 255);
    const sf::Color dark(60, 20, 20, 255);

    // ============================================================
    // Full heart — tile (13, 1) → pixel base (208, 16)
    // ============================================================
    {
        const int bx = 13 * 16, by = 1 * 16;

        setPixel(bx, by, 4, 2, red);
        setPixel(bx, by, 5, 2, red);
        setPixel(bx, by, 10, 2, red);
        setPixel(bx, by, 11, 2, red);

        setLine(bx, by, 3, 3, 6, red);
        setLine(bx, by, 3, 9, 12, red);

        setLine(bx, by, 4, 2, 13, red);
        setLine(bx, by, 5, 2, 13, red);

        setLine(bx, by, 6, 3, 12, red);
        setLine(bx, by, 7, 4, 11, red);
        setLine(bx, by, 8, 5, 10, red);
        setLine(bx, by, 9, 6, 9, red);

        setPixel(bx, by, 7, 10, red);
        setPixel(bx, by, 8, 10, red);
    }

    // ============================================================
    // Half heart — tile (14, 1) → pixel base (224, 16)
    //   Left side: filled red   Right side: dark outline
    // ============================================================
    {
        const int bx = 14 * 16, by = 1 * 16;

        // --- Left side: filled red ---
        setPixel(bx, by, 4, 2, red);
        setPixel(bx, by, 5, 2, red);

        setLine(bx, by, 3, 3, 6, red);

        setLine(bx, by, 4, 2, 7, red);
        setLine(bx, by, 5, 2, 7, red);

        setLine(bx, by, 6, 3, 7, red);
        setLine(bx, by, 7, 4, 7, red);
        setLine(bx, by, 8, 5, 7, red);
        setLine(bx, by, 9, 6, 7, red);

        setPixel(bx, by, 7, 10, red);

        // --- Right side: dark outline ---
        setPixel(bx, by, 10, 2, dark);
        setPixel(bx, by, 11, 2, dark);

        setPixel(bx, by, 9, 3, dark);
        setLine(bx, by, 3, 10, 12, dark);

        setPixel(bx, by, 8, 4, dark);
        setLine(bx, by, 4, 9, 13, dark);

        setPixel(bx, by, 8, 5, dark);
        setLine(bx, by, 5, 9, 13, dark);

        setPixel(bx, by, 8, 6, dark);
        setLine(bx, by, 6, 9, 12, dark);

        setPixel(bx, by, 8, 7, dark);
        setLine(bx, by, 7, 9, 11, dark);

        setPixel(bx, by, 8, 8, dark);
        setLine(bx, by, 8, 9, 10, dark);

        setPixel(bx, by, 8, 9, dark);
        setPixel(bx, by, 9, 9, dark);

        setPixel(bx, by, 8, 10, dark);
    }

    // ============================================================
    // Empty heart — tile (15, 1) → pixel base (240, 16)
    //   Outline only, dark red
    // ============================================================
    {
        const int bx = 15 * 16, by = 1 * 16;

        setPixel(bx, by, 4, 2, dark);
        setPixel(bx, by, 5, 2, dark);
        setPixel(bx, by, 10, 2, dark);
        setPixel(bx, by, 11, 2, dark);

        setLine(bx, by, 3, 3, 6, dark);
        setLine(bx, by, 3, 9, 12, dark);

        setLine(bx, by, 4, 2, 13, dark);
        setLine(bx, by, 5, 2, 13, dark);

        setLine(bx, by, 6, 3, 12, dark);
        setLine(bx, by, 7, 4, 11, dark);
        setLine(bx, by, 8, 5, 10, dark);
        setLine(bx, by, 9, 6, 9, dark);

        setPixel(bx, by, 7, 10, dark);
        setPixel(bx, by, 8, 10, dark);
    }
}
