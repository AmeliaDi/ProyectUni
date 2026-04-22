# ✨🎧 RETO: Construye tu propio DAW en C++ 🎧✨

¡Hola chicas! Bienvenidas al reto de programación de audio 🎀. Su misión, si deciden aceptarla, es construir un **Digital Audio Workstation (DAW)** súper lindo y funcional. 
viva cmake🪄

---

## 🌸 ¿Qué estamos usando? (Nuestro Tech Stack)

1. **[SFML 3](https://www.sfml-dev.org/):** La ventanita donde sucede la magia. Maneja el ratón, teclado y los gráficos base 💕.
2. **[Dear ImGui](https://github.com/ocornut/imgui):** Para la interfaz. Hacer botones bonitos y sliders aquí es tan fácil como escribir `ImGui::Button("Play")` 💅.
3. **[miniaudio](https://miniaud.io/):** Nuestro motor de audio. ¡Ya está descargado y listo para brillar en la carpeta `vendor/`!
4. **[SoundTouch](https://codeberg.org/soundtouch/soundtouch) (Para después):** Nos ayudará a estirar el audio sin que suene rarísimo.

---

## 👑 LOS NIVELES DEL RETO (Paso a paso)

Para que no nos estresemos, hemos dividido esto en niveles súper fáciles de seguir. ¡Pueden dividirse el trabajo!

### 🎀 Nivel 1: "¡Que suene algo, por favor!"
* **Meta:** Cargar un archivo `.wav` y reproducirlo al hacer clic en un botón.
* **¿Dónde trabajar?** En `src/main.cpp`.
* **Tips:** 
  1. Incluyan `#include "miniaudio.h"` (¡lean la documentación sobre `#define MINIAUDIO_IMPLEMENTATION` primero!).
  2. Inicialicen el motor de audio (`ma_engine`).
  3. Ya tienen un botón "Play" en la interfaz. Conecten ese botón para que el motor reproduzca el sonido (`ma_engine_play_sound`).

### 💖 Nivel 2: "Tomando el Control"
* **Meta:** Poder detener el audio y ajustar el volumen.
* **Tips:** 
  1. Usen la función de Stop de miniaudio para el botón "Stop".
  2. ¡Agreguen un slider de volumen! `ImGui::SliderFloat("Volumen", &volumen, 0.0f, 1.0f);`
  3. Conecten esa variable al volumen general de `miniaudio`.

### 🌷 Nivel 3: "Creando Pistas"
* **Meta:** Crear una clase en C++ llamada `Pista` o `Track` que represente un canal de audio independiente.
* **Tips:** 
  1. Deberían tener una lista de pistas: `std::vector<Track> pistas;`
  2. Cada pista debe tener su propio botón "Play" y "Volumen". ¡Usen ImGui para dibujar un cuadrito lindo por cada pista!

### ✨ Nivel 4: "Visualizando la Música" (El desafío divertido)
* **Meta:** Dibujar la forma de onda (waveform) de la pista cargada.
* **Tips:** 
  1. Tendrán que decodificar el archivo `.wav` en una lista de números (`float`).
  2. Usen `ImGui::PlotLines()` o dibujen líneas en SFML leyendo esos números. ¡Se verá súper pro!

### 🦋 Nivel 5: "El Toque Final (BPM & Time-Stretch)"
* **Meta:** Sincronizar audios automáticamente.
* **Tips:** 
  1. Descarguen la librería **SoundTouch**.
  2. Pasen el audio a SoundTouch y usen su clase `BPMDetect` para saber el ritmo.
  3. Agreguen un slider "BPM Maestro". Si el proyecto está en 130 y la pista en 120, díganle a SoundTouch: `setTempo(130.0 / 120.0)`. ¡Audio sincronizado a la perfección!

---

## 💅 ¿Cómo compilar y correr?

Cada vez que hagan cambios en `src/main.cpp`, solo abran su terminal y escriban esto:

```bash
# Compilar el código (armar todo)
cmake --build build -j$(nproc)

# Ejecutar el programa ✨
./build/DawProject
```

¡Mucha suerte! Tienen todo lo necesario para brillar. Pueden buscar tutoriales de **ImGui** y **miniaudio**, son súper amigables. ¡Ustedes pueden! 💕
