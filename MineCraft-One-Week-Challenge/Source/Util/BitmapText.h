#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

/// @brief Text renderer using stb_truetype (Chinese-capable, zero SFML).
/// Rasterizes TTF glyphs into RGBA pixel buffer, uploads via glTexImage2D,
/// displays via ImGui::Image.
class BitmapText {
  public:
    BitmapText();
    ~BitmapText();

    GLuint update(const std::vector<std::string>& lines,
                  int texWidth, int texHeight);

  private:
    GLuint m_texID = 0;
    int m_texW = 0, m_texH = 0;
    std::vector<unsigned char> m_buffer;
    std::vector<unsigned char> m_ttfData; // TTF file in memory
    void* m_fontInfo = nullptr;           // stbtt_fontinfo*
    float m_fontSize = 24.0f;

    int renderUTF8(unsigned char* buf, int bufW, int bufH,
                   int x, int y, const std::string& text);
    int measureTextWidth(const std::string& text);
};
