#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "StylophoneSynth.h"
#include <cmath>
#include <iostream>

// La constante Pi de toda la vida, pero extraída nativamente desde C++20. ¡Estilo moderno y preciso!
#include <numbers>
constexpr float PI = std::numbers::pi_v<float>;

// Este es el puente entre el mundo primitivo de C (miniaudio) y nuestra clase orientada a objetos en C++.
// Su única chamba es castear el puntero (userData) de vuelta a nuestra clase y llamar a `processAudio`.
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    StylophoneSynth* synth = static_cast<StylophoneSynth*>(pDevice->pUserData);
    if (synth) {
        synth->processAudio(static_cast<float*>(pOutput), static_cast<int>(frameCount));
    }
}

StylophoneSynth::StylophoneSynth() 
    : m_device(nullptr),
      m_sampleRate(DEFAULT_SAMPLE_RATE)
{
}

StylophoneSynth::~StylophoneSynth() {
    close(); // cerramos bn pa q no se queden zombies en la compu del profe
}

bool StylophoneSynth::init() {
    m_device = new ma_device; 
    
    // Configuración básica para miniaudio (la librería que nos salva la vida).
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;       // Usamos Float 32 bits porque suena súper nítido y evita recortes raros.
    config.playback.channels = 1;                   // Mono (1 canal), tal como el aparato original de los 60s.
    config.sampleRate        = static_cast<ma_uint32>(DEFAULT_SAMPLE_RATE); // 44.1kHz estándar.
    config.dataCallback      = data_callback;       // Conectamos nuestra función puente de arriba.
    config.pUserData         = this;                // Pasamos nuestra propia clase para poder acceder a ella desde el hilo de audio.

    if (ma_device_init(NULL, &config, m_device) != MA_SUCCESS) {
        std::cerr << "Fallo crítico: No se pudo arrancar la tarjeta de sonido a través de miniaudio." << std::endl;
        delete m_device;
        m_device = nullptr;
        return false;
    }

    m_sampleRate = static_cast<float>(m_device->sampleRate);

    if (ma_device_start(m_device) != MA_SUCCESS) {
        std::cerr << "error al arrancar el stream :(" << std::endl;
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
        return false;
    }

    return true;
}

void StylophoneSynth::close() {
    if (m_device) {
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
    }
}

void StylophoneSynth::noteOn(int track, float frequency) {
    if (track >= 0 && track < MAX_TRACKS) {
        // con relaxed basta, no ocupamos sincronizacion tan loca aca
        m_voices[track].targetFrequency.store(frequency, std::memory_order_relaxed);
        m_voices[track].gate.store(true, std::memory_order_relaxed);
    }
}

void StylophoneSynth::noteOff(int track) {
    if (track >= 0 && track < MAX_TRACKS) {
        m_voices[track].gate.store(false, std::memory_order_relaxed);
    }
}

void StylophoneSynth::processAudio(float* output, int frameCount) {
    // Tiempos súper rápidos de Attack (ataque) y Release (relajación).
    // Esto simula el golpe físico metálico del stylus contra la pista del teclado real.
    constexpr float ATTACK_MS = 10.0f;
    constexpr float RELEASE_MS = 50.0f;
    
    const float attackRate = 1.0f / ((ATTACK_MS / 1000.0f) * m_sampleRate);
    const float releaseRate = 1.0f / ((RELEASE_MS / 1000.0f) * m_sampleRate);

    // Filtro paso bajo de un polo (súper clásico en Procesamiento de Señales Digitales o DSP).
    // Nos ayuda a cortar esos agudos súper chillones y robóticos que lastiman el oído.
    constexpr float FILTER_CUTOFF_HZ = 2500.0f; 
    const float rc = 1.0f / (2.0f * PI * FILTER_CUTOFF_HZ);
    const float dt = 1.0f / m_sampleRate;
    const float alpha = dt / (rc + dt); 

    for (int i = 0; i < frameCount; ++i) {
        float mixedSample = 0.0f;

        // Loop por todas las pistas (Mezclador / Mixer)
        for (int v = 0; v < MAX_TRACKS; ++v) {
            auto& voice = m_voices[v];
            
            bool gate = voice.gate.load(std::memory_order_relaxed);
            float targetFreq = voice.targetFrequency.load(std::memory_order_relaxed);
            
            // Glide suave entre notas (portamento se llama creo)
            voice.currentFrequency += (targetFreq - voice.currentFrequency) * 0.005f;
            
            if (gate) {
                voice.envelope += attackRate;
                if (voice.envelope > 1.0f) voice.envelope = 1.0f; 
            } else {
                voice.envelope -= releaseRate;
                if (voice.envelope < 0.0f) voice.envelope = 0.0f; 
            }

            // Si el volumen bajó a casi cero, apagamos la onda matemática para no consumir procesador en vano (optimización bestial).
            if (voice.envelope > 0.0001f) {
                float phaseIncrement = voice.currentFrequency / m_sampleRate;
                voice.phase += phaseIncrement;
                if (voice.phase >= 1.0f) voice.phase -= 1.0f; 
                
                // Generamos nuestras dos formas de onda puras: Cuadrada y Diente de Sierra (Sawtooth).
                float square = (voice.phase < 0.5f) ? 1.0f : -1.0f;
                float saw = (voice.phase * 2.0f) - 1.0f;
                
                // La receta secreta: 70% cuadrada + 30% sierra. Esto le da ese tono zumbante inconfundible.
                float rawWave = (square * 0.7f) + (saw * 0.3f);
                
                // Aplicamos la fórmula mágica del filtro suavizador.
                voice.filterState = voice.filterState + alpha * (rawWave - voice.filterState);
                
                // Multiplicamos por la envolvente de volumen y lo sumamos a la pista maestra.
                mixedSample += voice.filterState * voice.envelope; 
            } else {
                // Reiniciar la fase (el punto de partida de la onda) para que no haya ruidos pop ("clicks") en la siguiente nota.
                voice.phase = 0.0f;
            }
        }

        // Soft Clipping (Tangente Hiperbólica): Es un truco de audio profesional.
        // Evita el clipeo digital (saturación fea) si tocamos muchas pistas a la vez comprimiendo los picos suavemente de forma analógica.
        output[i] = std::tanh(mixedSample) * MASTER_VOLUME;
    }
}
