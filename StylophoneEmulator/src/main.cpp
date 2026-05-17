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

// las notas q suenan, trate d meter mas d una octava pero ya no cabian 
const std::vector<std::string> STYLOPHONE_KEYS = {
    "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", 
    "A4", "A#4", "B4", "C5" 
};

// colores sacados de paletas viejas
const ImVec4 COL_BG          = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // gris q no lastima la vista
const ImVec4 COL_TEXT_AMBER  = ImVec4(1.00f, 0.78f, 0.17f, 1.00f); // ambar estilo matrix pero mas facha
const ImVec4 COL_ACCENT      = ImVec4(0.85f, 0.50f, 0.10f, 1.00f); // naranja pa q resalte

// Colores de las teclas metalicas
const ImVec4 COL_KEY_IDLE    = ImVec4(0.40f, 0.45f, 0.50f, 1.00f); 
const ImVec4 COL_KEY_HOVER   = ImVec4(0.50f, 0.55f, 0.60f, 1.00f); 
const ImVec4 COL_KEY_ACTIVE  = ImVec4(0.80f, 0.60f, 0.20f, 1.00f); 

// colores feos pa los botones fisicos del control
const ImVec4 COL_BTN_PLAY    = ImVec4(0.20f, 0.60f, 0.30f, 1.00f); 
const ImVec4 COL_BTN_RECORD  = ImVec4(0.70f, 0.20f, 0.20f, 1.00f); 
const ImVec4 COL_BTN_CLEAR   = ImVec4(0.60f, 0.50f, 0.10f, 1.00f); 

// Me copie un cacho del demo de imgui p cambiar tods los colores a la ves
void ApplyRetroStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.Colors[ImGuiCol_WindowBg]      = COL_BG;
    style.Colors[ImGuiCol_Text]          = COL_TEXT_AMBER;
    style.Colors[ImGuiCol_FrameBg]       = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]= ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = COL_ACCENT;
    style.Colors[ImGuiCol_Button]        = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]  = COL_ACCENT;
    style.Colors[ImGuiCol_SliderGrab]    = COL_ACCENT;
    style.Colors[ImGuiCol_SliderGrabActive] = COL_TEXT_AMBER;

    style.WindowRounding    = 0.0f;
    style.FrameRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.ItemSpacing       = ImVec2(12, 12); // Bastante separaditos q si no se enciman feo
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

    // Iniciar el ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ApplyRetroStyle();
    
    // cargar fuente retro chida q baje d interenet. 
    std::string fontPath = "assets/PressStart2P.ttf";
    if (std::filesystem::exists(fontPath)) {
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
    } else {
        std::cout << "[INFO] chale no enconctre la fuente en " << fontPath << "\n";
    }

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Inicializo mi super sinte
    StylophoneSynth synth;
    if (!synth.init()) {
        std::cerr << "valio gorro el motor d audio." << std::endl;
        return -1;
    }

    Sequencer sequencer;
    // Aki le paso las funciones lambda al secuenciador p q las ejecute desde su hilo
    sequencer.setNoteCallbacks(
        [&synth](float freq) { synth.noteOn(freq); },
        [&synth]() { synth.noteOff(); }
    );

    // el loop d toda la vdida
    bool done = false;
    Uint32 lastTime = SDL_GetTicks();
    char sequenceBuffer[2048] = ""; // un char gigante pq asi lo pide imgui ni pedo
    std::string currentUINote = "";

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
        if (sequencer.getMode() == SequencerMode::Recording) {
            strncpy(sequenceBuffer, sequencer.getSequenceText().c_str(), sizeof(sequenceBuffer) - 1);
        } else if (sequencer.getMode() == SequencerMode::Idle) {
            sequencer.setSequenceText(sequenceBuffer);
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
        ImGui::TextDisabled("=== Proyecto Progra v1 ===");
        ImGui::Spacing();
        
        ImGui::Text(">> CONTACTOS DE TECLADO (Usar Stylus/Raton)");
        bool anyKeyHeld = false;

        // Pinta el teclado chido con un for
        for (int i = 0; i < STYLOPHONE_KEYS.size(); ++i) {
            const auto& note = STYLOPHONE_KEYS[i];
            
            bool isPlayingFromSeq = (sequencer.getMode() == SequencerMode::Playing && sequencer.getCurrentPlayingNote() == note);
            
            if (isPlayingFromSeq) {
                ImGui::PushStyleColor(ImGuiCol_Button, COL_KEY_ACTIVE);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_KEY_ACTIVE);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,0,0,1)); // texto negro se lee mas bn
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, COL_KEY_IDLE);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_KEY_HOVER);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
            }

            if (i > 0 && i % 8 != 0) ImGui::SameLine(); // baja la linea al 8vo boton
            
            ImGui::Button(note.c_str(), ImVec2(45, 80)); 
            
            ImGui::PopStyleColor(3); // NO OLVIDAR ESTO SI NO SE RAYA LA INTERFAZ

            // El onMouseDown de imgui casi casi
            if (ImGui::IsItemActive()) {
                anyKeyHeld = true;
                if (currentUINote != note) {
                    currentUINote = note;
                    if (sequencer.getMode() != SequencerMode::Playing) {
                        synth.noteOn(Sequencer::getFrequencyFromNote(note));
                        if (sequencer.getMode() == SequencerMode::Recording) {
                            sequencer.recordNote(note);
                        }
                    }
                }
            }
        }

        // si sueltas el click
        if (!anyKeyHeld && !currentUINote.empty()) {
            currentUINote = "";
            if (sequencer.getMode() != SequencerMode::Playing) {
                synth.noteOff();
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
        
        // CAJOTA de texto pa meter la rola
        // Nota d clase: El flag _CharsUppercase nos salva d programar un toupper a mano wjujuuu
        ImGui::InputTextMultiline("##sequence", sequenceBuffer, IM_ARRAYSIZE(sequenceBuffer), ImVec2(0, ImGui::GetTextLineHeight() * 4), ImGuiInputTextFlags_CharsUppercase);

        ImGui::Spacing();

        // Botones fisicos d colores feos 
        // Puesto a 0,40 d tamño pa que auto ajusten a lo q mida su texto adentero
        
        // = PLAY =
        if (sequencer.getMode() == SequencerMode::Playing) {
            ImGui::PushStyleColor(ImGuiCol_Button, COL_BTN_PLAY);
            if (ImGui::Button("[X] STOP REPRODUCCION", ImVec2(0, 40))) sequencer.stop();
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, COL_BTN_PLAY);
            if (ImGui::Button("[>] PLAY", ImVec2(0, 40))) sequencer.play(sequenceBuffer);
            ImGui::PopStyleColor();
        }
        
        ImGui::SameLine();
        
        // = REC =
        ImGui::PushStyleColor(ImGuiCol_Button, COL_BTN_RECORD);
        if (sequencer.getMode() == SequencerMode::Recording) {
            if (ImGui::Button("[O] STOP GRABACION", ImVec2(0, 40))) sequencer.stop();
        } else {
            if (ImGui::Button("[O] GRABAR", ImVec2(0, 40))) sequencer.record();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        
        // = CLEAR =
        ImGui::PushStyleColor(ImGuiCol_Button, COL_BTN_CLEAR);
        if (ImGui::Button("[-] BORRAR MEMORIA", ImVec2(0, 40))) {
            sequenceBuffer[0] = '\0';
            sequencer.setSequenceText(""); // borra too
        }
        ImGui::PopStyleColor();

        if (sequencer.getMode() == SequencerMode::Playing) {
            ImGui::TextColored(COL_ACCENT, "ESTADO: Reproduciendo [%s]", sequencer.getCurrentPlayingNote().c_str());
        }

        ImGui::End();

        // Manda toodo a pantalla, dios vendiga opengl
        ImGui::Render();
        glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
        glClearColor(COL_BG.x, COL_BG.y, COL_BG.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Apagado chido
    synth.close();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0; // nos graduamos pa
}
