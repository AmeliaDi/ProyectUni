# 🎧 RETO: Construye tu propio DAW en C++

¡Bienvenidos al reto de programación de audio! Su misión, si deciden aceptarla, es construir un **Digital Audio Workstation (DAW)** básico pero funcional. 
viva cmake🪄

---

## 🛠️ ¿Qué estamos usando? (El "Stack")

1. **[SFML 3](https://www.sfml-dev.org/):** La ventana negra que ven al abrir el programa. Maneja el ratón, teclado y gráficos base.
2. **[Dear ImGui](https://github.com/ocornut/imgui):** Para la interfaz de usuario. Hacer botones, menús y sliders aquí es tan fácil como escribir `ImGui::Button("Play")`.
3. **[miniaudio](https://miniaud.io/):** Nuestro motor de audio. ¡Ya está descargado en la carpeta `vendor/`!
4. **[SoundTouch](https://codeberg.org/soundtouch/soundtouch) (Pendiente):** Para el futuro. Nos ayudará a estirar el audio (hacerlo rápido o lento) sin que suene como ardilla.

---

## 🏆 LOS NIVELES DEL RETO (Paso a paso)

Para que no se rompan la cabeza, hemos dividido esto en niveles como si fuera un videojuego. Repártanse los niveles entre ustedes.

### 🟢 Nivel 1: "¡Que suene algo, por favor!"
* **Objetivo:** Cargar un archivo `.wav` y reproducirlo al hacer clic en un botón.
* **¿Dónde trabajar?** En `src/main.cpp`.
* **Pistas:** 
  1. Incluyan `#include "miniaudio.h"` (¡ojo, lean la documentación de miniaudio sobre `#define MINIAUDIO_IMPLEMENTATION` antes del include!).
  2. Inicialicen el motor de audio (`ma_engine`).
  3. Cuentan con un botón "Play" en la interfaz. Hagan que cuando el código diga que el botón fue presionado, el motor reproduzca el sonido (`ma_engine_play_sound`).

### 🟡 Nivel 2: "El Controlador"
* **Objetivo:** Poder detener el audio y ajustar el volumen maestro.
* **Pistas:** 
  1. Usen la función de Stop de miniaudio para el botón "Stop".
  2. ¡Agreguen un `ImGui::SliderFloat("Volumen", &volumen, 0.0f, 1.0f);`!
  3. Conecten esa variable `volumen` al volumen general de `miniaudio`.

### 🟠 Nivel 3: "Las Pistas (Tracks)"
* **Objetivo:** Crear una clase en C++ llamada `Pista` o `Track` que represente un canal de audio independiente.
* **Pistas:** 
  1. Deberían poder tener un arreglo o vector de varias pistas: `std::vector<Track> pistas;`
  2. Cada pista debe tener su propio botón "Play" y su propio "Volumen" (¡Usen ImGui para dibujar un rectángulo por cada pista con sus controles!).

### 🔴 Nivel 4: "Visualizando el Sonido" (El desafío divertido)
* **Objetivo:** Dibujar la forma de onda (waveform) de la pista cargada.
* **Pistas:** 
  1. Para hacer esto, ya no pueden usar `ma_engine_play_sound` tan fácil. Tendrán que decodificar el archivo `.wav` en un arreglo de números tipo `float` (PCM data).
  2. Usen la función `ImGui::PlotLines()` o dibujen líneas en SFML leyendo los valores de ese arreglo. ¡Se verá súper profesional!

### 🟣 Nivel 5: "El Jefe Final (BPM & Time-Stretch)"
* **Objetivo:** Sincronizar audios automáticamente.
* **Pistas:** 
  1. Cuando dominen todo lo anterior, descarguen la librería **SoundTouch**.
  2. Pasen su arreglo de floats (el audio decodificado) a SoundTouch.
  3. SoundTouch tiene una clase `BPMDetect`. Pídanle que les diga el BPM de su archivo.
  4. Agreguen un slider "BPM Maestro" en ImGui. Si el proyecto está en 130 y la pista en 120, díganle a SoundTouch: `setTempo(130.0 / 120.0)`. ¡Bum! Audio sincronizado.

---

## 🚀 ¿Cómo compilar y correr?

Ya que los huesos están armados, cada vez que cambien el código en `src/main.cpp`, solo tienen que abrir la terminal y escribir:

```bash
# Compilar el código (apretar las tuercas)
cmake --build build -j$(nproc)

# Ejecutar el programa
./build/DawProject
```

¡Mucha suerte! Tienen la base perfecta para divertirse creando. No duden en buscar tutoriales de **ImGui** y **miniaudio**, son las herramientas más amigables del mundo del C++.
