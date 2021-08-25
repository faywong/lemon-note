//
// Created by faywong on 2021/8/24.
//
#include "imgui_md.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_opengl2.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Fonts and images (ImTextureID) must be loaded in other place,
//see https://github.com/ocornut/imgui/blob/master/docs/FONTS.md
static ImFont *g_font_regular;
static ImFont *g_font_bold;
static ImFont *g_font_bold_large;
static ImTextureID g_texture1;

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
    #if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    #endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

struct my_markdown : public imgui_md {
    ImFont *get_font() const override {
        if (m_is_table_header) {
            return g_font_bold;
        }

        switch (m_hlevel) {
            case 0:
                return m_is_strong ? g_font_bold : g_font_regular;
            case 1:
                return g_font_bold_large;
            default:
                return g_font_bold;
        }
    };

    void open_url() const override {
        // platform dependent code
        SDL_OpenURL(m_href.c_str());
    }

    bool get_image(image_info &nfo, const MD_SPAN_IMG_DETAIL* d) const override {
        // use m_href to identify images
        nfo.texture_id = g_texture1;
        nfo.size = {864, 486};
        nfo.uv0 = {0, 0};
        nfo.uv1 = {1, 1};
        nfo.col_tint = {1, 1, 1, 1};
        nfo.col_border = {0, 0, 0, 0};
        return true;
    }

    void html_div(const std::string &dclass, bool e) override {
        if (dclass == "red") {
            if (e) {
                m_table_border = false;
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            } else {
                ImGui::PopStyleColor();
                m_table_border = true;
            }
        }
    }
};


//call this function to render your markdown
void markdown(const char *str, const char *str_end) {
    static my_markdown s_printer;
    s_printer.print(str, str_end);
}

int main(int argc, const char *argv[]) {

    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Rendering example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // load images
    int my_image_width = 0;
    int my_image_height = 0;
    GLuint my_image_texture = 0;
    bool ret = LoadTextureFromFile("images/hynh.jpeg", &my_image_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    g_texture1 = reinterpret_cast<ImTextureID>(my_image_texture);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
//    io.Fonts->AddFontDefault();
    // default font here
    g_font_bold_large = g_font_bold = g_font_regular = io.Fonts->AddFontFromFileTTF("fonts/wqy-MicroHei.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
//    g_font_bold = io.Fonts->AddFontFromFileTTF("fonts/Ubuntu-Bold.ttf", 16.0f);
//    g_font_bold_large = io.Fonts->AddFontFromFileTTF("fonts/Ubuntu-Bold.ttf", 24.0f);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin(u8"markdown 渲染器样例");

        std::string markdown_str = u8"# Sid's 表格\n\n"
                          "Name &nbsp; &nbsp; &nbsp; &nbsp; | Multiline &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;<br>header  | [Link&nbsp;](#link1)\n"
                          ":------|:-------------------|:--\n"
                          "Value-One | Long <br>explanation <br>with \\<br\\>\\'s|1\n"
                          "~~Value-Two~~ | __text auto wrapped__\\: long explanation here |25 37 43 56 78 90\n"
                          "**etc** | [~~Some **link**~~](https://github.com/faywong)|3\n"
                          "\n"
                          " - [x] Marked item\n"
                          " - [ ] Unmarked item\n"
                          " - Ordinary (non-task) item\n\n"
                          u8"# 中文List\n"
                         "\n"
                         "1. First ordered list item\n"
                         "2. Another item\n"
                         "   * Unordered sub-list 1.\n"
                         "   * Unordered sub-list 2.\n"
                         "1. Actual numbers don't matter, just that it's a number\n"
                         "   1. **Ordered** sub-list 1\n"
                         "   2. **Ordered** sub-list 2\n"
                         "4. And another item with minuses.\n"
                         "   - __sub-list with underline__\n"
                         "   - sub-list with escapes: \\[looks like\\]\\(a link\\)\n"
                         "5. ~~Item with pluses and strikethrough~~.\n"
                         "   + sub-list 1\n"
                         "   + sub-list 2\n"
                         "   + [Just a link](https://github.com/mekhontsev/imgui_md).\n"
                         "      * Item with [link1](#link1)\n"
                         "      * Item with bold [**link2**](#link1)\n"
                                   "## 图片样例\n"
                                   "![image](https://gimg2.baidu.com/image_search/src=http%3A%2F%2Fvsd-picture.cdn.bcebos.com%2F7c791bffb7e97059897391d24e34969603de76e3.jpg&refer=http%3A%2F%2Fvsd-picture.cdn.bcebos.com&app=2002&size=f9999,10000&q=a80&n=0&g=0n&fmt=jpeg?sec=1632473708&t=fd59bba5438dd17d217f0bd224a78e76)\n";

//        std::string markdown_str = "<div class = \"red\">\n"
//                                   "\n"
//                                   "This table | is inside an | HTML div\n"
//                                   "--- | --- | ---\n"
//                                   "Still | ~~renders~~ | __nicely__\n"
//                                   "Border | **is not** | visible\n"
//                                   "\n"
//                                   "</div>";

//        std::string markdown_str = u8"# 中文List\n"
//                                   "\n"
//                                   "1. First ordered list item\n"
//                                   "2. Another item\n"
//                                   "   * Unordered sub-list 1.\n"
//                                   "   * Unordered sub-list 2.\n"
//                                   "1. Actual numbers don't matter, just that it's a number\n"
//                                   "   1. **Ordered** sub-list 1\n"
//                                   "   2. **Ordered** sub-list 2\n"
//                                   "4. And another item with minuses.\n"
//                                   "   - __sub-list with underline__\n"
//                                   "   - sub-list with escapes: \\[looks like\\]\\(a link\\)\n"
//                                   "5. ~~Item with pluses and strikethrough~~.\n"
//                                   "   + sub-list 1\n"
//                                   "   + sub-list 2\n"
//                                   "   + [Just a link](https://github.com/mekhontsev/imgui_md).\n"
//                                   "      * Item with [link1](#link1)\n"
//                                   "      * Item with bold [**link2**](#link1)";
        markdown(markdown_str.c_str(), markdown_str.c_str() + markdown_str.size());
        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}