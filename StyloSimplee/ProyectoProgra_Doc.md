# Documentación Técnica: Emulador de Stylophone (StyloSimle)

## Descripción General
El proyecto "StyloSimle" es un emulador de sintetizador por software basado en el clásico instrumento electrónico Stylophone. Fue desarrollado íntegramente en C++20 con un enfoque arquitectónico orientado al rendimiento y la latencia ultra baja, separando estrictamente la interfaz gráfica de usuario (GUI) del procesamiento de señales digitales (DSP).

---

## Stack Tecnológico
- **Lenguaje:** C++20
- **Sistema de Compilación:** CMake
- **Capa Gráfica y Contexto:** SDL2 + OpenGL3
- **Interfaz Gráfica de Usuario (GUI):** Dear ImGui
- **Motor de Audio:** Miniaudio (Implementación Single-header, procesamiento asíncrono)

---

## Buenas Prácticas Aplicadas en C++
Este proyecto se rige por convenciones modernas de programación orientada a objetos en C++:
1. **Espacio de nombres estándar (`std::`)**: En C++, `std` (Standard) es la biblioteca principal que contiene funcionalidades fundamentales como arreglos dinámicos (`std::vector`) o cadenas de texto (`std::string`). Se utiliza `std::` explícitamente en lugar del atajo `using namespace std;` para evitar "colisiones de nombres", lo cual es una regla de oro en el desarrollo profesional para evitar que variables con el mismo nombre interfieran entre sí.
2. **Uso de `constexpr`**: Variables que nunca van a cambiar y cuyos valores se conocen antes de compilar el código (como la frecuencia de muestreo de audio `DEFAULT_SAMPLE_RATE`) utilizan la palabra reservada `constexpr`. Esto optimiza enormemente el programa porque el compilador hace las sumas o cálculos una sola vez al generar el ejecutable, en lugar de gastar memoria de procesamiento calculándolo cada vez que el programa corre.
3. **Punteros Inteligentes y RAII**: Los recursos del sistema operativo (como pedir acceso a la tarjeta de sonido) se solicitan en la inicialización (el "constructor" de las clases) y se liberan obligatoria y automáticamente al cerrar el programa (en el "destructor" `close()`), previniendo las temidas fugas de memoria (Memory Leaks).
4. **Hilos Seguros (Thread-Safety) sin Mutex**: El motor de audio funciona en "segundo plano" a velocidades extremas. Si lo bloqueáramos usando semáforos comunes (`mutex`), el sonido sufriría pausas o ruidos molestos. Por ello, la comunicación entre la interfaz de usuario y el audio se realiza declarando variables como `std::atomic`. Las variables atómicas le aseguran al procesador que la escritura y lectura se harán a nivel de hardware, sin riesgo de que los hilos choquen entre sí.

---

## Arquitectura del Sistema: ¿Qué hace cada archivo?

El proyecto se divide en tres piezas clave:

### 1. Motor de Audio (`StylophoneSynth.h` y `.cpp`)
Es la fábrica de sonido. Es responsable de calcular, número por número, la onda sonora en tiempo real. Ahora cuenta con un **Motor Polifónico Multipista**, capaz de reproducir hasta 16 voces o "pistas" a la vez. No utiliza archivos MP3 ni WAV.
- **Variables Clave:** 
  - `struct SynthVoice`: Una estructura compacta que funciona como un "mini-sintetizador" individual. Guarda la frecuencia de una pista (`m_targetFrequency`), su fase y envolvente (volumen).
  - `std::array<SynthVoice, 16> m_voices`: El arreglo que almacena el estado de las 16 pistas. Permite evitar el uso de alocación de memoria dinámica (como `std::vector` o `new`), lo cual es una regla sagrada para hilos de audio ultra-rápidos.
- **Métodos Principales:**
  - `processAudio(float* output, int frameCount)`: Es el corazón de todo el aplicativo. Se llama cientos de veces por segundo en segundo plano.
  
  **¿Cómo se mezcla el sonido en un Multipista?**
  Para que escuches múltiples tonos a la vez, el método `processAudio` utiliza un "Mezclador por Software" (Soft Mixer) en estos sencillos pasos:
  1. **Oscilación de Pistas:** El programa itera sobre el vector de las 16 voces, calculando de forma independiente el avance (`phase`) y la forma de onda (70% cuadrada y 30% sierra) para cada una de ellas, aplicando sus propios filtros y volúmenes.
  2. **Sumatoria Master:** Las ondas procesadas de cada pista se suman aritméticamente a una variable central llamada `mixedSample`.
  3. **Limitador de Picos (Soft Clipping):** Si sumáramos 10 sonidos muy fuertes, la señal superaría el límite digital de `1.0` y `-1.0`, provocando un ruido horrible conocido como "clipping" o saturación digital. Para evitarlo de manera profesional, pasamos el resultado final por una función trigonométrica `std::tanh(mixedSample)`. La tangente hiperbólica comprime de manera natural y suave los picos extremos del audio, manteniendo un volumen equilibrado sin importar cuántas notas estén sonando.

### 2. Motor de Secuenciación (`Sequencer.h` y `.cpp`)
Es el encargado de leer múltiples textos, entenderlos y automatizar la música en base al tiempo.
- **Variables Clave:**
  - `std::vector<std::vector<std::string>> m_sequences`: Un vector dinámico "bidimensional" (lista de listas). Cada lista representa una pista (TRK 1, TRK 2, etc.), y almacena el orden de sus notas a corto plazo.
  - `m_timer` y `m_stepDuration`: Acumuladores de tiempo. Leen los "BPM" (Beats/Golpes por minuto) definidos por el usuario en el control deslizante y calculan matemáticamente cuántos milisegundos de espera deben transcurrir entre un compás y el siguiente.
- **Métodos Principales:**
  - `play(const std::vector<std::string>& sequenceTexts)`: Utiliza `std::istringstream` (un flujo o túnel de lectura de texto nativo de C++). Su trabajo es tragar el arreglo de los textos sucios escritos por el usuario (por ejemplo: "A4     C5\n\n 00"), ignorar todos los espacios, y guardarlos de forma limpia y paralela en cada uno de los vectores internos.
  - `getFrequencyFromNote()`: Es un traductor matemático. Convierte una cadena de texto (como "A4") a su frecuencia sonora pura (440.0f) iterando sobre un catálogo base y aplicando la fórmula de potencias de la escala musical occidental. Esta función también reconoce el token mágico `"00"`, retornando un perfecto `0.0f` que los otros métodos interpretan y ejecutan como un **Silencio** (Rest) musical.

### 3. Capa de Presentación (`main.cpp`)
El archivo de control central que coordina el dibujo de los gráficos y une la interfaz con la lógica de los motores de audio y secuencias.
- **Lógica de Renderizado:** El bloque `while (!done)` conforma el bucle de renderizado infinito del juego/app. Se encarga de limpiar el lienzo y re-dibujar los botones interactivos a un mínimo de 60 fotogramas por segundo, ajustándose milimétricamente al ancho de la pantalla sin espacios inútiles. 
- **Conexión de Componentes:** En este archivo nacen las variables `StylophoneSynth synth;` y `Sequencer sequencer;`. Para conectar sus cerebros sin mezclarlos, pasamos "funciones anónimas (lambdas)" que le instruyen al secuenciador: "Cuando el tiempo dictamine que debes tocar algo, llama internamente a la función `synth.noteOn(frecuencia)`".

---

## Explicación Funcional de la Interfaz Interactiva

- **Selector de Temas**: Incorporamos un menú desplegable (`ImGui::Combo`) en la esquina superior derecha del encabezado (junto al nuevo título `Proyecto Proga StyloSimple V1.3`). Este incluye esquemas de colores retro programados con paletas `ImVec4`, permitiéndote cambiar al instante entre: *Ámbar Retro (Fer)*, *Matrix Verde (enora)*, *Blanco y Negro (enora)*, y *Monokai Hacker (Fer)*.
- **Botón `[+] AÑADIR PISTA`**: Escala la lógica del secuenciador creando en tiempo real nuevas cajas de texto (`InputTextMultiline`) auto-ajustables en un contenedor provisto de barras de desplazamiento vertical (Scrollbar) para no romper el aspecto de la ventana principal.
- **Botón [>] PLAY:** Invoca a `sequencer.play()`. Analiza todos los textos de pistas disponibles en pantalla, vacía la memoria anterior y arranca el reloj interno global.
- **Grabación Dirigida (Radio Buttons):** Añadimos pequeños selectores (círculos) junto a cada caja de texto. Si seleccionas la Pista 2 y presionas grabar, todo lo que toques en el teclado virtual se registrará directa y exclusivamente en esa pista. ¡Súper útil para hacer armonías complejas sin borrar tu melodía principal!
- **Feedback Visual Dinámico:** Al reproducir la música, los títulos de las pistas cambian en tiempo real (ej. pasando de `TRK 1>` a `TRK 1 [C#5]>`) y brillan usando el color de acento principal. Esto te permite "ver" la música y saber exactamente qué nota está escupiendo cada pista en cada milisegundo.
- **Botón [O] GRABAR:** Modifica el estado del secuenciador a modo `Recording`. Mientras este estado permanezca activo, cualquier clic que el ratón efectúe sobre las teclas metálicas de la pantalla ejecutará en cadena la función `sequencer.recordNote(track_activo)`.
- **Botón [-] BORRAR MEMORIA:** Itera a través del `std::vector` de las cajas de texto y ejecuta un borrado físico veloz escribiendo `sequenceBuffers[i][0] = '\0'`. Al introducir este Carácter Nulo de terminación en C++, el sistema trunca la cadena instantáneamente. Es la manera más rústica, eficaz y limpia de vaciar la memoria.

---

## Guía de Compilación (Linux)

### Prerrequisitos
Deberás contar con herramientas de compilación modernas y las bibliotecas de desarrollo de SDL2. Abre una terminal y ejecuta:

```bash
# Para distribuciones basadas en Debian / Ubuntu
sudo apt install build-essential cmake libsdl2-dev libgl1-mesa-dev

# Para distribuciones basadas en Arch Linux / CachyOS
sudo pacman -S base-devel cmake sdl2 mesa
```

### Instrucciones de Compilación
El proyecto integra el motor CMake, diseñado explícitamente para automatizar las descargas de bibliotecas como Dear ImGui sin intervención humana:
1. Dirígete a la carpeta raíz del proyecto (donde reside el archivo base `CMakeLists.txt`).
2. Genera las instrucciones dependientes de tu sistema y compila los binarios:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
3. Ejecuta el binario empaquetado:
```bash
./StylophoneEmulator
```

---

## Guía de Extensibilidad y Modificación

### Cómo ampliar el registro musical (Añadir o remover teclas)
La arquitectura del programa fue construida bajo un paradigma de diseño por adaptación. Para añadir más alcance al teclado (por ejemplo, notas más graves como "C3" o más agudas como "E5"), cualquier programador junior o usuario intermedio puede hacerlo de manera indolora, sin que la aplicación sufra errores lógicos.

1. Abre con cualquier editor de texto el archivo `src/main.cpp`.
2. Localiza la constante vectorial global `STYLOPHONE_KEYS` posicionada en la zona superior (Línea 15 aprox):
```cpp
const std::vector<std::string> STYLOPHONE_KEYS = {
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5"
};
```
3. Incorpora tus nuevas notas musicales encuadrándolas entre comillas y separándolas por comas. (Asegúrate de escribirlas en formato estándar: Nombre + [Sostenido opcional] + Número de Octava).
4. Guarda y vuelve a ejecutar el comando `cmake --build .`

**¿Qué ocurre al compilar?**
El programa se ajustará automáticamente por sí solo sin tocar ni una sola línea de código extra:
- La interfaz de Dear ImGui calculará los nuevos índices, dibujará la cantidad exacta de botones nuevos y aplicará auto-salto de línea al superar múltiplos de 8.
- La biblioteca de `Sequencer` leerá automáticamente estas notas en el futuro. Puesto que no dependen de diccionarios estrictos para funcionar sino de resolución iterativa `getFrequencyFromNote()`, deducirán matemáticamente la frecuencia en Hercios basándose en la distancia interválica en tiempo real.
