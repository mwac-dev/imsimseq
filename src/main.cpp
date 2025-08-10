// Copyright (c) Created by MWAC-dev on 2025.
// src/main.cpp
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <iostream>

#include <SDL3/SDL.h>
// #include <SDL3/SDL_opengl.h>
#include <glad/glad.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"

#include <nlohmann/json.hpp>
#include "ims/catalogs.hpp"
#include "ims/json_adapters.hpp"

using json = nlohmann::json;

namespace {

[[noreturn]] void sdl_panic(const char* msg) {
    SDL_Log("%s: %s", msg, SDL_GetError());
    std::abort();
}

std::string join_tags(const std::vector<std::string>& tags) {
    std::string out;
    for (size_t i = 0; i < tags.size(); ++i) {
        out += tags[i];
        if (i + 1 < tags.size()) out += ", ";
    }
    return out;
}

// Simple case-insensitive contains
bool icontains(std::string_view hay, std::string_view needle) {
    auto tolower = [](unsigned char c){ return char(std::tolower(c)); };
    std::string H(hay.begin(), hay.end());
    std::string N(needle.begin(), needle.end());
    std::transform(H.begin(), H.end(), H.begin(), tolower);
    std::transform(N.begin(), N.end(), N.begin(), tolower);
    return H.find(N) != std::string::npos;
}

// Recursively render a json value as an ImGui tree
void RenderJsonTree(const json& j, const char* label = nullptr) {
    using value_t = json::value_t;

    auto nodeLabel = [&](const char* fallback)->std::string {
        if (label && *label) return label;
        return fallback;
    };

    switch (j.type()) {
        case value_t::object: {
            if (ImGui::TreeNode(nodeLabel("object").c_str(), "%s { %zu }", label ? label : "object", j.size())) {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    RenderJsonTree(it.value(), it.key().c_str());
                }
                ImGui::TreePop();
            }
            break;
        }
        case value_t::array: {
            if (ImGui::TreeNode(nodeLabel("array").c_str(), "%s [ %zu ]", label ? label : "array", j.size())) {
                for (size_t i = 0; i < j.size(); ++i) {
                    std::string idx = std::to_string(i);
                    RenderJsonTree(j[i], idx.c_str());
                }
                ImGui::TreePop();
            }
            break;
        }
        case value_t::string: {
            ImGui::BulletText("%s: \"%s\"", label ? label : "string", j.get_ref<const std::string&>().c_str());
            break;
        }
        case value_t::boolean: {
            ImGui::BulletText("%s: %s", label ? label : "bool", j.get<bool>() ? "true" : "false");
            break;
        }
        case value_t::number_integer:
        case value_t::number_unsigned: {
            ImGui::BulletText("%s: %lld", label ? label : "int", static_cast<long long>(j.get<long long>()));
            break;
        }
        case value_t::number_float: {
            ImGui::BulletText("%s: %g", label ? label : "float", j.get<double>());
            break;
        }
        case value_t::null: {
            ImGui::BulletText("%s: null", label ? label : "null");
            break;
        }
        default: {
            // fallback: dump
            std::string dumped = j.dump();
            ImGui::BulletText("%s: %s", label ? label : "value", dumped.c_str());
            break;
        }
    }
}

struct AppState {
    ims::Bundle bundle;
    int selected = -1;
    std::string filter;
};

bool LoadBundleFromFile(const char* path, AppState& state, std::string& errorOut) {
    std::ifstream f(path);
    if (!f) { errorOut = std::string("failed to open: ") + path; return false; }

    try {
        json j; f >> j;
        state.bundle = {};
        state.bundle.config_version = j.value("config_version", "1.0.0");

        if (j.contains("catalogs") && j["catalogs"].contains("materials")) {
            state.bundle.catalogs.materials =
                j["catalogs"]["materials"].get<std::vector<ims::Material>>();
        }
        state.selected = state.bundle.catalogs.materials.empty() ? -1 : 0;
        return true;
    } catch (const std::exception& e) {
        errorOut = std::string("JSON error: ") + e.what();
        return false;
    }
}

} // namespace

int main(int argc, char** argv) {
    // CLI: optional path param. If not provided, just open an empty UI.
    const char* path = argc > 1 ? argv[1] : nullptr;

    if (!SDL_Init(SDL_INIT_VIDEO)) sdl_panic("SDL_Init failed");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("imsimseq", 1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) sdl_panic("SDL_CreateWindow failed");

    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    if (!glctx) sdl_panic("SDL_GL_CreateContext failed");
    if (!SDL_GL_MakeCurrent(window, glctx)) sdl_panic("SDL_GL_MakeCurrent failed");
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) sdl_panic("gladLoadGLLoader failed");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForOpenGL(window, glctx)) sdl_panic("ImGui_ImplSDL3_InitForOpenGL failed");
    ImGui_ImplOpenGL3_Init("#version 460");

    AppState state{};
    std::string loadErr;
    if (path) {
        if (!LoadBundleFromFile(path, state, loadErr)) {
            std::cerr << loadErr << "\n";
        }
    }

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Main dockspace so you can tear off windows later
        {
            static bool p_open = true;
            static ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                         ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;
            const ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->WorkPos);
            ImGui::SetNextWindowSize(vp->WorkSize);
            ImGui::SetNextWindowViewport(vp->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            win_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::Begin("DockSpace", &p_open, win_flags);
            ImGui::PopStyleVar(2);

            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0, 0), dock_flags);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Quit", "Alt+F4")) running = false;
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        // Materials panel
        ImGui::Begin("Materials");
        // Filter bar
        {
            // ImGui::InputTextWithHint("##filter", "Filter by id/name/tag...", &state.filter);
            ImGui::SameLine();
            if (ImGui::Button("Clear")) { state.filter.clear(); }
            ImGui::SameLine();
            ImGui::TextDisabled("count: %zu", state.bundle.catalogs.materials.size());
        }

        ImGui::Separator();
        // Table header
        if (ImGui::BeginTable("materials_table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Tags", ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableHeadersRow();

            // Rows
            const auto& mats = state.bundle.catalogs.materials;
            for (int i = 0; i < (int)mats.size(); ++i) {
                const auto& m = mats[i];
                if (!state.filter.empty()) {
                    bool hit = icontains(m.id, state.filter) ||
                               icontains(m.name, state.filter);
                    if (!hit) {
                        for (auto& t : m.tags) {
                            if (icontains(t, state.filter)) { hit = true; break; }
                        }
                    }
                    if (!hit) continue;
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                bool selected = (state.selected == i);
                if (ImGui::Selectable(m.id.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    state.selected = i;
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(m.name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(join_tags(m.tags).c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();

        // Inspector panel
        ImGui::Begin("Inspector");
        if (state.selected >= 0 && state.selected < (int)state.bundle.catalogs.materials.size()) {
            const auto& m = state.bundle.catalogs.materials[state.selected];
            ImGui::Text("ID: %s", m.id.c_str());
            ImGui::Text("Name: %s", m.name.c_str());

            if (!m.tags.empty()) {
                ImGui::SeparatorText("Tags");
                for (auto& t : m.tags) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("[%s]", t.c_str());
                }
                ImGui::NewLine();
            }

            ImGui::SeparatorText("Properties");
            if (m.properties.is_null() || (m.properties.is_object() && m.properties.empty())) {
                ImGui::TextDisabled("<empty>");
            } else {
                // Tree view for JSON properties
                RenderJsonTree(m.properties, "properties");
            }
        } else {
            ImGui::TextDisabled("No material selected.");
        }
        ImGui::End();

        // Render
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
