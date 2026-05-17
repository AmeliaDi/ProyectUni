#pragma once

#include <string>
#include <vector>
#include <functional>

/**
 * @brief Enum con los estados del secuenciador q hice
 */
enum class SequencerMode {
    Idle,       ///< ta en pausa
    Playing,    ///< tocando la cancion sola
    Recording   ///< escuchando los clicks p q se escriban solitos
};

/**
 * @brief El cerebro de la logica musical d las secuencias de texto.
 * 
 * Lee cosas tipo "A4 C5" y las vuelve musica con el tiempo de los BPM q le pasemos.
 */
class Sequencer {
public:
    Sequencer();
    
    // =========================================================================
    // Config Base
    // =========================================================================
    
    // Cambia los golpes por minuto de la cancion
    void setBPM(int bpm);
    
    // Retorna a q velocidad andamos
    int getBPM() const;
    
    // =========================================================================
    // Botones de accion
    // =========================================================================

    // Empieza a leer el string y lo toca
    void play(const std::string& sequenceText);
    
    // para todo
    void stop();
    
    // Graba lo q le des clik
    void record();
    
    // La funcion mas importante, se llama cada frame en el loop de sdl
    void update(float deltaTime);
    
    // Mete una nota al texto q llevamos grabado
    void recordNote(const std::string& noteName);
    
    // =========================================================================
    // Para q imgui se hable con esto
    // =========================================================================

    std::string getSequenceText() const;
    void setSequenceText(const std::string& text);
    
    // Punteros a funciones (callbacks) q comunican la logica con el sinte real de audio
    void setNoteCallbacks(std::function<void(float)> noteOnCb, std::function<void()> noteOffCb);

    SequencerMode getMode() const { return m_mode; }
    
    // La nota q ta sonando ahoritita
    std::string getCurrentPlayingNote() const { return m_currentPlayingNote; }

    // =========================================================================
    // Matemáticas feas
    // =========================================================================

    // Saca los Hz de un string "C#5"
    static float getFrequencyFromNote(const std::string& note);

private:
    SequencerMode m_mode;        
    int m_bpm;                   
    float m_stepDuration;        // duracion en segundos d cada cuartillo o lo q sea
    float m_timer;               // acumulador d tiempo (deltaTime)
    
    std::vector<std::string> m_sequence; // el array limpio d notas sin espacios raros
    int m_currentIndex;          // en dnd vamos en la rola
    
    std::string m_sequenceText;       // lo q metio el usuario crudo
    std::string m_currentPlayingNote; 

    std::function<void(float)> m_noteOnCallback; // el on
    std::function<void()> m_noteOffCallback;     // el off
    
    // Magia d std::stringstream para limpiar el texto
    void parseSequenceText(const std::string& text);
    
    // Toca la nota en la que vamos
    void triggerCurrentNote();
};
