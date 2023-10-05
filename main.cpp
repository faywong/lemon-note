//
// Created by faywong on 2021/8/24.
//
#include <fstream>
#include <streambuf>
#include <iostream>

#include "imgui_md.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl2.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <ImGuiFileDialog.h>
#include <TextEditor.h>

// fs
#ifdef __APPLE__
#include <Availability.h> // for deployment target to support pre-catalina targets without std::fs 
#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || (defined(__cplusplus) && __cplusplus >= 201703L)) && defined(__has_include)
#if __has_include(<filesystem>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = fs::filesystem;
#endif
#endif
#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#define BIT(x) (1 << x)

struct AppState {
    std::string fileToEdit;
    std::string folderToView;

    AppState(): fileToEdit(""), folderToView("") {

    }
};

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

void DrawSplitter(int split_vertically, float thickness, float* size0, float* size1, float min_size0, float min_size1)
{
    ImVec2 backup_pos = ImGui::GetCursorPos();
    if (split_vertically)
        ImGui::SetCursorPosY(backup_pos.y + *size0);
    else
        ImGui::SetCursorPosX(backup_pos.x + *size0);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));          // We don't draw while active/pressed because as we move the panes the splitter button will be 1 frame late
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f,0.6f,0.6f,0.10f));
    ImGui::Button("##Splitter", ImVec2(!split_vertically ? thickness : -1.0f, split_vertically ? thickness : -1.0f));
    ImGui::PopStyleColor(3);

    ImGui::SetItemAllowOverlap(); // This is to allow having other buttons OVER our splitter. 

    if (ImGui::IsItemActive())
    {
        float mouse_delta = split_vertically ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;

        // Minimum pane size
        if (mouse_delta < min_size0 - *size0)
            mouse_delta = min_size0 - *size0;
        if (mouse_delta > *size1 - min_size1)
            mouse_delta = *size1 - min_size1;

        // Apply resize
        *size0 += mouse_delta;
        *size1 -= mouse_delta;
    }
    ImGui::SetCursorPos(backup_pos);
}

std::tuple<bool, uint32_t, std::string> DirectoryTreeViewRecursive(const fs::path& path, uint32_t* count, int* selection_mask)
{
	ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

	bool any_node_clicked = false;
	uint32_t node_clicked = 0;
    std::string selected_file = "";

	for (const auto& entry : fs::directory_iterator(path))
	{
		ImGuiTreeNodeFlags node_flags = base_flags;
		const bool is_selected = (*selection_mask & BIT(*count)) != 0;
		if (is_selected)
			node_flags |= ImGuiTreeNodeFlags_Selected;

		std::string absolute_path = entry.path().lexically_normal();
		std::string name = entry.path().string();

		auto lastSlash = name.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		name = name.substr(lastSlash, name.size() - lastSlash);

		bool entryIsFile = !fs::is_directory(entry.path());
		if (entryIsFile)
			node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)(*count), node_flags, name.c_str());

		if (ImGui::IsItemClicked())
		{
			node_clicked = *count;
			any_node_clicked = true;
            std::cout<<"any_node_clicked： "<< any_node_clicked << " absolute_path: " << absolute_path << " entryIsFile: " << entryIsFile <<std::endl;
            if (entryIsFile) {
                selected_file = absolute_path;
            }
		}

		(*count)--;

		if (!entryIsFile)
		{
			if (node_open)
			{

				auto clickState = DirectoryTreeViewRecursive(entry.path(), count, selection_mask);

				if (!any_node_clicked)
				{
					any_node_clicked = std::get<0>(clickState);
					node_clicked = std::get<1>(clickState);
                    selected_file = std::get<2>(clickState);
				}

				ImGui::TreePop();
			}
			else
			{
				for (const auto& e : fs::recursive_directory_iterator(entry.path()))
					(*count)--;
			}
		}
	}

	return { any_node_clicked, node_clicked, selected_file };
}

void UpdateEditorContent(TextEditor &editor, AppState &state) {
    // action
    std::ifstream t(state.fileToEdit);
    if (t.good())
    {
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        editor.SetText(str);
    }
}

void DrawFsTree(TextEditor &editor, AppState &appState) {
    std::string directoryPath = appState.folderToView;
	if (ImGui::CollapsingHeader("Folder", ImGuiTreeNodeFlags_DefaultOpen))
	{	
		uint32_t count = 0;
		for (const auto& entry : fs::recursive_directory_iterator(directoryPath))
			count++;

		static int selection_mask = 0;

		auto clickState = DirectoryTreeViewRecursive(directoryPath, &count, &selection_mask);

		if (std::get<0>(clickState))
		{
            std::string selected_file = std::get<2>(clickState);
            std::cout<<"selected_file: " << selected_file << std::endl;
            appState.fileToEdit = selected_file;
            UpdateEditorContent(editor, appState);
			// Update selection state
			// (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
			if (ImGui::GetIO().KeyCtrl)
				selection_mask ^= BIT(std::get<1>(clickState));          // CTRL+click to toggle
			else //if (!(selection_mask & (1 << clickState.second))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
				selection_mask = BIT(std::get<1>(clickState));           // Click to single-select
		}
	}
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
    SDL_Window* window = SDL_CreateWindow("柠檬笔记", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
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
    // default font here
    g_font_regular = io.Fonts->AddFontFromFileTTF("fonts/wqy-MicroHei.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    IM_ASSERT(g_font_regular != NULL);
    g_font_bold = io.Fonts->AddFontFromFileTTF("fonts/wqy-MicroHei.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    IM_ASSERT(g_font_bold != NULL);
    g_font_bold_large = io.Fonts->AddFontFromFileTTF("fonts/wqy-MicroHei.ttf", 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    IM_ASSERT(g_font_bold_large != NULL);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

///////////////////////////////////////////////////////////////////////
	// TEXT EDITOR SAMPLE
	TextEditor editor;
	auto lang = TextEditor::LanguageDefinition::CPlusPlus();

	// set your own known preprocessor symbols...
	static const char* ppnames[] = { "NULL", "PM_REMOVE",
		"ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI","D3D11_SDK_VERSION", "assert" };
	// ... and their corresponding values
	static const char* ppvalues[] = { 
		"#define NULL ((void*)0)", 
		"#define PM_REMOVE (0x0001)",
		"Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ", 
		"enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0", 
		"enum D3D_FEATURE_LEVEL", 
		"enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
		"#define WINAPI __stdcall",
		"#define D3D11_SDK_VERSION (7)",
		" #define assert(expression) (void)(                                                  \n"
        "    (!!(expression)) ||                                                              \n"
        "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
        " )"
		};

	for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
	{
		TextEditor::Identifier id;
		id.mDeclaration = ppvalues[i];
		lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
	}

	// set your own identifiers
	static const char* identifiers[] = {
		"HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG","LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
		"ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
		"ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
		"IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
	static const char* idecls[] = 
	{
		"typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
		"typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
		"ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
		"ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
		"IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
	for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
	{
		TextEditor::Identifier id;
		id.mDeclaration = std::string(idecls[i]);
		lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
	}
	editor.SetLanguageDefinition(lang);
	editor.SetPalette(TextEditor::GetLightPalette());

	// error markers
	// TextEditor::ErrorMarkers markers;
	// markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
	// markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
	// editor.SetErrorMarkers(markers);

	static AppState state;

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

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);

        ImVec2 viewportSize = viewport->Size;
        ImGui::SetNextWindowSize(viewportSize);
        // ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        static bool p_open = true;
        ImGui::Begin(u8"Lemon Notes", &p_open, window_flags);
        ImGui::PopStyleVar(3);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save"))
				{
					auto textToSave = editor.GetText();
                    std::ofstream out(state.fileToEdit);
                    out << textToSave;
                    out.close();
				} else if (ImGui::MenuItem("Open File")) {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".md,.cpp,.h,.hpp", 
                            ".");
				} else if (ImGui::MenuItem("Open Folder")) {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose a Directory", nullptr, ".");
				}
				if (ImGui::MenuItem("Quit", "Alt-F4"))
					break;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				bool ro = editor.IsReadOnly();
				if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
					editor.SetReadOnly(ro);
				ImGui::Separator();

				if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
					editor.Undo();
				if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
					editor.Redo();

				ImGui::Separator();

				if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
					editor.Copy();
				if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
					editor.Cut();
				if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
					editor.Delete();
				if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
					editor.Paste();

				ImGui::Separator();

				if (ImGui::MenuItem("Select all", nullptr, nullptr))
					editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Dark palette"))
					editor.SetPalette(TextEditor::GetDarkPalette());
				if (ImGui::MenuItem("Light palette"))
					editor.SetPalette(TextEditor::GetLightPalette());
				if (ImGui::MenuItem("Retro blue palette"))
					editor.SetPalette(TextEditor::GetRetroBluePalette());
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

        // display file dialog
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) 
        {
            // action if OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                state.fileToEdit = ImGuiFileDialog::Instance()->GetFilePathName();
                UpdateEditorContent(editor, state);
            }
            // close
            ImGuiFileDialog::Instance()->Close();
        }

        // display folder dialog
        if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey")) 
        {
            // action if OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                state.folderToView = ImGuiFileDialog::Instance()->GetFilePathName();
            }
            // close
            ImGuiFileDialog::Instance()->Close();
        }

        float h = viewportSize.y;
        auto thickness = 8.0f;
        float sz1 = (viewportSize.x - 2 * thickness)/5;

        // display fs tree
        ImGui::BeginChild("FsTree", ImVec2(sz1, h), true);
        DrawFsTree(editor, state);
        ImGui::EndChild();
        ImGui::SameLine();

        sz1 = 2 * (viewportSize.x - 2 * thickness)/5;
        float sz2 = sz1;
        DrawSplitter(false, thickness, &sz1, &sz2, 8, 8);

        // display editor
        ImGui::BeginChild("Editor", ImVec2(sz1, h), true);
        auto cpos = editor.GetCursorPosition();
		ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
			editor.IsOverwrite() ? "Ovr" : "Ins",
			editor.CanUndo() ? "*" : " ",
			"Markdown", state.fileToEdit.c_str());
		editor.Render("Editor");
        ImGui::EndChild();

        ImGui::SameLine();

        // display previewer
        ImGui::BeginChild("Previwer", ImVec2(sz2, h), true); // pass width here
        std::string editor_content = editor.GetText();
        markdown(editor_content.c_str(), editor_content.c_str() + editor_content.size());
        ImGui::EndChild();

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