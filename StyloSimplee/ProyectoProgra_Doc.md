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
Es la fábrica de sonido. Es responsable de calcular, número por número, la onda sonora en tiempo real. No utiliza archivos MP3 ni WAV.
- **Variables Clave:** 
  - `m_targetFrequency`: Guarda la nota musical (en unidades de Hercios) que la interfaz gráfica le ordena sonar.
  - `m_phase` y `m_envelope`: Variables de estado internas utilizadas en trigonometría para saber en qué posición exacta de la onda de sonido estamos y para simular cómo el volumen sube y baja abruptamente al presionar un contacto metálico en el instrumento real.
- **Métodos Principales:**
  - `processAudio(float* output, int frameCount)`: Es el corazón de todo el aplicativo. Se llama cientos de veces por segundo en segundo plano.
  
  **¿Cómo se genera exactamente una nota matemática?**
  Para que escuches un tono (por ejemplo, la nota "La" a 440 Hz), el método `processAudio` debe entregarle a tu tarjeta de sonido una lista de 44,100 números (muestras) cada segundo. Lo hace en estos sencillos pasos:
  1. **El Avance o Fase (`m_phase`)**: El programa calcula cuánto avanzará la onda en un solo instante dividiendo la frecuencia deseada entre la velocidad de tu audio (44,100).
  2. **Dibujando la Onda:** Usa ese porcentaje de avance (que va de 0.0 a 1.0) para crear dos formas geométricas puras:
     - *Onda Cuadrada:* Si el avance es menor a la mitad (0.5), el valor numérico será `1.0`; si es mayor, será `-1.0`. Esto nos da el sonido cuadrado clásico de los videojuegos de 8 bits.
     - *Onda de Sierra (Sawtooth):* Es una línea diagonal matemática que sube de `-1.0` a `1.0` y se reinicia. Esto le añade un "zumbido" metálico al tono.
  3. **La Mezcla Final:** Multiplica el 70% del valor de la onda cuadrada más el 30% de la onda de sierra. Este balance fue ajustado manualmente para calcar el sonido real de los circuitos del Stylophone de los años 60.
  4. **Volumen y Filtrado:** Por último, ese número se multiplica por la variable de volumen `m_envelope` (que sube de golpe o se apaga suavemente dependiendo de si tienes el ratón apretado en una tecla) y luego cruza por la "Ecuación de Filtro Paso Bajo", la cual suaviza los picos de la onda para que el pitido no te perfore los tímpanos.

### 2. Motor de Secuenciación (`Sequencer.h` y `.cpp`)
Es el encargado de leer texto, entenderlo y automatizar la música en base al tiempo.
- **Variables Clave:**
  - `std::vector<std::string> m_sequence`: Un arreglo dinámico que funciona como memoria a corto plazo. Guarda cada nota por separado y en orden.
  - `m_timer` y `m_stepDuration`: Acumuladores de tiempo. Leen los "BPM" (Beats/Golpes por minuto) definidos por el usuario en el control deslizante y calculan matemáticamente cuántos milisegundos de espera deben transcurrir entre una nota y otra.
- **Métodos Principales:**
  - `parseSequenceText(const std::string& text)`: Utiliza `std::istringstream` (un flujo o túnel de lectura de texto nativo de C++). Su trabajo es tragar un bloque de texto sucio escrito por el usuario (por ejemplo: "A4     C5\n\n E4"), ignorar todos los espacios o dobles saltos de línea inútiles, y guardar nota por nota en la memoria en una fracción de milisegundo.
  - `getFrequencyFromNote()`: Es un traductor matemático. Convierte una cadena de texto (como "A4") a su frecuencia sonora pura (440.0f) iterando sobre un catálogo base y aplicando la fórmula de potencias de la escala musical occidental.

### 3. Capa de Presentación (`main.cpp`)
El archivo de control central que coordina el dibujo de los gráficos y une la interfaz con la lógica de los motores de audio y secuencias.
- **Lógica de Renderizado:** El bloque `while (!done)` conforma el bucle de renderizado infinito del juego/app. Se encarga de limpiar el lienzo y re-dibujar los botones interactivos a un mínimo de 60 fotogramas por segundo, ajustándose milimétricamente al ancho de la pantalla sin espacios inútiles. 
- **Conexión de Componentes:** En este archivo nacen las variables `StylophoneSynth synth;` y `Sequencer sequencer;`. Para conectar sus cerebros sin mezclarlos, pasamos "funciones anónimas (lambdas)" que le instruyen al secuenciador: "Cuando el tiempo dictamine que debes tocar algo, llama internamente a la función `synth.noteOn(frecuencia)`".

---

## Explicación Funcional de la Interfaz Interactiva

- **Botón [>] PLAY:** Invoca a `sequencer.play()`. Analiza el texto disponible en la pantalla gigante de texto, vacía cualquier memoria de secuencias anterior, la reemplaza por la nueva e inicia el reloj interno que comenzará a mandar señales de `noteOn` a nuestro sintetizador asíncrono en segundo plano.
- **Botón [O] GRABAR:** Modifica el estado o "Modo" del secuenciador a modo `Recording`. Mientras este estado permanezca activo, cualquier clic que el ratón efectúe sobre una tecla metálica ejecutará en cadena la función `sequencer.recordNote()`. Esta función imprime de vuelta hacia la pantalla (añadiendo el respectivo espacio) la nota oprimida por el usuario.
- **Botón [-] BORRAR MEMORIA:** Ejecuta el bloque físico de reinicio compuesto por `sequenceBuffer[0] = '\0';` y `sequencer.setSequenceText("");`. En lenguaje C y C++, la variable `sequenceBuffer` funge como el arreglo de caracteres (char array) primitivo que guarda lo que el usuario ve en la caja de texto. Al introducir como primer carácter al símbolo `'\0'` (conocido como Carácter Nulo de terminación), el sistema operativo entiende inmediatamente que esa cadena de texto ha finalizado, truncándola. Es la manera más rústica, eficaz y de menor consumo de CPU para "vaciar" o purgar la entrada y la memoria de secuencias de un plumazo.

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
    "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", 
    "A4", "A#4", "B4", "C5" 
};
```
3. Incorpora tus nuevas notas musicales encuadrándolas entre comillas y separándolas por comas. (Asegúrate de escribirlas en formato estándar: Nombre + [Sostenido opcional] + Número de Octava).
4. Guarda y vuelve a ejecutar el comando `cmake --build .`

**¿Qué ocurre al compilar?**
El programa se ajustará automáticamente por sí solo sin tocar ni una sola línea de código extra:
- La interfaz de Dear ImGui calculará los nuevos índices, dibujará la cantidad exacta de botones nuevos y aplicará auto-salto de línea al superar múltiplos de 8.
- La biblioteca de `Sequencer` leerá automáticamente estas notas en el futuro. Puesto que no dependen de diccionarios estrictos para funcionar sino de resolución iterativa `getFrequencyFromNote()`, deducirán matemáticamente la frecuencia en Hercios basándose en la distancia interválica en tiempo real.
