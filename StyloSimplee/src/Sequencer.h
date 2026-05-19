#pragma once

#include <string>
#include <vector>
#include <functional>

/**
 * @brief El Cerebro Rítmico (Sequencer)
 * 
 * Este módulo es como nuestro director de orquesta. Se encarga de leer 
 * todo el texto sucio que el usuario mete en las cajitas (ej. "A4 00 C5"), 
 * extraer cuáles notas son reales, calcular el "timing" preciso usando los BPM, 
 * y mandar la señal asíncrona al sintetizador para que las reproduzca.
 * Además maneja un sistema multipista paralelo como un DAW real!
 */
enum class SequencerMode {
    Idle,      ///< El sistema está en reposo total
    Playing,   ///< Las pistas están sonando automáticamente
    Recording  ///< Todo clic en el piano virtual se registrará
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

    // Empieza a leer los strings y los toca todos a la vez
    void play(const std::vector<std::string>& sequenceTexts);
    
    // para todo
    void stop();
    
    // Graba lo q le des clik
    void record();
    
    // La funcion mas importante, se llama cada frame en el loop de sdl
    void update(float deltaTime);
    
    // Mete una nota al texto q llevamos grabado en la pista dada
    void recordNote(size_t track, const std::string& noteName);
    
    // =========================================================================
    // Para q imgui se hable con esto
    // =========================================================================

    std::string getSequenceText(size_t track) const;
    void setSequenceText(size_t track, const std::string& text);
    
    // Pistas dinámicas
    void addTrack();
    size_t getTrackCount() const;
    
    // Punteros a funciones (callbacks) q comunican la logica con el sinte real de audio
    void setNoteCallbacks(std::function<void(int, float)> noteOnCb, std::function<void(int)> noteOffCb);

    SequencerMode getMode() const { return m_mode; }
    
    // La nota q ta sonando ahoritita en una pista
    std::string getCurrentPlayingNote(size_t track) const;

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
    
    std::vector<std::vector<std::string>> m_sequences; // el array de arrays d notas
    std::vector<int> m_currentIndexes;                 // en dnd vamos en cada rola
    
    std::vector<std::string> m_sequenceTexts;          // lo q metio el usuario
    std::vector<std::string> m_currentPlayingNotes;    // q suena en cada pista

    std::function<void(int, float)> m_noteOnCallback;  // el on (track, freq)
    std::function<void(int)> m_noteOffCallback;        // el off (track)
    
    // Toca la nota en la que vamos para todas las pistas
    void triggerCurrentNotes();
};
