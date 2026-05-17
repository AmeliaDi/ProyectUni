#include "Sequencer.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include <array>
#include <algorithm>

Sequencer::Sequencer() 
    : m_mode(SequencerMode::Idle),
      m_bpm(120), // 120 bpm es el defolt en casi todos lados jeje
      m_stepDuration(60.0f / 120.0f),
      m_timer(0.0f),
      m_currentIndex(0)
{
}

void Sequencer::setBPM(int bpm) {
    if (bpm <= 0) bpm = 1; // no qeremos q divida x cero y crashee todo
    m_bpm = bpm;
    // la ecuacion para sacar segundos en base al bpm. (1 minuto / los beats)
    m_stepDuration = 60.0f / static_cast<float>(m_bpm);
}

int Sequencer::getBPM() const {
    return m_bpm;
}

void Sequencer::play(const std::string& sequenceText) {
    m_sequenceText = sequenceText;
    parseSequenceText(m_sequenceText);
    
    if (m_sequence.empty()) {
        return; // we, no hay nada q tocar
    }

    m_mode = SequencerMode::Playing;
    m_currentIndex = 0;
    m_timer = 0.0f;
    triggerCurrentNote();
}

void Sequencer::stop() {
    m_mode = SequencerMode::Idle;
    m_currentPlayingNote = "";
    if (m_noteOffCallback) {
        m_noteOffCallback();
    }
}

void Sequencer::record() {
    m_mode = SequencerMode::Recording;
    // Nomas cambiamos d estado pq no qremos q se borre lo q ya staba escrito 
}

void Sequencer::update(float deltaTime) {
    if (m_mode != SequencerMode::Playing) {
        return; // si no ta en play, pa que hacemos mate
    }

    m_timer += deltaTime;
    
    // truquito del staccato: suelta la nota poquito antes d q acabe el compas
    // pa q si vienen 2 notas iguales se escuche el corte y no un bip laaaargo infinito
    if (m_timer >= m_stepDuration * 0.9f && !m_currentPlayingNote.empty()) {
        if (m_noteOffCallback) {
            m_noteOffCallback();
        }
        m_currentPlayingNote = "";
    }

    // Ya nos pasamos del tiempo del compas, hay q tocar la q sigue
    if (m_timer >= m_stepDuration) {
        m_timer -= m_stepDuration;
        m_currentIndex++;
        
        if (m_currentIndex >= m_sequence.size()) {
            stop(); // ya se acabo la rola
            return;
        }
        
        triggerCurrentNote();
    }
}

void Sequencer::recordNote(const std::string& noteName) {
    if (m_mode != SequencerMode::Recording) return;
    
    // Le mete un espacion si no habia para q quede bonito en la cajita d texto
    if (!m_sequenceText.empty() && m_sequenceText.back() != ' ' && m_sequenceText.back() != '\n') {
        m_sequenceText += " ";
    }
    m_sequenceText += noteName;
}

std::string Sequencer::getSequenceText() const {
    return m_sequenceText;
}

void Sequencer::setSequenceText(const std::string& text) {
    m_sequenceText = text;
}

void Sequencer::setNoteCallbacks(std::function<void(float)> noteOnCb, std::function<void()> noteOffCb) {
    m_noteOnCallback = noteOnCb;
    m_noteOffCallback = noteOffCb;
}

void Sequencer::parseSequenceText(const std::string& text) {
    m_sequence.clear();
    
    // El stringstream es la onda. Tu le pasas el texto puerco con enter, tab y un buen
    // d espacios juntos y el solito te lo devuelve limpiecito token por token 
    std::istringstream ss(text);
    std::string token;
    while (ss >> token) {
        m_sequence.push_back(token);
    }
}

void Sequencer::triggerCurrentNote() {
    std::string note = m_sequence[m_currentIndex];
    
    float freq = getFrequencyFromNote(note);
    if (freq > 0.0f) {
        m_currentPlayingNote = note;
        if (m_noteOnCallback) {
            m_noteOnCallback(freq); // Manda la func asincrona
        }
    } else {
        // Silencio !
        m_currentPlayingNote = "";
        if (m_noteOffCallback) {
            m_noteOffCallback();
        }
    }
}

float Sequencer::getFrequencyFromNote(const std::string& note) {
    // Array pa buscar cuantos semitonos nos movemos a partir de A (La)
    // Cuesta menos memoria hacerlo asi q andar con puros if-else larguisimos
    struct NoteOffset {
        const char* name;
        int offset;
    };
    
    static constexpr std::array<NoteOffset, 12> noteOffsets = {{
        {"C", -9}, {"C#", -8}, {"D", -7}, {"D#", -6}, 
        {"E", -5}, {"F", -4},  {"F#", -3}, {"G", -2}, 
        {"G#", -1}, {"A", 0},  {"A#", 1},  {"B", 2}
    }};

    // Cosas pa hacer silencio
    if (note.empty() || note == "-" || note == "rest" || note == "R") return 0.0f;

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
