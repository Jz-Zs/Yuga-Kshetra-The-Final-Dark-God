#include "GUI.h"

#include <print>

#include <glad/glad.h>
#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_sfml/imgui-SFML.h>

// Minimal modern OpenGL (3.3+) renderer for ImGui
// Replaces ImGui::SFML::Render() which uses legacy GL 1.x immediate mode
// that is unavailable in OpenGL 4.6 Core Profile.
namespace
{
GLuint g_ShaderProgram = 0;
GLuint g_VBO = 0;
GLuint g_EBO = 0;
GLuint g_VAO = 0;
GLint  g_ProjLoc = -1;
GLint  g_TexLoc  = -1;

const char* kVertexShader = R"glsl(
#version 330 core
uniform mat4 ProjMtx;
layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec4 Color;
out vec2 Frag_UV;
out vec4 Frag_Color;
void main() {
    Frag_UV = UV;
    Frag_Color = Color;
    gl_Position = ProjMtx * vec4(Position.xy, 0.0, 1.0);
}
)glsl";

const char* kFragmentShader = R"glsl(
#version 330 core
uniform sampler2D Texture;
in vec2 Frag_UV;
in vec4 Frag_Color;
layout(location = 0) out vec4 Out_Color;
void main() {
    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
}
)glsl";

GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::println(std::cerr, "ImGui shader compile error: {}", log);
    }
    return s;
}

void createDeviceObjects()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    g_ShaderProgram = glCreateProgram();
    glAttachShader(g_ShaderProgram, vs);
    glAttachShader(g_ShaderProgram, fs);
    glLinkProgram(g_ShaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    g_ProjLoc = glGetUniformLocation(g_ShaderProgram, "ProjMtx");
    g_TexLoc  = glGetUniformLocation(g_ShaderProgram, "Texture");

    glGenBuffers(1, &g_VBO);
    glGenBuffers(1, &g_EBO);
    glGenVertexArrays(1, &g_VAO);
}

void destroyDeviceObjects()
{
    if (g_VAO) { glDeleteVertexArrays(1, &g_VAO); g_VAO = 0; }
    if (g_VBO) { glDeleteBuffers(1, &g_VBO);  g_VBO = 0; }
    if (g_EBO) { glDeleteBuffers(1, &g_EBO);  g_EBO = 0; }
    if (g_ShaderProgram) { glDeleteProgram(g_ShaderProgram); g_ShaderProgram = 0; }
}

void renderImGui(ImDrawData* draw_data)
{
    if (!draw_data || draw_data->CmdListsCount == 0)
        return;

    const ImGuiIO& io = ImGui::GetIO();
    int fb_w = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_h = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_w <= 0 || fb_h <= 0)
        return;

    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup OpenGL state
    GLint last_program = 0, last_texture = 0, last_array_buffer = 0;
    GLint last_element_array_buffer = 0, last_vertex_array = 0;
    GLint last_blend_src = 0, last_blend_dst = 0;
    GLint last_scissor[4] = {};
    GLboolean last_blend = glIsEnabled(GL_BLEND);
    GLboolean last_cull  = glIsEnabled(GL_CULL_FACE);
    GLboolean last_depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst);
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor);

    // Set ImGui render state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    glUseProgram(g_ShaderProgram);
    glBindVertexArray(g_VAO);

    // Orthographic projection for ImGui
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float ortho[4][4] = {
        { 2.0f/(R-L),    0.0f,           0.0f, 0.0f },
        { 0.0f,          2.0f/(T-B),     0.0f, 0.0f },
        { 0.0f,          0.0f,          -1.0f, 0.0f },
        { (R+L)/(L-R),   (T+B)/(B-T),    0.0f, 1.0f },
    };
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, &ortho[0][0]);
    glUniform1i(g_TexLoc, 0);

    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          (void*)offsetof(ImDrawVert, pos));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          (void*)offsetof(ImDrawVert, uv));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                          (void*)offsetof(ImDrawVert, col));

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                     cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                     cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int i = 0; i < cmd_list->CmdBuffer.Size; i++) {
            const ImDrawCmd* cmd = &cmd_list->CmdBuffer[i];
            if (cmd->UserCallback) {
                if (cmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ; // Not needed for our simple renderer
                else
                    cmd->UserCallback(cmd_list, cmd);
            } else {
                ImVec4 clip = cmd->ClipRect;
                clip.x -= draw_data->DisplayPos.x;
                clip.y -= draw_data->DisplayPos.y;
                clip.z -= draw_data->DisplayPos.x;
                clip.w -= draw_data->DisplayPos.y;
                clip.x *= draw_data->FramebufferScale.x;
                clip.y *= draw_data->FramebufferScale.y;
                clip.z *= draw_data->FramebufferScale.x;
                clip.w *= draw_data->FramebufferScale.y;

                if (clip.x < (float)fb_w && clip.y < (float)fb_h &&
                    clip.z >= 0.0f && clip.w >= 0.0f) {
                    glScissor((int)clip.x, (int)((float)fb_h - clip.w),
                              (int)(clip.z - clip.x), (int)(clip.w - clip.y));
                    GLuint tex = (GLuint)(intptr_t)cmd->GetTexID();
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glDrawElements(GL_TRIANGLES, (GLsizei)cmd->ElemCount,
                                   sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT
                                                          : GL_UNSIGNED_INT,
                                   (void*)(intptr_t)(cmd->IdxOffset * sizeof(ImDrawIdx)));
                }
            }
        }
    }

    // Restore OpenGL state
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendFunc(last_blend_src, last_blend_dst);
    if (!last_blend) glDisable(GL_BLEND);
    if (last_cull)   glEnable(GL_CULL_FACE);
    if (last_depth)  glEnable(GL_DEPTH_TEST);
    if (!last_scissor_test) glDisable(GL_SCISSOR_TEST);
    glScissor(last_scissor[0], last_scissor[1], last_scissor[2], last_scissor[3]);
}

} // anonymous namespace

namespace GUI
{
    bool init(sf::Window* window)
    {
        if (!ImGui::SFML::Init(*window, sf::Vector2f{window->getSize()}, false))
            return false;
        createDeviceObjects();

        // Manually create font texture (ImGui::SFML::UpdateFontTexture is not exported)
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        if (pixels == nullptr || width <= 0 || height <= 0)
            return false;

        GLuint fontTex = 0;
        glGenTextures(1, &fontTex);
        glBindTexture(GL_TEXTURE_2D, fontTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        io.Fonts->TexData->SetTexID((ImTextureID)(intptr_t)fontTex);
        io.Fonts->TexData->SetStatus(ImTextureStatus_OK);

        return true;
    }

    void begin_frame(sf::Window& window, sf::Time dt)
    {
        ImGui::SFML::SetCurrentWindow(window);
        auto mousePos = sf::Mouse::getPosition(window);
        auto size = window.getSize();
        ImGui::SFML::Update(mousePos, sf::Vector2f(size), dt);
    }

    void shutdown()
    {
        destroyDeviceObjects();
        ImGui::SFML::Shutdown();
    }

    void render(sf::Window& window)
    {
        ImGui::SFML::SetCurrentWindow(window);
        ImGui::Render();
        renderImGui(ImGui::GetDrawData());
    }

    void event(const sf::Window& window, sf::Event& e)
    {
        ImGui::SFML::ProcessEvent(window, e);
    }

} // namespace GUI
