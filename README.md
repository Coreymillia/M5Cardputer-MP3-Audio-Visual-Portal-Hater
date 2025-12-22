# XtraCardputer - Multi-Function Pocket Computer

## Overview
XtraCardputer transforms your M5Cardputer into a full-featured pocket computer with multiple applications accessible through an intuitive menu system.

## Features

### 🎯 Core System
- **Boot to Screensaver**: Powers on to the beautiful 341-effect screensaver
- **ESC Menu Access**: Press ESC at any time to access the main menu
- **Auto-Return**: Menu automatically returns to screensaver after 1 minute of inactivity

### 📱 Applications

1. **🎨 Screensaver** 
   - Multiple visual effects (subset of original 341 collection optimized for multi-app environment)
   - Cycles through effects automatically
   - Manual effect switching with keyboard

2. **🎵 MP3 Player**
   - Play audio files from SD card
   - Shuffle mode support
   - Auto-advance to next song
   - Exit with M key

3. **📡 WiFi Scanner**
   - Advanced network scanning with signal strength
   - Security type detection
   - Real-time network updates
   - Scrollable network list

4. **🌐 Matrix Portal**
   - Creates WiFi access point with Matrix-themed captive portal
   - Receive messages from connected devices
   - Digital void message storage
   - Auto-redirect functionality

5. **⏰ Clock & Alarm**
   - Digital clock display with settings
   - Alarm configuration and management
   - Persistent time and alarm settings
   - Real-time clock display

6. **🎮 Games**
   - **Snake**: Classic snake game with high score tracking
   - **Tetris**: Portrait mode Tetris with hold/next piece display
   - **Tetroids**: Asteroids-style space shooter

7. **🔢 Calculator**
   - Basic arithmetic operations (+, -, ×, ÷)
   - Clear function and result display
   - Simple key mapping for M5Cardputer keyboard

8. **📡 Spamtastic Beacon**
   - Multiple beacon categories (Satan, Hacker, Government, etc.)
   - WiFi beacon spam functionality
   - Category-based beacon selection
   - Real-time beacon transmission

9. **🛡️ Evil Portal Hater**
   - Monitor and detect captive portals
   - Custom message broadcasting
   - Send counter-messages
   - User-configurable messages
   - One preset message option

## Hardware Requirements
- M5Cardputer with ESP32-S3
- SD card (for music and file storage)
- Built-in WiFi, keyboard, and display

## Development Plan

### Phase 1: Core Framework ✅
- [x] Project structure
- [ ] Menu system implementation
- [ ] Navigation and input handling
- [ ] Screen management

### Phase 2: Basic Applications
- [ ] Screensaver integration
- [ ] WiFi scanner
- [ ] Clock functionality

### Phase 3: Advanced Features
- [ ] MP3 player integration
- [ ] WiFi captive portal
- [ ] File reader

### Phase 4: Security Features
- [ ] Evil Portal Hater port
- [ ] Message customization

## Technical Architecture

### Menu System
```
Main Menu (ESC to access)
├── 1. Screensaver
├── 2. MP3 Player  
├── 3. WiFi Scanner
├── 4. Captive Portal
├── 5. Clock & Alarm
├── 6. File Reader
└── 7. Portal Hater
```

### Input Handling
- **ESC**: Return to menu from any app
- **Arrow Keys**: Navigate menus
- **Enter**: Select menu item
- **Numbers**: Quick menu selection
- **App-specific**: Each app has custom controls

### Memory Management
- Efficient screen buffer usage
- Minimal RAM footprint per app
- SD card for data storage
- PSRAM utilization for larger operations

## Getting Started

1. Flash firmware to M5Cardputer
2. Insert SD card with music files (optional)
3. Power on - boots to screensaver
4. Press ESC to access menu system
5. Use number keys or arrows to navigate

## Contributing
This project welcomes contributions for new applications and features.

## Credits and Acknowledgments

This project builds upon and integrates code from several excellent open-source projects:

### Core Firmware Inspiration
- **[Bruce](https://github.com/pr3y/Bruce)** - Multi-tool firmware that inspired the menu system and WiFi portal functionality
- **[Nemo](https://github.com/n0xa/m5stick-nemo)** - WiFi security testing framework that contributed to beacon and portal features

### Specialized Components
- **[Open-Wifi-Scanner](https://github.com/geo-tp/Open-Wifi-Scanner)** - Enhanced WiFi scanning capabilities with signal strength and security detection
- **[Spamtastic-M5StickC-Plus2-Beacon](https://github.com/Coreymillia/Spamtastic-M5StickC-Plus2-Beacon)** - Beacon spam functionality and category system
- **[TETROIDS-M5StickC-Plus2](https://github.com/Coreymillia/TETROIDS-M5StickC-Plus2)** - Asteroids-style game ported to M5Cardputer

### Original 341 Effects Port
- **XScreensCYD Project** - Original collection of 341 visual effects, adapted for M5Cardputer hardware constraints

### Development Framework
- **PlatformIO** - Cross-platform build system
- **ESP32 Arduino Framework** - Core development platform
- **M5Cardputer Libraries** - Hardware abstraction and display drivers

### Memory Optimization Note
Due to the multi-application architecture of XtraCardputer, the full 341 visual effects collection has been optimized to fit alongside games, WiFi tools, and other features. For a dedicated screensaver experience with all 341 effects, see the standalone XScreens projects.

Special thanks to the open-source community for creating the foundational tools and libraries that made this project possible.

## License
Open source - specific license TBD