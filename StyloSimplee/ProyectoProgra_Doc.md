# Documentación Técnica Avanzada: Proyecto Proga StyloSimple V1.3

> **Versión:** 1.3 - *Compiladote*  
> **Autores / Colaboradores (Temas):** Fer, enora  
> **Tecnologías Base:** C++20, SDL2, OpenGL3, Dear ImGui, Miniaudio.

---

## 1. Visión General del Proyecto

**StyloSimple** no es solo un emulador de un juguete retro de los años 60; es un motor de **Síntesis Digital de Señales (DSP)** y una **Estación de Trabajo de Audio Digital (DAW)** multipista en miniatura, construido completamente desde cero en C++20. 

El objetivo principal de este proyecto es demostrar un control riguroso sobre la memoria, la concurrencia multihilo (multithreading) y las matemáticas aplicadas al sonido, manteniendo una latencia de hardware casi nula. Todo esto envuelto en una interfaz gráfica altamente responsiva, estilizada con estéticas retro dinámicas.

---

## 2. Arquitectura de Software Rigurosa

Para que un emulador de audio se sienta "real", la latencia (el retraso entre hacer clic y escuchar el sonido) debe ser inferior a 15 milisegundos. Para lograr esto, StyloSimple divide su arquitectura en dos mundos paralelos:

1. **El Hilo Principal (GUI Thread):** Corre a unos 60-144 FPS. Se encarga exclusivamente de dibujar la interfaz, leer los clics del ratón (SDL2) y actualizar el reloj del secuenciador.
2. **El Hilo de Audio (DSP Thread):** Corre a velocidades extremas. Es invocado por la tarjeta de sonido (a través de `miniaudio`) y procesa 44,100 muestras matemáticas por segundo.

### 2.1 Concurrencia Lock-Free (El problema de los Mutex)
Si utilizamos primitivas de sincronización clásicas como los `std::mutex` para comunicar el Hilo Principal con el Hilo de Audio, estaríamos forzando al motor de sonido a "esperar" a la interfaz gráfica. Esa espera produce micro-cortes auditivos horribles conocidos como *clicks* o *pops*. 

**La Solución Experta:** 
En lugar de bloquear hilos, utilizamos variables **atómicas** (`std::atomic<float>` y `std::atomic<bool>`) con un modelo de memoria relajado (`std::memory_order_relaxed`). Esto garantiza a nivel de hardware que, cuando la UI escribe una nueva frecuencia (ej. el usuario presiona una tecla), el hilo de audio siempre leerá un valor coherente sin necesidad de bloquearse. Es un flujo de información unidireccional ultrarrápido y seguro.

### 2.2 Gestión de Memoria (RAII)
No hay un solo `delete` suelto en la lógica principal ni se emplean punteros crudos (`raw pointers`) más allá del encapsulamiento estricto de las librerías en C. Los recursos se adquieren en los constructores y se liberan en los destructores (RAII). Las pistas de audio polifónicas se almacenan en un `std::array<SynthVoice, 16>` estático (memoria en el Stack), prohibiendo estrictamente el uso de memoria dinámica (`std::vector` o `new`) dentro del callback de audio para evitar pausas por el *Garbage Collection* o *Page Faults* del sistema operativo.

---

## 3. Despiece de Módulos (Deep Dive)

### 3.1 Motor DSP y Síntesis de Audio (`StylophoneSynth.h / .cpp`)
Este módulo es un sintetizador aditivo/sustractivo puro. No utiliza archivos MP3 ni WAV. Dibuja formas geométricas en el aire utilizando matemáticas.

- **Generación de Onda:** 
  Un Stylophone original suena "metálico" y "zumbante". Matemáticamente, esto se logra mezclando una **Onda Cuadrada** (armónicos impares) y una **Onda de Sierra** (todos los armónicos). En `processAudio()`, un acumulador de fase (que va de 0.0 a 1.0) dibuja ambas ondas, y el motor mezcla un **70% de Cuadrada con un 30% de Sierra**, replicando el voltaje de los circuitos analógicos originales.
  
- **Filtro de Paso Bajo (Low-Pass Filter):** 
  Las ondas puras generadas digitalmente tienen picos infinitos (Aliasing) que lastiman el oído. Se implementó una ecuación de diferencias de un filtro de 1-polo (RC Filter discreto) configurado a 2500Hz. Esto "suaviza" las esquinas de la onda, entregando un sonido cálido.

- **Soft Clipping (Limitador Analógico):**
  StyloSimple es multipista (polifónico a 16 voces). Si el usuario reproduce 10 notas a la vez, la suma matemática de las ondas superará el límite digital de amplitud (`1.0`), creando una distorsión catastrófica (Digital Hard Clipping). Para solucionar esto como verdaderos expertos en audio, toda la mezcla maestra pasa por la función `std::tanh(mixedSample)`. La tangente hiperbólica es una curva en "S" que comprime suavemente los picos altos (Soft Clipping), manteniendo el volumen robusto y cálido sin importar cuántas voces estén sonando a la vez.

- **Envolventes (ADSR Simplificado):**
  Para evitar ruidos estáticos al presionar o soltar una tecla, el volumen no salta de 0 a 1. Se implementó una rampa lineal de Ataque (`Attack`, 10ms) y Relajación (`Release`, 50ms) multiplicada a la onda de salida final.

### 3.2 Cerebro de Secuenciación (`Sequencer.h / .cpp`)
Es un motor DAW (*Digital Audio Workstation*) integrado capaz de leer texto y convertirlo en eventos de tiempo exacto.

- **Manejo del Tiempo (`deltaTime`):** 
  En lugar de usar retrasos bloqueantes (`sleep()`), el método `update(float deltaTime)` lee cuántos milisegundos reales tardó el procesador en dibujar el último frame. Acumula ese tiempo y, basándose en la ecuación `60 / BPM`, sabe el momento milimétricamente exacto para avanzar el "cabezal de reproducción" al siguiente compás (step).
  
- **Traductor Matemático Temperado (`getFrequencyFromNote`):**
  Lee las cadenas de texto del usuario (como `"C#5"`) y extrae la nota y la octava. Basado en la afinación temperada moderna (donde A4 = 440 Hz), busca el semitono correspondiente en un diccionario estático y calcula la frecuencia con total precisión matemática sin uso de condicionales masivos (if/else). También reconoce el token mágico `"00"` y lo interpreta explícitamente como un silencio musical, retornando `0.0f` para que la voz asíncrona se apague.

### 3.3 Capa de Presentación (`main.cpp`)
El orquestador visual construido sobre el framework gráfico de modo inmediato, **Dear ImGui**.

- **Sincronización de Memoria (ImGui vs C++):** ImGui está escrito primariamente en C, por lo que demanda punteros crudos a arreglos de caracteres (`char[]`) para las cajas de texto. El `main.cpp` puentea de forma brillante la seguridad de `std::string` del secuenciador hacia un buffer local `std::array<char, 2048>`, copiando su estado ida y vuelta en cada frame sin sobrecargar la memoria caché.
- **Temas Dinámicos:** Se extrajeron todas las constantes de color hardcodeadas hacia una estructura global `ThemeColors`. Un menú desplegable modifica en caliente toda la paleta de la aplicación con temas que van desde el crudo *"Matrix Verde"* hasta el popular perfil para programadores *"Monokai Hacker"*.

---

## 4. Glosario Analítico de Funciones (API Interna)

Para garantizar un entendimiento total y riguroso de la arquitectura, a continuación se desglosa el propósito exacto de cada función y método de nuestras clases principales. Entender para qué sirve cada "void" o retorno es vital para dominar la ingeniería detrás de StyloSimple:

### Clase `StylophoneSynth` (Capa DSP)
- **`StylophoneSynth()` (Constructor):** Inicializa el sintetizador, asignando los punteros del hardware a nulo y estableciendo la frecuencia de muestreo base en 44100Hz.
- **`~StylophoneSynth()` (Destructor):** Invoca a `close()`. Garantiza mediante RAII que no queden procesos "zombies" ni memoria sucia al cerrar la ventana.
- **`bool init()`:** Crea y configura el hilo asíncrono de `miniaudio` (definiendo modo Mono y formato Float32), e inyecta la función puente `data_callback`. Retorna un booleano `true` si la tarjeta de sonido del usuario respondió correctamente.
- **`void close()`:** Destruye y desvincula de forma explícita y segura el dispositivo lógico de hardware.
- **`void noteOn(int track, float frequency)`:** Invocado por el Secuenciador o la UI táctil. Recibe un índice de pista (`track`) y una frecuencia tonal en Hercios. Escribe de manera atómica (`memory_order_relaxed`) estos valores en la estructura individual `SynthVoice` y "abre su compuerta" interna (`Gate = true`).
- **`void noteOff(int track)`:** Apaga atómicamente la compuerta de una pista específica (`Gate = false`), lo que interrumpe la oscilación matemática e inicia la fase de relajación (*Release*) de su envolvente de volumen.
- **`void processAudio(float* output, int frameCount)`:** El núcleo matemático absoluto. Es ejecutado exclusivamente por miniaudio en un hilo secundario de extrema prioridad. Itera `frameCount` veces, calculando microsegundo a microsegundo la suma iterativa de osciladores de sierra/cuadrada, envolventes y filtros paso-bajo para las 16 pistas. Finalmente aplica *Soft Clipping* y deposita la matriz sonora resultante en el buffer de salida (`output`).

### Clase `Sequencer` (Capa DAW y Rítmica)
- **`Sequencer()` (Constructor):** Calcula la duración inicial matemática del compás basándose en el estándar pop de 120 BPM e instancia la **Pista 1** por defecto.
- **`void setNoteCallbacks(...)`:** Recibe dos expresiones *Lambda* (`std::function`) inyectadas directamente desde `main.cpp`. Su función es guardar estos "cables lógicos" para que el secuenciador pueda mandar señales directas de encendido y apagado al sintetizador, logrando un desacoplamiento perfecto entre clases.
- **`void setBPM(int bpm)`:** Recalcula dinámicamente la constante de tiempo algorítmica `m_stepDuration` (duración exacta de cada compás/paso expresada en segundos reales) dividiendo `60.0f` entre los BPM proporcionados.
- **`void addTrack()`:** Empuja (`push_back`) nuevos *arrays* vacíos a los vectores multidimensionales, escalando instantáneamente la capacidad polifónica del motor a demanda en tiempo de ejecución.
- **`void play(...)`:** El Analizador Léxico (*Lexical Parser*). Se deshace de toda la memoria rítmica antigua, itera sobre los crudos strings ingresados por el usuario, los fragmenta utilizando flujos espaciales (`std::istringstream`), y los inyecta en los vectores de reproducción paralela. Por último, inicia el cronómetro maestro cambiando su estado a `Playing`.
- **`void record()` / `void stop()`:** Mutadores simples de la máquina de estados. Transicionan las banderas atómicas internas a `SequencerMode::Recording` o `SequencerMode::Idle`, reiniciando los acumuladores de tiempo si el flujo lo amerita.
- **`void recordNote(int track, const std::string& note)`:** Función de inyección en vivo. Al interactuar el usuario con el piano virtual, este método anexa el texto exacto (ej. `"A#4"`) al final de la memoria lineal de la pista especificada.
- **`void update(float deltaTime)`:** El Acumulador de Latencia. Suma los milisegundos reales que tardó la computadora en procesar cada fotograma. Al rebasar la marca temporal de `m_stepDuration`, llama a `triggerCurrentNotes()` y adelanta algorítmicamente un paso el "cabezal de reproducción" de todos los vectores paralelos.
- **`void triggerCurrentNotes()`:** El orquestador de eventos asíncronos. Itera a través de absolutamente todas las pistas activas, recupera el *string* actual de la matriz bidimensional, invoca la traducción frecuencial, y dispara los callbacks (hilos) hacia la tarjeta de sonido.
- **`static float getFrequencyFromNote(...)`:** El decodificador tonal determinista. Empleando la teoría de distancias de semitonos occidentales (A4=440Hz), convierte un crudo String (notación anglosajona) a una frecuencia matemática perfecta (Float). Reconoce implícitamente el uso de tokens como `"00"` devolviendo `0.0f` para representar Silencios Digitales (`Rests`).

---

## 5. Manual de Usuario Interfaz (UI)

La interfaz gráfica no es solo visual, reacciona y retroalimenta al usuario:

1. **Selector de Temas:** Esquina superior derecha. Cambia la estética de toda la aplicación al instante.
2. **Teclado Extendido:** Rango gigantesco desde C2 hasta G5 (44 teclas). El tamaño (padding/width) fue matemáticamente ajustado para que nombres como `C#4` encajen perfectamente en su bounding box.
3. **Reproducción Multipista y Feedback Dinámico:** Cuando presionas `[>] PLAY`, cada pista se evalúa paralelamente. El nombre de la pista (ej. `TRK 1>`) cambiará dinámicamente mostrándote en tiempo real la nota que está escupiendo (ej. `TRK 1 [D#4]>`) y **brillará intensamente** con el color de acento de tu tema seleccionado. Esto te permite visualizar auditivamente qué instrumento hace qué melodía.
4. **Grabación Dirigida (Radio Buttons):** A la izquierda de cada pista verás un pequeño círculo seleccionable. Si eliges, por ejemplo, el TRK 2, al presionar `[O] GRABAR` toda tu interacción con el teclado (ratón o pantalla táctil) se registrará directa y exclusivamente en esa pista. Perfecto para superponer acordes y armonías sin arruinar tu bajo principal.
5. **Borrado Seguro:** El botón `[-] BORRAR MEMORIA` inserta inmediatamente el carácter nulo (`'\0'`) en la primera posición de memoria de todas las pistas. Es el método algorítmico más rápido (O(1)) para vaciar buffers pesados en C++.

---

## 6. Guía de Compilación (Linux)

### Prerrequisitos de Arquitectura
StyloSimple requiere compiladores modernos compatibles con el estándar C++20 y los encabezados de desarrollo gráfico para Wayland/X11:

```bash
# Para distribuciones Debian / Ubuntu / Pop_OS!
sudo apt install build-essential cmake libsdl2-dev libgl1-mesa-dev

# Para distribuciones Arch Linux / Manjaro / CachyOS
sudo pacman -S base-devel cmake sdl2 mesa
```

### Instrucciones de Compilación (Out-of-Source Build)
El proyecto utiliza CMake con configuración automática de dependencias externas (trae su propia versión de ImGui embebida):

```bash
# 1. Crear directorio aislado para binarios (para no manchar el código fuente)
mkdir build && cd build

# 2. Generar árbol de dependencias (Makefiles)
cmake ..

# 3. Compilar usando todos los hilos del procesador disponibles
cmake --build . 

# 4. Ejecutar el demonio musical
./StylophoneEmulator
```

---

## 7. Guía de Extensibilidad (Modding)

La arquitectura de StyloSimple abraza fuertemente el principio Abierto/Cerrado (Open/Closed Principle de SOLID). Puedes expandir sus límites sin tocar su lógica interna.

### ¿Cómo ampliar el registro musical? (Más Teclas)
El sistema utiliza un diseño adaptativo. No hay números fijos hardcodeados en el renderizado del teclado de la UI. 

1. Abre el archivo `src/main.cpp`.
2. Localiza el vector global `STYLOPHONE_KEYS` (Línea 20 aprox).
3. Agrega las notas que desees (respetando la sintaxis `[NOTA][#][OCTAVA]`, ejemplo: `"C6"`).
4. Recompila con `make` (o `cmake --build .`).

**¿Qué sucederá mágicamente?**
- `ImGui` iterará automáticamente tu vector. Ajustará el layout de la pantalla, dibujará los botones nuevos y hará salto de línea (wrap) tras el onceavo botón sin romper las matemáticas del diseño.
- `Sequencer::getFrequencyFromNote()` no tiene diccionarios estáticos limitantes; deduce la altura tonal basándose en las distancias interválicas, así que automáticamente le enviará a `StylophoneSynth` la frecuencia precisa en Hz de tus nuevas notas extremas. 

> ¡El motor no se rompe, el motor se adapta!
