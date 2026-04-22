#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>

int main() {
    // 1. Crear la ventana gráfica
    sf::RenderWindow window(sf::VideoMode({1024, 768}), "Mi DAW Básico");
    window.setFramerateLimit(60); // 60 FPS

    // 2. Inicializar ImGui con SFML
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Error: No se pudo inicializar ImGui-SFML" << std::endl;
        return -1;
    }

    sf::Clock deltaClock;

    // Bucle Principal de la aplicación (Main Loop)
    while (window.isOpen()) {
        // Procesar todos los eventos de la ventana (teclado, ratón, cerrar ventana)
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // Actualizar el estado de ImGui para el siguiente frame
        ImGui::SFML::Update(window, deltaClock.restart());

        // --- INICIO DE LA INTERFAZ GRÁFICA (GUI) ---
        ImGui::Begin("Controles del DAW");
        
        ImGui::Text("Bienvenido a tu primer DAW!");
        ImGui::Spacing();

        if (ImGui::Button("Play", ImVec2(100, 30))) {
            std::cout << "[Audio Engine] Play presionado!" << std::endl;
            // Aquí en la Fase 2 llamaremos a miniaudio para reproducir
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Stop", ImVec2(100, 30))) {
            std::cout << "[Audio Engine] Stop presionado!" << std::endl;
        }

        ImGui::Spacing();
        
        // Un slider de ejemplo para el BPM
        static int projectBpm = 120;
        ImGui::SliderInt("BPM del Proyecto", &projectBpm, 60, 200);

        ImGui::End();
        // --- FIN DE LA INTERFAZ GRÁFICA ---

        // Limpiar la ventana con un color gris oscuro
        window.clear(sf::Color(30, 30, 30));
        
        // Dibujar ImGui en la ventana de SFML
        ImGui::SFML::Render(window);
        
        // Mostrar en pantalla todo lo que dibujamos
        window.display();
    }

    // 3. Cerrar y limpiar recursos
    ImGui::SFML::Shutdown();
    
    return 0;
}
