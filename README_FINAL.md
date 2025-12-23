# XtraCardputer - Ultimate Multi-Function Pocket Computer

**The most comprehensive M5Cardputer firmware ever created!** 🎉

## 🎯 Project Overview

XtraCardputer transforms your M5Cardputer into a fully-featured portable computer with games, WiFi tools, multimedia player, and 28+ screensavers. This project represents months of development and optimization to create the ultimate M5Cardputer experience.

## ✨ Features

### 🎵 **MP3 Player with ESP8266Audio**
- High-quality MP3 playback from SD card
- Shuffle mode, volume control, next/previous
- **NEW: Toggleable screensaver** (V key - defaults OFF for stable audio)
- Auto-scanning of MP3 files

### 🎮 **Games**
- **Tetris** (landscape mode optimized)
- **Calculator** (full scientific functions)  
- **Tetroids** (enhanced Tetris variant)
- **Asteroids** (classic arcade action)

### 📡 **WiFi & Security Tools**
- **WiFi Scanner** (detailed network analysis)
- **Portal Hater** (rogue AP detection)
- **PineAP Hunter** (evil twin detection)
- **Custom Portal** (captive portal testing)

### 🌐 **Matrix Portal**
- WebServer with captive portal
- Matrix-themed interactive interface
- Comment collection system

### 🎨 **28 Screensavers**
- Plasma, Matrix Rain, Life, Xmatrix
- Geometric patterns, particle systems
- Energy streams, binary effects
- Musical visualizations and more!

### ⚡ **System Features**
- ESP32Time with NTP sync
- Persistent preferences storage
- SD card file management
- Serial debugging support

## 🚀 Quick Start

### **Method 1: M5Burner (Recommended)**
1. Download `XtraCardputer-ULTIMATE-WITH-TOGGLE.bin` from releases
2. Open M5Burner, select M5Cardputer
3. Upload the .bin file - Download button should appear
4. Flash and enjoy!

### **Method 2: PlatformIO Development**
```bash
git clone <this-repo>
cd "Official XtraCardPuter Repo"
pio run --target upload
```

## 🎛️ Controls

### **Main Menu**
- Number keys (1-9) = Select function
- M = Exit current function

### **MP3 Player**
- P = Play/Pause
- N = Next song, B = Previous song
- S = Shuffle toggle
- **V = Visual screensaver toggle** ⭐
- ←/→ = Volume control
- R = Rescan SD card
- M = Exit to main menu

### **Games**
- W/S = Up/Down navigation
- A/D or specific game keys = Actions
- M = Exit to main menu

## 📁 Project Structure

```
Official XtraCardPuter Repo/
├── src/
│   └── main.cpp              # Main application code (~8000 lines)
├── include/                  # Header files
├── lib/                      # Custom libraries
├── releases/                 # Pre-built binaries
│   └── XtraCardputer-ULTIMATE-WITH-TOGGLE.bin
├── platformio.ini            # PlatformIO configuration
├── partitions_large.csv      # Custom partition table
└── README.md                 # This file
```

## 🔧 Technical Details

### **Hardware Requirements**
- M5Stack Cardputer (ESP32-S3 based)
- MicroSD card (for MP3 files)
- Optional: External speaker for better audio

### **Firmware Specifications**
- **Size**: ~1.6MB compiled
- **Flash**: 8MB configuration required for M5Burner
- **RAM**: ~124KB used
- **Platform**: ESP32-S3 with Arduino framework

### **Key Dependencies**
- M5Cardputer library
- ESP8266Audio for MP3 playback
- ESP32Time for RTC functionality
- M5Unified for display management

## 🐛 Known Issues & Solutions

### **M5Burner Compatibility**
- ✅ **SOLVED**: Use 8MB flash size (not 4MB)
- The `XtraCardputer-ULTIMATE-WITH-TOGGLE.bin` is properly formatted

### **MP3 Audio Issues**
- ✅ **SOLVED**: Screensaver defaults to OFF
- Use V key to toggle screensaver only when desired
- Audio remains stable with screensaver disabled

### **SD Card Requirements**
- Format as FAT32
- Place MP3 files in root directory or subfolders
- Ensure sufficient free space for file operations

## 📈 Development History

This project went through extensive development phases:

1. **Base Development** - Core menu system and basic features
2. **Game Integration** - Added Tetris, Calculator, Asteroids
3. **WiFi Tools** - Portal Hater, WiFi Scanner, security tools
4. **MP3 Player** - ESP8266Audio integration and optimization
5. **Screensaver System** - 28+ visual effects and animations
6. **M5Burner Optimization** - Fixed flash size and partition issues
7. **Final Polish** - Audio fixes, toggle controls, perfect stability

## 🎉 Project Achievements

- **M5Burner Compatible** - Works perfectly with M5Stack's official tool
- **Stable Audio Playback** - No crashes or glitches
- **Comprehensive Feature Set** - 9 main functions, 28+ screensavers
- **Professional Polish** - Intuitive controls, error handling
- **Future-Proof** - Well-documented, modular code structure

## 🛠️ Development Environment

- **IDE**: PlatformIO (recommended) or Arduino IDE
- **Platform**: Espressif32
- **Framework**: Arduino
- **Board**: ESP32-S3-DevKitC-1

## 📄 License

This project is open source. See individual component licenses for specific terms.

## 🙏 Acknowledgments

- M5Stack for the amazing Cardputer hardware
- ESP8266Audio library contributors
- Open source community for tools and libraries
- Extensive testing and feedback from the maker community

---

**Made with ❤️ for the M5Stack Community**

*This represents the culmination of extensive development work to create the ultimate M5Cardputer experience. Enjoy your pocket computer!* 🚀