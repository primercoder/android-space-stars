# stars — N-Body Gravity Simulation on Android

Interactive 2D gravitational n-body simulation for Android. Tap to spawn stars that orbit and collide via Newtonian gravity, with glowing trails and color blending on merge.

<p align="center">
  <img src="asserts/stars_short1.png" width="45%" />
  <img src="asserts/stars_short2.png" width="45%" />
</p>

## Features

- **N-body physics** — RK4 integration with 50 substeps per frame for stable orbits
- **Tap to create** — tap a spot to place a static star, drag to launch a star with velocity
- **Pan mode** — toggle MOVE mode to drag and explore the infinite canvas
- **Star collisions** — inelastic merging with colour blending
- **Glowing trails** — fading ribbons rendered per star
- **Circular particles** — smooth GLES 2.0 shader-based rendering
- **HUD** — star count, total ever created, pause indicator

## Controls

| Button | Action |
|--------|--------|
| **LAUNCH** (default mode) | Tap = static star, Drag = launch with velocity |
| **MOVE** | Drag to pan camera |
| **Pause / Go on** | Toggle simulation |
| **Reset** | Clear all stars and trails |
| **Grid / Hide** | Toggle background grid |

## Build

### Prerequisites

- Android SDK (API 34)
- Android NDK 27+
- CMake 3.28+
- Ninja

### Setup SDL2

The project requires SDL2 source at `app/jni/SDL/` (the NDK CMake build compiles it from source).

```bash
# Download SDL2
curl -L https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/SDL2-2.30.11.tar.gz -o /tmp/SDL2.tar.gz
tar xzf /tmp/SDL2.tar.gz -C /tmp

# Place it where the build expects it
mv /tmp/SDL2-2.30.11 app/jni/SDL
```

### Build release APK

```bash
./gradlew assembleRelease
```

Output: `app/build/outputs/apk/release/app-release.apk`

### Install

```bash
adb install -r app/build/outputs/apk/release/app-release.apk
```

## Tech Stack

- **C++20** — simulation engine
- **OpenGL ES 2.0** — shader-based rendering
- **SDL2** — windowing, input, Android glue
- **GLM** — vector math
- **Gradle + NDK** — Android build system
