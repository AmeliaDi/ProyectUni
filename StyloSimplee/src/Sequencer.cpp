#include "Sequencer.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include <array>
#include <algorithm>

Sequencer::Sequencer() 
    : m_mode(SequencerMode::Idle),
      m_bpm(120), // 120 bpm, ritmo pop estándar
      m_stepDuration(60.0f / 120.0f),
      m_timer(0.0f)
{
    addTrack(); // Iniciamos con 1 pista por defecto
}

void Sequencer::setBPM(int bpm) {
    if (bpm <= 0) bpm = 1; // Evitamos divisiones por cero
    m_bpm = bpm;
    // La ecuación para obtener la duración de cada paso según el BPM (60 / beats)
    m_stepDuration = 60.0f / static_cast<float>(m_bpm);
}

int Sequencer::getBPM() const {
    return m_bpm;
}

void Sequencer::addTrack() {
    // Inicializamos con valores vacíos los vectores paralelos
    // Cada índice (0, 1, 2...) representa una pista (TRK 1, TRK 2...)
    m_sequenceTexts.push_back("");
    m_sequences.push_back(std::vector<std::string>());
    m_currentIndexes.push_back(0);
    m_currentPlayingNotes.push_back("");
}

size_t Sequencer::getTrackCount() const {
    return m_sequenceTexts.size();
}

void Sequencer::play(const std::vector<std::string>& sequenceTexts) {
    m_sequenceTexts = sequenceTexts;
    
    // Asegurarnos de tener suficientes estructuras de estado para cada texto
    while (m_sequences.size() < m_sequenceTexts.size()) {
        m_sequences.push_back(std::vector<std::string>());
        m_currentIndexes.push_back(0);
        m_currentPlayingNotes.push_back("");
    }
    
    bool hasNotes = false;
    for (size_t t = 0; t < m_sequenceTexts.size(); ++t) {
        m_sequences[t].clear();
        std::istringstream ss(m_sequenceTexts[t]);
        std::string token;
        while (ss >> token) {
            m_sequences[t].push_back(token);
            hasNotes = true;
        }
        m_currentIndexes[t] = 0;
        m_currentPlayingNotes[t] = "";
    }
    
    if (!hasNotes) {
        return; // Nada que reproducir
    }

    m_mode = SequencerMode::Playing;
    m_timer = 0.0f;
    triggerCurrentNotes();
}

void Sequencer::stop() {
    m_mode = SequencerMode::Idle;
    for (size_t t = 0; t < m_currentPlayingNotes.size(); ++t) {
        m_currentPlayingNotes[t] = "";
        if (m_noteOffCallback) {
            m_noteOffCallback(static_cast<int>(t));
        }
    }
}

void Sequencer::record() {
    m_mode = SequencerMode::Recording;
    // Cambiamos de estado sin borrar la secuencia actual
}

void Sequencer::update(float deltaTime) {
    if (m_mode != SequencerMode::Playing) return;

    // Sumamos el tiempo que tardó el frame
    m_timer += deltaTime;

    // Si ya superamos el tiempo de espera (el beat o compás)...
    if (m_timer >= m_stepDuration) {
        m_timer -= m_stepDuration;
        
        // Avanzamos el "cabezal de reproducción" de todas las pistas simultáneamente
        bool allFinished = true;
        for (size_t t = 0; t < m_sequences.size(); ++t) {
            m_currentIndexes[t]++;
            if (m_currentIndexes[t] < m_sequences[t].size()) {
                allFinished = false; // Todavía hay notas en esta pista
            }
        }
        
        // Si todas las pistas llegaron al final, apagamos la reproducción
        if (allFinished) {
            stop();
            return;
        }
        
        triggerCurrentNotes();
    }
}

void Sequencer::recordNote(size_t track, const std::string& noteName) {
    if (m_mode != SequencerMode::Recording) return;
    
    while (m_sequenceTexts.size() <= track) addTrack();
    
    // Grabamos en la pista seleccionada
    if (!m_sequenceTexts[track].empty() && m_sequenceTexts[track].back() != ' ' && m_sequenceTexts[track].back() != '\n') {
        m_sequenceTexts[track] += " ";
    }
    m_sequenceTexts[track] += noteName;
}

std::string Sequencer::getSequenceText(size_t track) const {
    if (track < m_sequenceTexts.size()) return m_sequenceTexts[track];
    return "";
}

void Sequencer::setSequenceText(size_t track, const std::string& text) {
    if (track < m_sequenceTexts.size()) {
        m_sequenceTexts[track] = text;
    }
}

void Sequencer::setNoteCallbacks(std::function<void(int, float)> noteOnCb, std::function<void(int)> noteOffCb) {
    m_noteOnCallback = noteOnCb;
    m_noteOffCallback = noteOffCb;
}

void Sequencer::triggerCurrentNotes() {
    // Iteramos por todas las pistas a la vez
    for (size_t t = 0; t < m_sequences.size(); ++t) {
        if (m_currentIndexes[t] < m_sequences[t].size()) {
            std::string note = m_sequences[t][m_currentIndexes[t]];
            
            float freq = getFrequencyFromNote(note);
            if (freq > 0.0f) {
                m_currentPlayingNotes[t] = note;
                if (m_noteOnCallback) {
                    m_noteOnCallback(static_cast<int>(t), freq); // Activamos la voz en el sinte
                }
            } else {
                // Silencio detectado
                m_currentPlayingNotes[t] = "";
                if (m_noteOffCallback) {
                    m_noteOffCallback(static_cast<int>(t));
                }
            }
        } else {
            // Aseguramos que no queden notas "zombies" sonando
            m_currentPlayingNotes[t] = "";
            if (m_noteOffCallback) {
                m_noteOffCallback(static_cast<int>(t));
            }
        }
    }
}

std::string Sequencer::getCurrentPlayingNote(size_t track) const {
    if (track < m_currentPlayingNotes.size()) return m_currentPlayingNotes[track];
    return "";
}

float Sequencer::getFrequencyFromNote(const std::string& note) {
    // Si el usuario escribe "00" o espacios vacíos, es un Silencio Musical (Rest).
    // Devolvemos 0.0f para que el sintetizador calle a esta voz.
    if (note == "00" || note.empty() || note == "-" || note == "rest" || note == "R") return 0.0f;
    
    struct NoteOffset {
        const char* name;
        int offset;
    };

    // Diccionario estático: Mapea el nombre de la nota a su valor relativo
    static constexpr std::array<NoteOffset, 12> noteOffsets = {{
        {"C", -9}, {"C#", -8}, {"D", -7}, {"D#", -6}, 
        {"E", -5}, {"F", -4},  {"F#", -3}, {"G", -2}, 
        {"G#", -1}, {"A", 0},  {"A#", 1},  {"B", 2}
    }};

    if (note.empty() || note == "-" || note == "rest" || note == "R" || note == "00") return 0.0f;

    std::string noteName = "";
    int octave = 4; // la 4ta octava es la standar bro

    size_t i = 0;
    while (i < note.length() && !std::isdigit(note[i])) {
        // a mayusculas x si el profe pone "a4" envez d "A4" en las pruebas
        noteName += static_cast<char>(std::toupper(note[i]));
        i++;
    }
    
    if (i < note.length()) {
        try {
            octave = std::stoi(note.substr(i));
        } catch (const std::exception&) {
            // si pone letras en vez de numero q no crashee
            octave = 4;
        }
    }

    auto it = std::find_if(noteOffsets.begin(), noteOffsets.end(), 
        [&noteName](const NoteOffset& n) { return n.name == noteName; });

    if (it != noteOffsets.end()) {
        // formula para sacar el pitch real de la nota segun la raiz dozeava de 2 q vi en clase
        int halfStepsFromA4 = it->offset + (octave - 4) * 12;
        return 440.0f * std::pow(2.0f, static_cast<float>(halfStepsFromA4) / 12.0f);
    }

    return 0.0f; 
}
