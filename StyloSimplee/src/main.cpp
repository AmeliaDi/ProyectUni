#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "StylophoneSynth.h"
#include "Sequencer.h"

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

// =========================================================================
// Constantes locas de diseño pa que se vea bn retro el proyecto
// =========================================================================

// las notas q suenan, aumentadas a 44 teclas (C2 a G5)
const std::vector<std::string> STYLOPHONE_KEYS = {
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5"
};

struct ThemeColors {
    ImVec4 bg;
    ImVec4 text;
    ImVec4 accent;
    ImVec4 keyIdle;
    ImVec4 keyHover;
    ImVec4 keyActive;
    ImVec4 btnPlay;
    ImVec4 btnRecord;
    ImVec4 btnClear;
};

ThemeColors g_colors;
int g_currentTheme = 0;

// Me robé un cacho del demo original de ImGui y lo modifiqué 
// para inyectar nuestros propios colores dinámicos a la interfaz entera.
void ApplyTheme(int themeIndex) {
    g_currentTheme = themeIndex;
    switch (themeIndex) {
        case 0: // Ámbar Retro (Default)
            g_colors = {
                ImVec4(0.12f, 0.12f, 0.12f, 1.00f), ImVec4(1.00f, 0.78f, 0.17f, 1.00f), ImVec4(0.85f, 0.50f, 0.10f, 1.00f),
                ImVec4(0.40f, 0.45f, 0.50f, 1.00f), ImVec4(0.50f, 0.55f, 0.60f, 1.00f), ImVec4(0.80f, 0.60f, 0.20f, 1.00f),
                ImVec4(0.20f, 0.60f, 0.30f, 1.00f), ImVec4(0.70f, 0.20f, 0.20f, 1.00f), ImVec4(0.60f, 0.50f, 0.10f, 1.00f)
            };
            break;
        case 1: // Matrix Verde
            g_colors = {
                ImVec4(0.05f, 0.05f, 0.05f, 1.00f), ImVec4(0.20f, 1.00f, 0.20f, 1.00f), ImVec4(0.10f, 0.80f, 0.10f, 1.00f),
                ImVec4(0.10f, 0.30f, 0.10f, 1.00f), ImVec4(0.20f, 0.50f, 0.20f, 1.00f), ImVec4(0.40f, 1.00f, 0.40f, 1.00f),
                ImVec4(0.10f, 0.40f, 0.10f, 1.00f), ImVec4(0.50f, 0.10f, 0.10f, 1.00f), ImVec4(0.40f, 0.40f, 0.10f, 1.00f)
            };
            break;
        case 2: // Blanco y Negro
            g_colors = {
                ImVec4(0.00f, 0.00f, 0.00f, 1.00f), ImVec4(1.00f, 1.00f, 1.00f, 1.00f), ImVec4(0.70f, 0.70f, 0.70f, 1.00f),
                ImVec4(0.20f, 0.20f, 0.20f, 1.00f), ImVec4(0.40f, 0.40f, 0.40f, 1.00f), ImVec4(0.90f, 0.90f, 0.90f, 1.00f),
                ImVec4(0.30f, 0.30f, 0.30f, 1.00f), ImVec4(0.50f, 0.50f, 0.50f, 1.00f), ImVec4(0.40f, 0.40f, 0.40f, 1.00f)
            };
            break;
        case 3: // Monokai Hacker
            g_colors = {
                ImVec4(0.15f, 0.16f, 0.13f, 1.00f), ImVec4(0.97f, 0.97f, 0.95f, 1.00f), ImVec4(0.90f, 0.15f, 0.45f, 1.00f),
                ImVec4(0.40f, 0.85f, 0.94f, 1.00f), ImVec4(0.65f, 0.89f, 0.30f, 1.00f), ImVec4(0.99f, 0.59f, 0.00f, 1.00f),
                ImVec4(0.65f, 0.89f, 0.30f, 1.00f), ImVec4(0.90f, 0.15f, 0.45f, 1.00f), ImVec4(0.99f, 0.59f, 0.00f, 1.00f)
            };
            break;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg]      = g_colors.bg;
    style.Colors[ImGuiCol_Text]          = g_colors.text;
    style.Colors[ImGuiCol_FrameBg]       = ImVec4(g_colors.bg.x + 0.08f, g_colors.bg.y + 0.08f, g_colors.bg.z + 0.08f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]= ImVec4(g_colors.bg.x + 0.15f, g_colors.bg.y + 0.15f, g_colors.bg.z + 0.15f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = g_colors.accent;
    style.Colors[ImGuiCol_Button]        = ImVec4(g_colors.bg.x + 0.15f, g_colors.bg.y + 0.15f, g_colors.bg.z + 0.15f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(g_colors.bg.x + 0.25f, g_colors.bg.y + 0.25f, g_colors.bg.z + 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]  = g_colors.accent;
    style.Colors[ImGuiCol_SliderGrab]    = g_colors.accent;
    style.Colors[ImGuiCol_SliderGrabActive] = g_colors.text;
    style.Colors[ImGuiCol_ChildBg]       = ImVec4(g_colors.bg.x - 0.03f, g_colors.bg.y - 0.03f, g_colors.bg.z - 0.03f, 1.00f);

    style.WindowRounding    = 0.0f; 
    style.FrameRounding     = 0.0f; // Botones cuadrados originales pero con espaciado
    style.ScrollbarRounding = 0.0f;
    style.ItemSpacing       = ImVec2(14, 14); // Mejor espaciado general
    style.FramePadding      = ImVec2(12, 12); // Más espacio para las letras en las cajitas
}

int main(int argc, char* argv[]) {
    // prender el sdl2 y si no jala ps nos salimos ALV
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "Ya valio madre SDL2: " << SDL_GetError() << std::endl;
        return -1;
    }

#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // Dejo la ventana a 750 pq mi laptop no aguanta mas reso, a parte el wayland rompe el auto size
    SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Window* window = SDL_CreateWindow("StyloSimle", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          750, 480, window_flags);
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // prender el Vsync a fuerzas p q no gaste la bateria

    // ==========================================================
    // Inicialización de la Interfaz (Dear ImGui)
    // ==========================================================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Aplicamos el tema por defecto (Ámbar Retro)
    ApplyTheme(g_currentTheme);
    
    // Cargamos una tipografía Pixel-Art (PressStart2P) que me bajé de internet.
    // Buscamos en varias rutas por si corren el .exe desde diferentes lados.
    std::string fontPath = "assets/PressStart2P.ttf";
    if (!std::filesystem::exists(fontPath)) {
        fontPath = "../assets/PressStart2P.ttf"; 
    }

    if (std::filesystem::exists(fontPath)) {
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
    } else {
        std::cout << "[INFO] Chale, no encontré la fuente en " << fontPath << " (Usando fuente default)\n";
    }

    // Le decimos a ImGui que dibuje encima de la ventana SDL que hicimos arriba.
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // ==========================================================
    // Inicialización del Motor de Audio y Secuenciador
    // ==========================================================
    StylophoneSynth synth;
    if (!synth.init()) {
        std::cerr << "Valió gorro el motor de audio, cerrando aplicación." << std::endl;
        return -1;
    }

    Sequencer sequencer;
    // ¡Trucazo de C++ moderno! Le pasamos funciones anónimas (lambdas) al secuenciador 
    // para conectar el Cerebro Rítmico con la Tarjeta de Sonido sin que las clases se enreden.
    sequencer.setNoteCallbacks(
        [&synth](int track, float freq) { synth.noteOn(track, freq); },
        [&synth](int track) { synth.noteOff(track); }
    );

    // ==========================================================
    // Bucle Principal (El "Game Loop")
    // ==========================================================
    bool done = false;
    Uint32 lastTime = SDL_GetTicks();
    
    // Aquí es donde vive la memoria de las pistas (las cajitas de texto de la UI)
    // ImGui usa arrays de 'char' al estilo lenguaje C, así que necesitamos sincronizarlos 
    // con los vectores más modernos de C++ (std::string) que usa el secuenciador internamente.
    std::vector<std::array<char, 2048>> sequenceBuffers(sequencer.getTrackCount());
    for (size_t i = 0; i < sequenceBuffers.size(); ++i) {
        sequenceBuffers[i].fill(0);
        strncpy(sequenceBuffers[i].data(), sequencer.getSequenceText(i).c_str(), 2047);
    }
    
    std::string currentUINote = "";
    int activeRecordTrack = 0; // Pista seleccionada para grabar

    while (!done) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // event handler d sdl
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        sequencer.update(deltaTime);

        // Copiamos la mem del string std a la caja d char y viceversa segun sea
        // Sincronizar numero de pistas si se agregaron
        if (sequencer.getTrackCount() > sequenceBuffers.size()) {
            size_t oldSize = sequenceBuffers.size();
            sequenceBuffers.resize(sequencer.getTrackCount());
            for (size_t i = oldSize; i < sequenceBuffers.size(); ++i) {
                sequenceBuffers[i].fill(0);
                strncpy(sequenceBuffers[i].data(), sequencer.getSequenceText(i).c_str(), 2047);
            }
        }

        if (sequencer.getMode() == SequencerMode::Recording) {
            for (size_t i = 0; i < sequenceBuffers.size(); ++i) {
                strncpy(sequenceBuffers[i].data(), sequencer.getSequenceText(i).c_str(), 2047);
            }
        } else if (sequencer.getMode() == SequencerMode::Idle) {
            for (size_t i = 0; i < sequenceBuffers.size(); ++i) {
                sequencer.setSequenceText(i, sequenceBuffers[i].data());
            }
        }

        // --- Render UI ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Que ocupe tooddoooo el espacio p q se vea bonito
        ImGuiWindowFlags uiFlags = ImGuiWindowFlags_NoResize 
                                 | ImGuiWindowFlags_NoMove
                                 | ImGuiWindowFlags_NoTitleBar
                                 | ImGuiWindowFlags_NoSavedSettings
                                 | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("HardwarePanel", nullptr, uiFlags);
        
        // pa presumir al profe
        ImGui::TextDisabled("=== Proyecto Proga StyloSimple V1.3 - Compiladote ===");
        
        // Poner el combobox de temas a la derecha
        ImGui::SameLine(ImGui::GetWindowWidth() - 360);
        ImGui::SetNextItemWidth(300);
        const char* themes[] = { "Ambar Retro (Fer)", "Matrix Verde (enora)", "Blanco y Negro (enora)", "Monokai Hacker (Fer)" };
        if (ImGui::Combo("##tema", &g_currentTheme, themes, IM_ARRAYSIZE(themes))) {
            ApplyTheme(g_currentTheme);
        }
        
        ImGui::Spacing();
        
        ImGui::Text(">> CONTACTOS DE TECLADO (Usar Stylus/Raton)");
        ImGui::TextDisabled("Rango extendido: Puedes escribir notas desde C2 hasta G5 (44 teclas activas).");
        ImGui::Spacing();
        bool anyKeyHeld = false;

        // Pinta el teclado chido con un for
        for (int i = 0; i < STYLOPHONE_KEYS.size(); ++i) {
            const auto& note = STYLOPHONE_KEYS[i];
            
            bool isPlayingFromSeq = false;
            if (sequencer.getMode() == SequencerMode::Playing) {
                // Checamos si esta sonando en CUALQUIER pista
                for (size_t t = 0; t < sequencer.getTrackCount(); ++t) {
                    if (sequencer.getCurrentPlayingNote(t) == note) {
                        isPlayingFromSeq = true;
                        break;
                    }
                }
            }
            
            if (isPlayingFromSeq) {
                ImGui::PushStyleColor(ImGuiCol_Button, g_colors.keyActive);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_colors.keyActive);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,0,0,1)); // texto negro se lee mas bn
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, g_colors.keyIdle);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_colors.keyHover);
                ImGui::PushStyleColor(ImGuiCol_Text, g_colors.text);
            }

            if (i > 0 && i % 11 != 0) ImGui::SameLine(); // baja la linea al 11vo boton
            
            // Hacemos el botón un poco más ancho (65) para que quepan las notas con '#' sin apretarse
            ImGui::Button(note.c_str(), ImVec2(65, 55)); 
            
            ImGui::PopStyleColor(3); // ¡Súper importante sacar los 3 colores del stack o se rompe ImGui!

            // onMouseDown: Si el usuario aprieta este botón en este mismo instante...
            if (ImGui::IsItemActive()) {
                anyKeyHeld = true;
                if (currentUINote != note) {
                    currentUINote = note;
                    
                    // Si no estamos reproduciendo una pista automática, dejamos sonar el teclado libremente
                    if (sequencer.getMode() != SequencerMode::Playing) {
                        synth.noteOn(activeRecordTrack, Sequencer::getFrequencyFromNote(note)); // Disparamos la nota en el motor de audio
                        
                        // Si está habilitado el modo grabación, pegamos la nota al buffer
                        if (sequencer.getMode() == SequencerMode::Recording) {
                            sequencer.recordNote(activeRecordTrack, note);
                        }
                    }
                }
            }
        }

        // si sueltas el click
        if (!anyKeyHeld && !currentUINote.empty()) {
            currentUINote = "";
            if (sequencer.getMode() != SequencerMode::Playing) {
                synth.noteOff(activeRecordTrack); // Apagar pista activa
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text(">> MODULO SECUENCIADOR E/S");
        
        int bpm = sequencer.getBPM();
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::SliderInt("BPM", &bpm, 40, 300)) {
            sequencer.setBPM(bpm); // upddate del bpm al slider
        }

        ImGui::Spacing();
        
        if (ImGui::Button(" [+] AÑADIR PISTA ")) {
            sequencer.addTrack();
        }
        ImGui::Spacing();
        
        // CAJOTAS de texto pa meter la rola
        // Nota d clase: El flag _CharsUppercase nos salva d programar un toupper a mano wjujuuu
        ImGui::BeginChild("TracksRegion", ImVec2(0, ImGui::GetTextLineHeight() * 10), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (size_t i = 0; i < sequenceBuffers.size(); ++i) {
            
            // Selector de pista
            std::string radioId = "##rec" + std::to_string(i);
            if (ImGui::RadioButton(radioId.c_str(), activeRecordTrack == static_cast<int>(i))) {
                activeRecordTrack = static_cast<int>(i);
            }
            ImGui::SameLine();
            
            std::string playingNote = sequencer.getCurrentPlayingNote(i);
            std::string labelText = "TRK " + std::to_string(i + 1);
            
            if (sequencer.getMode() == SequencerMode::Playing && !playingNote.empty()) {
                labelText += " [" + playingNote + "]>";
                // Brilla si esta tocando con el color activo (se hace notar)
                ImGui::TextColored(g_colors.keyActive, "%s", labelText.c_str());
            } else {
                labelText += ">";
                ImGui::TextColored(g_colors.accent, "%s", labelText.c_str());
            }
            ImGui::SameLine();
            
            std::string label = "##sequence" + std::to_string(i);
            ImGui::InputTextMultiline(label.c_str(), sequenceBuffers[i].data(), sequenceBuffers[i].size(), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 3), ImGuiInputTextFlags_CharsUppercase);
            ImGui::Spacing();
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // Botones fisicos d colores feos 
        // Puesto a 0,40 d tamño pa que auto ajusten a lo q mida su texto adentero
        
        // = PLAY =
        if (sequencer.getMode() == SequencerMode::Playing) {
            ImGui::PushStyleColor(ImGuiCol_Button, g_colors.btnPlay);
            if (ImGui::Button("[X] STOP REPRODUCCION", ImVec2(0, 40))) sequencer.stop();
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, g_colors.btnPlay);
            if (ImGui::Button("[>] PLAY", ImVec2(0, 40))) {
                std::vector<std::string> seqs;
                for (size_t i = 0; i < sequenceBuffers.size(); ++i) {
                    seqs.push_back(std::string(sequenceBuffers[i].data()));
                }
                sequencer.play(seqs);
            }
            ImGui::PopStyleColor();
        }
        
        ImGui::SameLine();
        
        // = REC =
        ImGui::PushStyleColor(ImGuiCol_Button, g_colors.btnRecord);
        if (sequencer.getMode() == SequencerMode::Recording) {
            if (ImGui::Button("[O] STOP GRABACION", ImVec2(0, 40))) sequencer.stop();
        } else {
            if (ImGui::Button("[O] GRABAR", ImVec2(0, 40))) sequencer.record();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        
        // = CLEAR =
        ImGui::PushStyleColor(ImGuiCol_Button, g_colors.btnClear);
        if (ImGui::Button("[-] BORRAR MEMORIA", ImVec2(0, 40))) {
            for (size_t i = 0; i < sequenceBuffers.size(); ++i) {
                sequenceBuffers[i][0] = '\0';
                sequencer.setSequenceText(i, ""); // borra too
            }
        }
        ImGui::PopStyleColor();

        if (sequencer.getMode() == SequencerMode::Playing) {
            std::string states = "";
            for (size_t t = 0; t < sequencer.getTrackCount(); ++t) {
                std::string currentNote = sequencer.getCurrentPlayingNote(t);
                if (!currentNote.empty()) {
                    states += "T" + std::to_string(t+1) + ":[" + currentNote + "] ";
                }
            }
            ImGui::TextColored(g_colors.accent, "ESTADO: Reproduciendo %s", states.c_str());
        }

        ImGui::End();

        // Manda toodo a pantalla, dios vendiga opengl
        ImGui::Render();
        glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
        glClearColor(g_colors.bg.x, g_colors.bg.y, g_colors.bg.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // ==========================================================
    // Apagado Limpio y Seguro
    // ==========================================================
    synth.close();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0; // ¡Listo, acabamossss maquina!
}
