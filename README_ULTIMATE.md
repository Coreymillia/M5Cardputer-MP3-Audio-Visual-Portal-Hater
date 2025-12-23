# XtraCardputer - Ultimate Multi-Function Pocket Computer

**The most comprehensive M5Cardputer firmware ever created!** 🎉

## 🎯 Overview

XtraCardputer transforms your M5Cardputer into a full-featured pocket computer with 12+ applications, games, WiFi security tools, and entertainment features - all accessible through an intuitive menu system.

## ✨ Core System Features

### 🎨 **Boot Experience**
- **Boot to Screensaver**: Powers on to beautiful 28-effect visual display
- **Menu Access**: Press **FnM** at any time to access the main menu
- **Auto-Return**: Menu automatically returns to screensaver after 1 minute of inactivity
- **Smart Navigation**: Seamless transitions between all applications

## 📱 Applications & Features

### 🎨 **1. Screensaver System (28 Effects)**
- Multiple visual effects (optimized subset from original 341 collection)
- **Spacebar**: Toggle effects on/off while running
- **Automatic cycling** through effects
- **Manual switching** with keyboard navigation
- Effects include: Plasma, Matrix Rain, Life, Geometric patterns, Energy streams, and more!

### 🎵 **2. MP3 Player with ESP8266Audio**
- **High-quality MP3 playback** from SD card
- **V key**: Toggle visual screensaver (defaults OFF for stable audio) ⭐
- **Auto-scanning**: Scans entire SD card for MP3 files
- **Controls**: P=Play/Pause, N=Next, B=Previous, S=Shuffle, ←/→=Volume, R=Rescan
- **Performance Note**: Initial scan of 1,900 songs takes 10-15 minutes (one-time only)
- **File Location**: Place MP3s in SD root or folders (Music directory supported)
- **Exit**: M key returns to main menu

### 📡 **3. Advanced WiFi Scanner**
- **Real-time network scanning** with signal strength (RSSI)
- **Security type detection** (Open, WPA, WPA2, WEP)
- **Scrollable network list** with detailed information
- **Live updates** as networks appear/disappear
- **Professional-grade scanning** for security assessment

### 🌐 **4. Matrix Portal**
- **Creates WiFi access point** with Matrix-themed interface
- **Captive portal functionality** with auto-redirect
- **Message collection system** from connected devices
- **Digital void storage** for received messages
- **Custom Matrix aesthetic** with green-on-black styling

### ⏰ **5. Clock & Alarm System**
- **Digital clock display** with NTP time sync
- **Visual alarm notifications** with screen effects
- **Persistent settings** stored in preferences
- **Real-time updates** with ESP32Time integration
- **Timezone support** and automatic DST handling

### 🎮 **6. Games Collection**

#### **Tetris Landscape Mode**
- **W**=Left, **E**=Right, **A** or **S**=Soft drop
- **2** or **3**=Hard drop
- **I**=Rotate counter-clockwise, **O**=Rotate clockwise
- **8** or **9**=Hold piece

#### **Tetris Portrait Mode** 
- **B**=Left, **H**=Right, **G** or **V**=Hard drop
- **J** or **N**=Soft drop
- **9**=Rotate clockwise, **P**=Rotate counter-clockwise
- **8** or **I**=Hold piece

#### **Tetroids (Asteroids-style)**
- **D**=Rotate counter-clockwise, **F**=Rotate clockwise
- **L**=Forward thrust, **K**=Fire
- **P**=Warp (emergency teleport)
- Classic space physics and asteroid destruction

#### **Calculator**
- **Full scientific calculator** with basic arithmetic
- **+, -, ×, ÷** operations with decimal support
- **Clear function** and result display
- **Optimized key mapping** for M5Cardputer keyboard

### 📡 **7. Spamtastic Beacon**
- **Multiple beacon categories**: Satan, Hacker, Government, Corporate, etc.
- **WiFi beacon spam functionality** for testing
- **Category-based selection** with themed SSIDs
- **Real-time transmission** with status display
- **Educational/testing purposes** for network security

### 🛡️ **8. Evil Portal Hater (2 Modes)**
- **Monitor and detect** rogue captive portals
- **Preset Mode**: Quick counter-messaging with built-in responses
- **Custom Mode**: User-configurable counter-messages
- **Real-time broadcasting** of warning messages
- **Network protection** against evil twin attacks

### 🔍 **9. Deauth Hunter**
- **Monitor deauthentication attacks** on WiFi networks
- **Real-time detection** of WiFi jamming attempts
- **Network security assessment** tool
- **Attack pattern analysis** and logging

### 🍯 **10. PineAP Hunter**
- **Detect PineAP devices** and evil twin access points
- **BSSID tracking** and suspicious network identification  
- **Multi-SSID analysis** for rogue AP detection
- **Security assessment** for penetration testing

### 😂 **11. 3,541 Joke Scroller**
- **Massive collection** of 3,541 jokes and puns
- **Auto-scroll mode** with customizable timing
- **Manual navigation**: W/S for previous/next
- **A key**: Toggle auto-mode on/off
- **Entertainment system** for breaks between serious work

### 📊 **12. Audio Visualizer**
- **Real-time audio visualization** with microphone input
- **Multiple visualization modes** (spectrum, waveform, etc.)
- **W/S keys**: Switch between visualization modes
- **Integration** with MP3 player for music visualization

## 🔧 M5Burner Compatibility Solution

### **The Challenge We Solved**
During development, we discovered M5Burner was rejecting our merged binaries despite them being valid ESP32-S3 firmware. After extensive investigation, we found the solution:

### **Critical Fix: Flash Size Configuration**
```bash
# ❌ What Doesn't Work (causes M5Burner rejection)
esptool --chip esp32s3 merge-bin --flash-size 4MB -o firmware.bin [...]

# ✅ What Works (M5Burner accepts)
esptool --chip esp32s3 merge-bin --flash-size 8MB -o firmware.bin [...]
```

### **Complete M5Burner Build Process**
```bash
# 1. Build with PlatformIO
pio run

# 2. Create M5Burner-compatible merged binary
esptool --chip esp32s3 merge-bin \
  --flash-mode dio \
  --flash-freq 80m \
  --flash-size 8MB \
  -o XtraCardputer-Ultimate.bin \
  0x0000 .pio/build/m5stack-cardputer/bootloader.bin \
  0x8000 .pio/build/m5stack-cardputer/partitions.bin \
  0x10000 .pio/build/m5stack-cardputer/firmware.bin

# 3. Upload to M5Burner - Download button should appear!
```

### **Key Discovery**
M5Burner uses **flash size as a validation parameter**, not just for actual flashing. Even though M5Cardputer has sufficient flash memory, using `4MB` in the merge command triggers M5Burner's internal rejection heuristics. **Using `8MB` allows M5Burner to recognize and accept the firmware.**

## 🎛️ Universal Controls

### **Main Menu Navigation**
- **FnM**: Access main menu from anywhere
- **Number keys (1-12)**: Quick app selection  
- **Arrow keys**: Navigate menu options
- **Enter**: Select highlighted item
- **M**: Exit current app to main menu

### **Global Controls**
- **ESC**: Emergency return to menu
- **FnM**: Menu access override
- **Auto-timeout**: Returns to screensaver after inactivity

## 🔧 Hardware Requirements

- **M5Stack Cardputer** with ESP32-S3
- **MicroSD card** (for MP3 files and data storage)
- **Built-in components**: WiFi, keyboard, display, speaker
- **Optional**: External speaker for enhanced MP3 audio

## 📁 Technical Specifications

### **Firmware Stats**
- **Source Code**: 7,793 lines of optimized C++
- **Compiled Size**: ~1.6MB (37.4% of 4MB flash)
- **RAM Usage**: 124KB (38.0% of 327KB)
- **Flash Format**: 8MB configuration for M5Burner compatibility

### **Architecture**
- **Memory Management**: Efficient screen buffer usage, minimal RAM per app
- **Storage**: SD card for user data, preferences in NVRAM
- **Processing**: PSRAM utilization for complex operations
- **Network**: Full WiFi stack with AP/STA modes

### **Key Dependencies**
- M5Cardputer library for hardware abstraction
- ESP8266Audio for high-quality MP3 playback
- ESP32Time for RTC and NTP synchronization
- M5Unified for display management

## 🚀 Getting Started

### **Method 1: M5Burner (Recommended)**
1. Download `XtraCardputer-ULTIMATE-WITH-TOGGLE.bin` from releases
2. Open M5Burner, select M5Cardputer device
3. Upload the .bin file - Download button should appear
4. Flash and enjoy the ultimate M5Cardputer experience!

### **Method 2: PlatformIO Development**
```bash
git clone [this-repo]
cd "Official XtraCardPuter Repo"
pio run --target upload
```

### **First Boot**
1. Power on - automatically boots to screensaver
2. Press **FnM** to access the main menu
3. Use number keys (1-12) for quick app selection
4. Insert SD card with MP3 files for music playback
5. Explore all 12 applications and features!

## 📈 Development Journey

This project represents months of intensive development:

1. **Foundation** - Menu system and basic applications
2. **Games Integration** - Tetris variants and calculator  
3. **WiFi Security Tools** - Scanner, Portal Hater, PineAP Hunter
4. **Entertainment** - MP3 player, joke scroller, visualizer
5. **M5Burner Compatibility** - Solved flash size validation issue
6. **Polish Phase** - Audio fixes, screensaver toggle, stability

## 🎉 Project Achievements

- **M5Burner Compatible** - Works perfectly with official M5Stack tool
- **12+ Applications** - Most comprehensive M5Cardputer firmware available
- **Stable Audio Playback** - No crashes, toggle screensaver control
- **Professional Polish** - Intuitive controls, error handling, documentation
- **Security Focus** - Multiple WiFi assessment and protection tools
- **Entertainment Value** - Games, music, jokes, and visual effects

## 🙏 Credits and Acknowledgments

This project builds upon excellent open-source work:

### **Core Inspiration**
- **Bruce** - Multi-tool firmware inspiring menu system
- **Nemo** - WiFi security framework for portal features
- **XScreensCYD** - Original 341 visual effects collection

### **Specialized Components** 
- **Open-Wifi-Scanner** - Enhanced scanning capabilities
- **Spamtastic-M5StickC** - Beacon spam functionality
- **TETROIDS-M5StickC** - Asteroids game port

### **Development Framework**
- **PlatformIO** - Cross-platform build system
- **ESP32 Arduino** - Core development platform  
- **M5Cardputer Libraries** - Hardware abstraction

## 📄 License

Open source project - contributions welcome! See individual component licenses for specific terms.

---

**Made with ❤️ for the M5Stack Community**

*This represents the culmination of extensive development work to create the ultimate M5Cardputer experience. From solving M5Burner compatibility to perfecting audio playback, every feature has been carefully crafted for maximum functionality and user experience.* 🚀

**Enjoy your ultimate pocket computer!** 🎯