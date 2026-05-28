#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

/// @brief Minimal software-rendered bitmap text.
/// Renders ASCII text into an RGBA pixel buffer, uploads to GL texture,
/// designed for display via ImGui::Image (bypasses all font systems).
class BitmapText {
  public:
    BitmapText();
    ~BitmapText();

    /// Render text lines into internal texture, return GL texture ID.
    /// Call every frame with current text. Re-uploads only when content changes.
    GLuint update(const std::vector<std::string>& lines, int width, int height);

  private:
    void drawChar(unsigned char* buf, int bufW, int x, int y, char c);
    void drawString(unsigned char* buf, int bufW, int x, int y,
                    const std::string& s, int maxWidth);

    GLuint m_texID = 0;
    int m_texW = 0, m_texH = 0;
    std::vector<unsigned char> m_buffer;
    std::string m_lastContent; // For change detection

    // Minimal 5x7 font in 8x8 cells (ASCII 32-126)
    static const unsigned char s_font[95][8];
};
