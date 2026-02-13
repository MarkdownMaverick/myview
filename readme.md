# My View - Media Center
A lightweight, hardware-accelerated media player and library manager built with **C** and **Raylib**. Featuring a grid-based UI, automatic thumbnail generation, and a custom genre-tagging system.
## ✨ Features
* **Media Library:** Automatic scanning and organization of Movies, Videos, and Music.
* **Multi-Genre Tagging:** Assign and filter media by multiple genres (Horror, Comedy, Sci-Fi, etc.).
* **Hardware Video Playback:** High-performance playback powered by the `libmpv` backend.
* **Smart Thumbnails:** Automatic generation of video previews using `ffmpeg`.
* **Favorites System:** Quick access to your most-watched content.
* **Custom UI:** Clean, dark-themed interface with custom logo support and smooth animations.
## 🛠 Prerequisites
Before building, ensure you have the following development libraries installed:
* **Raylib** (UI and Rendering)
* **libmpv** (Video Engine)
* **ffmpeg** (Required for thumbnail generation)
* **cJSON** (Included/Required for settings persistence)
* **Zenity** (Required for the file picker dialog)
## 🚀 Getting Started
Tests have only been done in manjaro, full ubuntu compatibilty will be added soon
### 1. Installation (Linux) 
```bash
# Install dependencies (Ubuntu/Debian example)
sudo apt update
sudo apt install libraylib-dev libmpv-dev ffmpeg zenity
```
### 2. Building
The project includes a `makefile`. Simply run:
```bash
make
```
### 3. Usage
Run the executable from the root directory:
```bash
./myview
```
## 📂 Project Structure
* `src/main.c` - Entry point and main state machine.
* `src/ui.c` - UI rendering logic and Genre Grid.
* `src/player.c` - MPV integration and playback controls.
* `src/settings.c` - JSON persistence for favorites and tags.
* `src/thumbnails.c` - FFmpeg thumbnail extraction logic.
* `gui/` - App assets png glb mp4 etc.
* `fonts/` Font location
* `sfx/` 
## 📝 Configuration
The app saves your preferences in `settings.json`. You can manually edit this file or use the in-app **TAG** and **Favorite** menus to update your library.
```
