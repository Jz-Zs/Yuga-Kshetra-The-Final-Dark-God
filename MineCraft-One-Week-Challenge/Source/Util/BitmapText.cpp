#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "BitmapText.h"

#include <fstream>
#include <iostream>
#include <vector>

// Simple UTF-8 decoder: returns next codepoint and advances pointer
static int utf8_decode(const char** str)
{
    const unsigned char* s = (const unsigned char*)*str;
    if (s[0] < 0x80) { (*str)++; return s[0]; }
    if ((s[0] & 0xE0) == 0xC0) { (*str)+=2; return ((s[0]&0x1F)<<6)|(s[1]&0x3F); }
    if ((s[0] & 0xF0) == 0xE0) { (*str)+=3; return ((s[0]&0x0F)<<12)|((s[1]&0x3F)<<6)|(s[2]&0x3F); }
    if ((s[0] & 0xF8) == 0xF0) { (*str)+=4; return ((s[0]&0x07)<<18)|((s[1]&0x3F)<<12)|((s[2]&0x3F)<<6)|(s[3]&0x3F); }
    (*str)++; return '?';
}

// Read entire file into memory
static std::vector<unsigned char> readFile(const char* path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto sz = f.tellg();
    f.seekg(0);
    std::vector<unsigned char> data((size_t)sz);
    f.read((char*)data.data(), sz);
    return data;
}

BitmapText::BitmapText()
{
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\simhei.ttf",
        "C:\\Windows\\Fonts\\simsun.ttc",
        "C:\\Windows\\Fonts\\arial.ttf",
        nullptr
    };

    for (int i = 0; fontPaths[i]; i++) {
        m_ttfData = readFile(fontPaths[i]);
        if (!m_ttfData.empty()) {
            m_fontInfo = malloc(sizeof(stbtt_fontinfo));
            int off = stbtt_GetFontOffsetForIndex(m_ttfData.data(), 0);
            if (stbtt_InitFont((stbtt_fontinfo*)m_fontInfo, m_ttfData.data(), off)) {
                std::cout << "[BitmapText] stb_truetype loaded: "
                          << fontPaths[i] << std::endl;
                return;
            }
            free(m_fontInfo);
            m_fontInfo = nullptr;
            m_ttfData.clear();
        }
    }
    std::cout << "[BitmapText] No TTF found, text disabled." << std::endl;
}

BitmapText::~BitmapText()
{
    if (m_texID) glDeleteTextures(1, &m_texID);
    if (m_fontInfo) free(m_fontInfo);
}

int BitmapText::renderUTF8(unsigned char* buf, int bufW, int bufH,
                            int x, int y, const std::string& text)
{
    auto* fi = (stbtt_fontinfo*)m_fontInfo;
    if (!fi) return 0;

    float scale = stbtt_ScaleForPixelHeight(fi, m_fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(fi, &ascent, &descent, &lineGap);
    int baseline = (int)(ascent * scale);

    int cx = x;
    const char* s = text.c_str();
    int prevCp = 0;
    while (*s) {
        int cp = utf8_decode(&s);

        // Kerning from previous character
        if (prevCp)
            cx += (int)(scale * stbtt_GetCodepointKernAdvance(fi, prevCp, cp));

        int gw, gh, xoff, yoff;
        unsigned char* glyph = stbtt_GetCodepointBitmap(
            fi, scale, scale, cp, &gw, &gh, &xoff, &yoff);

        if (glyph) {
            int drawX = cx + xoff;
            int drawY = y + baseline + yoff;
            for (int row = 0; row < gh; row++) {
                int py = drawY + row;
                if (py < 0 || py >= bufH) continue;
                for (int col = 0; col < gw; col++) {
                    int px = drawX + col;
                    if (px < 0 || px >= bufW) continue;
                    unsigned char alpha = glyph[row * gw + col];
                    if (alpha == 0) continue;
                    int off = (py * bufW + px) * 4;
                    buf[off+0] = 255;
                    buf[off+1] = 255;
                    buf[off+2] = 255;
                    buf[off+3] = (unsigned char)std::min(255, buf[off+3] + alpha);
                }
            }
            stbtt_FreeBitmap(glyph, nullptr);
        }

        int adv;
        stbtt_GetCodepointHMetrics(fi, cp, &adv, nullptr);
        cx += (int)(adv * scale);
        prevCp = cp;
    }

    return cx;
}

int BitmapText::measureTextWidth(const std::string& text)
{
    auto* fi = (stbtt_fontinfo*)m_fontInfo;
    if (!fi) return (int)text.size() * 8;
    float scale = stbtt_ScaleForPixelHeight(fi, m_fontSize);
    int w = 0, prevCp = 0;
    const char* s = text.c_str();
    while (*s) {
        int cp = utf8_decode(&s);
        if (prevCp)
            w += (int)(scale * stbtt_GetCodepointKernAdvance(fi, prevCp, cp));
        int adv;
        stbtt_GetCodepointHMetrics(fi, cp, &adv, nullptr);
        w += (int)(adv * scale);
        prevCp = cp;
    }
    return w;
}

GLuint BitmapText::update(const std::vector<std::string>& lines,
                           int texWidth, int texHeight, bool center)
{
    if (!m_fontInfo) {
        // No font: return empty texture
        if (m_texID == 0) glGenTextures(1, &m_texID);
        return m_texID;
    }

    // Allocate buffer
    int bufSize = texWidth * texHeight * 4;
    if ((int)m_buffer.size() != bufSize)
        m_buffer.assign(bufSize, 0);
    else
        std::memset(m_buffer.data(), 0, bufSize);

    int y = 4;
    for (auto& line : lines) {
        int x = 4;
        if (center) {
            int lineW = measureTextWidth(line);
            x = (texWidth - lineW) / 2;
            if (x < 0) x = 0;
        }
        renderUTF8(m_buffer.data(), texWidth, texHeight, x, y, line);
        renderUTF8(m_buffer.data(), texWidth, texHeight, x + 1, y, line); // bold: +1px offset
        y += (int)(m_fontSize * 1.1f);
    }

    // Upload
    if (m_texID == 0)
        glGenTextures(1, &m_texID);
    glBindTexture(GL_TEXTURE_2D, m_texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (texWidth != m_texW || texHeight != m_texH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, m_buffer.data());
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight,
                        GL_RGBA, GL_UNSIGNED_BYTE, m_buffer.data());
    }
    m_texW = texWidth;
    m_texH = texHeight;

    return m_texID;
}
