#pragma once

#include <atomic>

// declaraciones pa que el compilador no llore y compile mas rapido 
// (tip del profe para no incluir todo miniaudio aki)
struct ma_device;
struct ma_context;

/**
 * @brief Clase del sinte del stylophone.
 * 
 * Todo el sonido es generado con mates, nada de archivos wav pq luego pesa un buen.
 * Trate de mezclar una cuadrada con sierra para que suene chido y zumbante como el original.
 * 
 * @note **OJO con los hilos:**
 *       Como el audio va en su propio rollo (hilo de alta prioridad), si usamos mutex normal
 *       se nos traba la interfaz o hace ruidos feos (clicks). Asi q mejor use variables atómicas
 *       (`std::atomic`) con `memory_order_relaxed` pq nomas necesitamos el ultimo valor de la nota.
 */
class StylophoneSynth {
public:
    // constructor normalito
    StylophoneSynth();
    
    // destructor, hay q acordarse de cerrar el device o se queda trabado el pulseaudio
    ~StylophoneSynth();

    // Arranca el miniaudio
    bool init();

    // Apaga todo
    void close();

    // La UI llama esto pa que suene una nota (frecuencia en Hz)
    void noteOn(float frequency);

    // suelta la nota actual
    void noteOff();

    /**
     * @brief El callback del audio!!
     * 
     * Rellena el buffer con las matematicas.
     * CUIDAO: esto corre en el hilo del miniaudio. NO PONER cout, ni news ni interactuar con la ui aki
     * o crashea horrible todo el programa (ya me paso wey).
     */
    void processAudio(float* output, int frameCount);

    // constantes globales (constexpr para que se vea pro)
    static constexpr float DEFAULT_SAMPLE_RATE = 44100.0f;
    static constexpr float MASTER_VOLUME = 0.2f;

private:
    ma_device* m_device; // el aparatito de audio de miniaudio
    
    // Variables atomicas pa comunicarnos entre hilos sin romper nada
    std::atomic<float> m_targetFrequency; // q nota queremos q suene
    std::atomic<bool> m_gate;             // si esta presionado o no

    // Variables internas del sinte (SOLO TOCAR EN EL HILO DE AUDIO PLS)
    float m_phase;              // en q parte de la onda andamos
    float m_currentFrequency;   // frec actual (hace un slide leve pa q suene mas natural)
    float m_envelope;           // volumen actual (ataque y release)
    float m_sampleRate;         
    
    float m_filterState;        // variable p/ la ecuacion diferencial del filtro
};
