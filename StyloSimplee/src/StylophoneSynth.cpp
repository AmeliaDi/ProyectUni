#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "StylophoneSynth.h"
#include <cmath>
#include <iostream>

// la pi de toda la vida pero en C++20 p/ verte farol
#include <numbers>
constexpr float PI = std::numbers::pi_v<float>;

// Esta funcion me saco canas. Es el callback en puro C q ocupa la libreria.
// Nomas sirve pa castear la clase y mandar llamar nuestro processAudio
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    StylophoneSynth* synth = static_cast<StylophoneSynth*>(pDevice->pUserData);
    if (synth) {
        synth->processAudio(static_cast<float*>(pOutput), static_cast<int>(frameCount));
    }
}

StylophoneSynth::StylophoneSynth() 
    : m_device(nullptr),
      m_targetFrequency(440.0f), // empezamos en A4 porsiacaso
      m_gate(false),             
      m_phase(0.0f),
      m_currentFrequency(440.0f),
      m_envelope(0.0f),
      m_sampleRate(DEFAULT_SAMPLE_RATE),
      m_filterState(0.0f)
{
}

StylophoneSynth::~StylophoneSynth() {
    close(); // cerramos bn pa q no se queden zombies en la compu del profe
}

bool StylophoneSynth::init() {
    m_device = new ma_device; 
    
    // configuracion basica pal miniaudio
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;       // float 32 suena mas limpio
    config.playback.channels = 1;                   // mono como el aparato original
    config.sampleRate        = static_cast<ma_uint32>(DEFAULT_SAMPLE_RATE);
    config.dataCallback      = data_callback;       // conectamos la func de arriba
    config.pUserData         = this;                // pasamos nuestra clase (el this pointer we)

    if (ma_device_init(NULL, &config, m_device) != MA_SUCCESS) {
        std::cerr << "ya valio, no pudo arrancar miniaudio." << std::endl;
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

void StylophoneSynth::noteOn(float frequency) {
    // con relaxed basta, no ocupamos sincronizacion tan loca aca
    m_targetFrequency.store(frequency, std::memory_order_relaxed);
    m_gate.store(true, std::memory_order_relaxed);
}

void StylophoneSynth::noteOff() {
    m_gate.store(false, std::memory_order_relaxed);
}

void StylophoneSynth::processAudio(float* output, int frameCount) {
    // leer las variables nomas una vez al inicio del bloqe d audio pa no matar la cpu
    bool gate = m_gate.load(std::memory_order_relaxed);
    float targetFreq = m_targetFrequency.load(std::memory_order_relaxed);
    
    // tiempos super rapidos d ataque y release pa emular el stylus d metal pegando
    constexpr float ATTACK_MS = 10.0f;
    constexpr float RELEASE_MS = 50.0f;
    
    const float attackRate = 1.0f / ((ATTACK_MS / 1000.0f) * m_sampleRate);
    const float releaseRate = 1.0f / ((RELEASE_MS / 1000.0f) * m_sampleRate);

    // Filtro de paso bajo d un polo (me costo enteder esto en la clase d seales)
    // Corta agudos chiyones q lastiman el oido
    constexpr float FILTER_CUTOFF_HZ = 2500.0f; 
    const float rc = 1.0f / (2.0f * PI * FILTER_CUTOFF_HZ);
    const float dt = 1.0f / m_sampleRate;
    const float alpha = dt / (rc + dt); 

    for (int i = 0; i < frameCount; ++i) {
        
        // Glide suave entre notas (portamento se llama creo)
        m_currentFrequency += (targetFreq - m_currentFrequency) * 0.005f;
        
        if (gate) {
            m_envelope += attackRate;
            if (m_envelope > 1.0f) m_envelope = 1.0f; 
        } else {
            m_envelope -= releaseRate;
            if (m_envelope < 0.0f) m_envelope = 0.0f; 
        }

        float sample = 0.0f;

        // si no hay volumen ni calculamos la onda (optimizacion brutal)
        if (m_envelope > 0.0001f) {
            float phaseIncrement = m_currentFrequency / m_sampleRate;
            m_phase += phaseIncrement;
            if (m_phase >= 1.0f) m_phase -= 1.0f; 
            
            // Cuadrada y sierra mezcladas pa dar el tono zumbante
            float square = (m_phase < 0.5f) ? 1.0f : -1.0f;
            float saw = (m_phase * 2.0f) - 1.0f;
            
            // 70 cuad, 30 sierra. experimente un chingo pa encontrar este balance xd
            float rawWave = (square * 0.7f) + (saw * 0.3f);
            
            // Formula del filtro q robe del libro de dsp
            m_filterState = m_filterState + alpha * (rawWave - m_filterState);
            
            // aplicamos el dca al final
            sample = m_filterState * m_envelope * MASTER_VOLUME; 
        } else {
            // resetear fase pa q no haya pops cuando empiece de nuevo la nota
            m_phase = 0.0f;
        }

        output[i] = sample;
    }
}
