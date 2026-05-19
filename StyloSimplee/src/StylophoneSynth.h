#pragma once

#include <atomic>
#include <array>
// declaraciones pa que el compilador no llore y compile mas rapido 
// (tip del profe para no incluir todo miniaudio aki)
struct ma_device;
struct ma_context;

/**
 * @brief Clase central del sintetizador (StylophoneSynth).
 * 
 * ¡Aquí es donde ocurre la magia! Todo el sonido es generado con matemáticas puras 
 * en tiempo real, cero archivos WAV o MP3 porque ocupan mucho espacio.
 * Mezclamos una onda cuadrada con una de sierra para lograr ese tono zumbante 
 * tan característico del hardware retro original.
 * 
 * @note **Arquitectura Hilo-Segura (Thread Safety):**
 *       El motor de audio corre en su propio hilo de altísima prioridad. Si usamos 
 *       un `mutex` tradicional, la interfaz gráfica podría trabarse o causaríamos 
 *       clicks feos en el audio. Por eso, usamos variables atómicas (`std::atomic`) 
 *       con `memory_order_relaxed`, garantizando una comunicación rapidísima 
 *       y segura entre la UI y la tarjeta de sonido.
 */
class StylophoneSynth {
public:
    // Constructor: Prepara el terreno y asigna valores por defecto.
    StylophoneSynth();
    
    // Destructor: ¡Súper importante! Limpia la memoria y apaga el hardware de audio 
    // para no dejar procesos "zombies" consumiendo RAM.
    ~StylophoneSynth();

    // Arranca el motor de miniaudio y abre el canal con la tarjeta de sonido.
    bool init();

    // Cierra el flujo de audio de forma limpia y segura.
    void close();

    // Ordena encender una nota en una pista específica. 
    // 'frequency' es el tono en Hercios (Hz) a reproducir.
    void noteOn(int track, float frequency);

    // Ordena apagar la nota actual de la pista dada (inicia el Release).
    void noteOff(int track);

    /**
     * @brief El Callback de Audio (El corazón del programa)
     * 
     * Esta función es llamada por miniaudio cientos de veces por segundo.
     * Aquí calculamos matemáticamente cómo se dibuja la onda de sonido.
     * REGLA DE ORO: No usar `std::cout`, ni `new`, ni interactuar con ImGui aquí. 
     * Este hilo debe ser ultrarrápido o el audio tartamudeará.
     */
    void processAudio(float* output, int frameCount);

    // Constantes globales (`constexpr` se evalúa al compilar, súper optimizado)
    static constexpr float DEFAULT_SAMPLE_RATE = 44100.0f; // Calidad de CD estándar
    static constexpr float MASTER_VOLUME = 0.2f;           // Para no reventar los tímpanos

    static constexpr int MAX_TRACKS = 16; // Límite de pistas polifónicas simultáneas

private:
    ma_device* m_device; // el aparatito de audio de miniaudio
    
    // Estructura que guarda el estado de cada pista individualmente (como un mini-sintetizador)
    struct SynthVoice {
        std::atomic<float> targetFrequency{440.0f}; // La nota a la que queremos llegar (en Hz)
        std::atomic<bool> gate{false};              // ¿Está la tecla presionada? (true=suena, false=se apaga)
        float phase = 0.0f;                         // En qué parte del ciclo de la onda vamos (0.0 a 1.0)
        float currentFrequency = 440.0f;            // Frecuencia real actual (hace un 'slide' hacia targetFrequency)
        float envelope = 0.0f;                      // El nivel de volumen actual de esta voz (ADSR simplificado)
        float filterState = 0.0f;                   // Memoria matemática del filtro paso bajo para esta voz
    };

    std::array<SynthVoice, MAX_TRACKS> m_voices;
    float m_sampleRate;         
};
