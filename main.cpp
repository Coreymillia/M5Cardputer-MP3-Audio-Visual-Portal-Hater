// XtraCardputer v4.0 - Multi-Function Pocket Computer - COMPLETE BUILD 2024-12-22  
// Full-featured build - ALL functionality restored. Multi-file M5Burner approach.

#include <M5Cardputer.h>
#include <WiFi.h>
#include <ESP32Time.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <math.h>
#include "jokes.h"
#include "fix_fft.h"
#include <driver/i2s.h>

// Deauth Hunter includes
#include "esp_wifi.h"
#include <vector>
#include <map>
#include <algorithm>

// PinAP Hunter structures
struct SSIDRecord {
  String essid;
  int32_t rssi;
  uint32_t last_seen;
  
  SSIDRecord(const String& ssid, int32_t signal) : essid(ssid), rssi(signal), last_seen(millis()) {}
};

struct PineRecord {
  uint8_t bssid[6];
  std::vector<SSIDRecord> essids;
  int32_t rssi;
  uint32_t last_seen;
  
  PineRecord() : rssi(-100), last_seen(0) {
    memset(bssid, 0, 6);
  }
};

struct PineAPHunterStats {
  std::vector<PineRecord> detected_pineaps;
  std::map<String, std::vector<SSIDRecord>> scan_buffer;
  uint32_t total_scans = 0;
  uint32_t last_scan_time = 0;
  uint32_t scan_cycle_start = 0;
  bool list_changed = false;
  int selected_bssid_index = 0;
  int view_mode = 0; // 0=main list, 1=BSSID details, 2=SSID list
};

// MP3 Player includes
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include "handlefile.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

static constexpr uint8_t m5spk_virtual_channel = 0;

// AudioOutputM5Speaker class from official M5Unified example
class AudioOutputM5Speaker : public AudioOutput
{
  public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0)
    {
      _m5sound = m5sound;
      _virtual_ch = virtual_sound_channel;
    }
    virtual ~AudioOutputM5Speaker(void) {};
    virtual bool begin(void) override { return true; }
    virtual bool ConsumeSample(int16_t sample[2]) override
    {
      if (_tri_buffer_index < tri_buf_size)
      {
        _tri_buffer[_tri_index][_tri_buffer_index  ] = sample[0];
        _tri_buffer[_tri_index][_tri_buffer_index+1] = sample[1];
        _tri_buffer_index += 2;

        return true;
      }

      flush();
      return false;
    }
    virtual void flush(void) override
    {
      if (_tri_buffer_index)
      {
        _m5sound->playRaw(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _virtual_ch);
        _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
        _tri_buffer_index = 0;
      }
    }
    virtual bool stop(void) override
    {
      flush();
      _m5sound->stop(_virtual_ch);
      return true;
    }

  protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    static constexpr size_t tri_buf_size = 1536;
    int16_t _tri_buffer[3][tri_buf_size];
    size_t _tri_buffer_index = 0;
    size_t _tri_index = 0;
};
bool isPaused = false;
String filetoplay;
char f[256];

// Color definitions
#ifndef GRAY
#define GRAY 0x8410
#endif

// Key definitions (use actual keys that work on M5Cardputer)
#define KEY_ESC 27
#define KEY_ENTER 0x28  // Use M5Cardputer's actual KEY_ENTER
#define KEY_UP 'w' 
#define KEY_DOWN 's'
#define KEY_LEFT ','
#define KEY_RIGHT ';'
#define KEY_FN 0xff     // M5Cardputer Fn key
#define KEY_OPT 0x00    // M5Cardputer Opt key

// Define PI as float
#ifndef PI
#define PI 3.14159265359f
#endif

// Application states
enum AppState {
  STATE_SCREENSAVER,
  STATE_MENU,
  STATE_MP3_PLAYER,
  STATE_WIFI_SCANNER,
  STATE_CAPTIVE_PORTAL,
  STATE_CLOCK_ALARM,
  STATE_PORTAL_HATER,
  STATE_TETRIS,
  STATE_TETRIS_PORTRAIT,
  STATE_TETROIDS,
  STATE_CALCULATOR,
  STATE_BEACON_SPAMMER
};

// Menu items
enum MenuItems {
  MENU_SCREENSAVER = 0,
  MENU_MP3_PLAYER,
  MENU_WIFI_SCANNER,
  MENU_CAPTIVE_PORTAL,
  MENU_CLOCK_ALARM,
  MENU_PORTAL_HATER,
  MENU_CUSTOM_PORTAL_HATER,  // New: Custom message portal hater
  MENU_DEAUTH_HUNTER,        // New: Deauth attack detector
  MENU_PINEAP_HUNTER,        // New: PineAP device detector
  MENU_JOKE_SCROLLER,        // New: Joke scroller from M5StickC repo
  MENU_AUDIO_VISUALIZER,     // New: Chromatic Cascade audio visualizer
  MENU_TETRIS,               // New: Tetris game (landscape)
  MENU_TETRIS_PORTRAIT,      // New: Tetris game (portrait)
  MENU_TETROIDS,             // New: Tetroids asteroids game
  MENU_CALCULATOR,           // New: Simple calculator
  MENU_BEACON_SPAMMER,       // New: Spamtastic beacon spammer
  MENU_COUNT
};

// Animation modes for screensaver
enum AnimationMode {
  BOXED = 0,
  SPIRAL,
  MATRIX,
  PLASMA,
  FIRE,
  STARS,
  TUNNEL,
  WAVE,
  LIFE,
  XJACK,
  CRITICAL,
  SPHERE,
  EPICYCLE,
  FLOW,
  GLMATRIX,
  XMATRIX,
  BINARYHORIZON,
  BINARYRING,
  SWIRL,
  BLASTER,
  BLOCKTUBE,
  BOUBOULE,
  ENERGYSTREAM,
  FADEPLOT,
  EXEC,
  FIREWORKX,
  FPS,
  FONTGLIDE,
  MODE_COUNT = 28
};

// Global state
AppState currentState = STATE_SCREENSAVER;
MenuItems selectedMenuItem = MENU_SCREENSAVER;
unsigned long lastActivityTime = 0;
unsigned long menuTimeout = 60000; // 1 minute
ESP32Time rtc(0);
Preferences preferences;

// Screensaver state
AnimationMode currentMode = BOXED;
float animationTime = 0;
unsigned long lastModeChange = 0;
const unsigned long AUTO_SCROLL_INTERVAL = 30000; // 30 seconds per effect
bool autoScrollEnabled = true; // Toggle for autoscroll
unsigned long frameCount = 0;

// Simple matrix rain for MP3 screensaver
#define MAX_MATRIX_DROPS 10
struct MatrixDrop {
  int x, y;
  int speed;
  char character;
  bool active;
};
MatrixDrop matrixDrops[MAX_MATRIX_DROPS];

// Portal Hater globals
bool portalHaterActive = false;
bool scanningForPortals = false;
int portalKillCount = 0;
unsigned long lastPortalScan = 0;
const unsigned long PORTAL_SCAN_INTERVAL = 30000; // 30 seconds
String lastAttackedPortal = "";

// Custom Portal Hater globals
String customLine1 = "";  // Start empty for user input
String customLine2 = "";  // Start empty for user input
bool customPortalHaterActive = false;
// Removed global HTTPClient to prevent memory issues

// Deauth Hunter globals
struct DeauthStats {
  uint32_t total_deauths = 0;
  uint32_t unique_aps = 0;
  int32_t rssi_sum = 0;
  uint32_t rssi_count = 0;
  int32_t avg_rssi = -90;
  uint32_t last_reset_time = 0;
};

// PinAP Hunter globals
PineAPHunterStats pineap_hunter_stats;
bool pineap_hunter_active = false;
uint32_t last_pineap_scan = 0;
const uint32_t PINEAP_SCAN_INTERVAL = 3000; // 3 seconds
int ph_alert_ssids = 3; // Alert threshold for SSIDs per BSSID
int pineap_cursor = 0; // Local cursor for PinAP Hunter navigation

// WiFi frame structure definitions
typedef struct {
  int16_t fctl;
  int16_t duration;
  uint8_t da[6];
  uint8_t sa[6];
  uint8_t bssid[6];
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

typedef struct {
  uint8_t payload[0];
  WifiMgmtHdr hdr;
} wifi_ieee80211_packet_t;

// Channel hopping configuration
const uint8_t WIFI_CHANNELS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const uint8_t NUM_CHANNELS = sizeof(WIFI_CHANNELS) / sizeof(WIFI_CHANNELS[0]);

// Deauth Hunter global state
DeauthStats deauth_stats;
uint8_t current_channel_idx = 0;
uint32_t last_channel_change = 0;
uint32_t scan_cycle_start = 0;
bool deauth_hunter_active = false;
std::vector<String> seen_ap_macs;

// Menu item names
const char* menuItems[MENU_COUNT] = {
  "1. Screensaver",
  "2. MP3 Player",
  "3. WiFi Scanner", 
  "4. Matrix Portal",
  "5. Clock & Alarm",
  "6. Portal Hater",
  "7. Custom Portal Hater",
  "8. Deauth Hunter",
  "9. PinAP Hunter",
  "10. Joke Scroller",
  "11. Audio Visualizer",
  "12. Tetris Game",
  "13. Tetris Portrait",
  "14. Tetroids Game",
  "15. Calculator",
  "16. Beacon Spammer"
};

// Function declarations
void enterMenu();
void enterScreensaver();
void displayMenu();
void handleMenu();
void selectMenuItem();
void displayMessage(const char* title, const char* message);
void enterMP3Player();
void enterJokeScroller(); // New: Joke scroller function
void enterAudioVisualizer(); // New: Audio visualizer function
bool checkEscapeKey();

// Screensaver functions
void initScreensaver();
void handleScreensaver();
void drawCurrentEffect();
void showEffectInfo();
void drawBoxed();
void drawSpiral();
void drawMatrix();
void drawPlasma();
void drawFire();
void drawStars();
void drawTunnel();
void drawWave();
void drawLife();
void drawXjack();

// Simple matrix rain functions for MP3 screensaver
void initSimpleMatrix();
void drawSimpleMatrix();
void drawCritical();
void drawSphere();
void drawEpicycle();
void drawFlow();
void drawGlmatrix();
void drawXmatrix();
void drawBinaryhorizon();
void drawBinaryring();
void drawSwirl();
void drawBlaster();
void drawBlocktube();
void drawBouboule();
void drawEnergystream();
void drawFadeplot();
void drawExec();
void drawFireworkx();
void drawFps();
void drawFontglide();

// ===== TETRIS GAME CLASS =====
#define TETRIS_BLOCK_SIZE 8
#define TETRIS_FIELD_WIDTH 10
#define TETRIS_FIELD_HEIGHT 16  // Maximized height - use full screen
#define TETRIS_OFFSET_X 5       // Start at very left
#define TETRIS_OFFSET_Y 5       // Start at very top

class TetrisGame {
private:
  uint8_t field[TETRIS_FIELD_HEIGHT][TETRIS_FIELD_WIDTH];
  int currentPiece;
  int currentRot;
  int posX, posY;
  int score;
  int level;
  unsigned long lastDropTime;
  unsigned long lockDelayStart;
  bool lockDelayActive;
  int dropSpeed;
  bool gameOver;
  
  // Modern features
  int heldPiece;
  int nextPiece;
  bool canHold;
  int linesCleared;
  
  int pieces[7][4][2][4];
  uint16_t pieceColors[7];
  
  bool test(int y, int x, int piece, int rot);
  void placePiece();
  void clearLines();
  void newPiece(bool setPiece);
  void drawGhostPiece();
  void drawHoldPiece();
  void drawNextPiece();
  void drawMiniPiece(int pieceType, int x, int y, int scale);
  int calculateDropDistance();
  void holdPiece();
  void handleTetrisInput();
  
public:
  void init();
  void update();
  void draw();
  bool isGameOver() { return gameOver; }
  int getScore() { return score; }
  void reset() { gameOver = false; init(); }
};

TetrisGame tetrisGame;

// ===== PORTRAIT TETRIS GAME CLASS =====
#define TETRIS_P_BLOCK_SIZE 8       // Back to 8 like landscape
#define TETRIS_P_FIELD_WIDTH 10     // Same width as landscape  
#define TETRIS_P_FIELD_HEIGHT 28    // Maximized height for portrait (28*8 = 224px fits in 240px)
#define TETRIS_P_OFFSET_X 5         // Start at left edge
#define TETRIS_P_OFFSET_Y 5         // Start at top edge

class TetrisPortraitGame {
private:
  uint8_t field[TETRIS_P_FIELD_HEIGHT][TETRIS_P_FIELD_WIDTH];
  int currentPiece;
  int currentRot;
  int posX, posY;
  int score;
  int level;
  unsigned long lastDropTime;
  unsigned long lockDelayStart;
  bool lockDelayActive;
  int dropSpeed;
  bool gameOver;
  
  // Modern features
  int heldPiece;
  int nextPiece;
  bool canHold;
  int linesCleared;
  
  int pieces[7][4][2][4];
  uint16_t pieceColors[7];
  
  bool test(int y, int x, int piece, int rot);
  void placePiece();
  void clearLines();
  void newPiece(bool setPiece);
  void drawGhostPiece();
  void drawHoldPiece();
  void drawNextPiece();
  void drawMiniPiece(int pieceType, int x, int y, int scale);
  int calculateDropDistance();
  void holdPiece();
  void handleTetrisPortraitInput();
  
public:
  void init();
  void update();
  void draw();
  bool isGameOver() { return gameOver; }
  int getScore() { return score; }
  void reset() { gameOver = false; init(); }
};

TetrisPortraitGame tetrisPortraitGame;

// Tetris function declarations
void initTetris();
void handleTetris();
void initTetrisPortrait();
void handleTetrisPortrait();

// Tetroids function declarations  
void initTetroids();
void handleTetroids();

// Calculator function declarations
void initCalculator();
void handleCalculator();

// Beacon Spammer function declarations
void initBeaconSpammer();
void handleBeaconSpammer();

// WiFi Network structure for enhanced scanner
struct WiFiNetwork {
  String ssid;
  int rssi;
  wifi_auth_mode_t encryption;
  bool open = false;
  bool webAccess = false;  
  bool vulnerable = false;
  String bssid;
};

// Enhanced WiFi Scanner functions
void initAdvancedWiFiScanner();
void handleAdvancedWiFiScanner();
std::vector<WiFiNetwork> getWiFiNetworks();
std::vector<WiFiNetwork> getOpenWiFiNetworks(std::vector<WiFiNetwork>& networks);
std::vector<WiFiNetwork> getClosedWiFiNetworks(const std::vector<WiFiNetwork>& networks);
std::vector<WiFiNetwork> getVulnerableWiFiNetworks(std::vector<WiFiNetwork>& networks);
std::vector<WiFiNetwork> getValidatedOpenWiFiNetworks(std::vector<WiFiNetwork>& networks);
bool testOpenWiFiConnection(const WiFiNetwork& network);
String encryptionTypeToString(wifi_auth_mode_t encryption);
void displayAdvancedWiFiResults(std::vector<WiFiNetwork>& networks, int scrollOffset);
void displayWiFiCounts(std::vector<WiFiNetwork>& networks);

// Application functions
void initMP3Player();
void handleMP3Player();
void initWiFiScanner();
void handleWiFiScanner();
void displayWiFiResults(int networkCount);
void initCaptivePortal();
void handleCaptivePortal();
void handleMatrixRoot();
void handleMatrixSubmit();
void initClockAlarm();
void handleClockAlarm();
void handleClockSettings();
void initPortalHater();
void handlePortalHater();

// Portal Hater functions
void scanForEvilPortals();
void attackPortal(String ssid);

// Custom Portal Hater functions  
void enterCustomPortalHater();
void scanForEvilPortalsCustom();
void attackPortalCustom(String ssid);
String getTextInput(String prompt, String currentText, int maxLength = 50);

// Deauth Hunter functions
void initDeauthHunter();
void handleDeauthHunter();
void enterDeauthHunter();
void deauth_hunter_setup();
void deauth_hunter_loop();
void deauth_hunter_cleanup();
void start_deauth_monitoring();
void stop_deauth_monitoring();
void hop_channel();
void reset_stats_if_needed();
void add_unique_ap(const char* mac);
int calculate_rssi_bars(int rssi);
void updateDeauthDisplay();
static void IRAM_ATTR deauth_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type);

// PinAP Hunter functions
void enterPinAPHunter();
void handlePinAPHunter();
void pineap_hunter_setup();
void pineap_hunter_loop();
void pineap_hunter_cleanup();
void pineap_scan_and_analyze();
void pineap_process_scan_results();
void pineap_add_scan_result(const String& bssid_str, const String& essid, int32_t rssi);
void pineap_maintain_buffer_size();
void pineap_update_display();
void pineap_draw_main_list();
void pineap_draw_ssid_list();
String pineap_bssid_to_string(const uint8_t* bssid);
void pineap_string_to_bssid(const String& bssid_str, uint8_t* bssid);
static void extract_mac(char *addr, uint8_t* data, uint16_t offset);

void setup() {
  Serial.begin(115200);
  Serial.println("XtraCardputer - Multi-Function Pocket Computer");
  
  // Initialize M5Cardputer with speaker config
  auto cfg = M5.config();
  cfg.external_speaker.hat_spk = true;  // Enable HAT speaker
  M5Cardputer.begin(cfg);
  
  // Configure speaker for better audio quality
  auto spk_cfg = M5Cardputer.Speaker.config();
  spk_cfg.sample_rate = 128000;  // High sample rate for quality (original uses 128000)
  spk_cfg.task_pinned_core = APP_CPU_NUM;
  M5Cardputer.Speaker.config(spk_cfg);
  
  M5Cardputer.Display.setRotation(1); // Landscape mode
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Set initial speaker volume (0-255 range)
  M5Cardputer.Speaker.setVolume(128);  // Start at 50% volume
  
  // REMOVED: WiFi initialization moved to lazy/on-demand only
  // WiFi.mode(WIFI_STA);     // REMOVED - was causing M5Burner to classify as firmware  
  // WiFi.disconnect();       // REMOVED - was causing M5Burner to classify as firmware
  
  // Set initial time
  rtc.setTime(0, 0, 12, 20, 12, 2025); // sec, min, hour, day, month, year
  
  // Initialize screensaver
  initScreensaver();
  
  // Start in screensaver mode
  currentState = STATE_SCREENSAVER;
  lastActivityTime = millis();
  
  Serial.printf("Display size: %dx%d\n", M5Cardputer.Display.width(), M5Cardputer.Display.height());
  Serial.println("Press ESC to access menu");
}

void loop() {
  M5Cardputer.update();
  
  // Check for menu key press (try multiple options)
  if (M5Cardputer.Keyboard.isPressed()) {
    // Try ESC, Fn key, or BtnA (physical button)
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ESC) || 
        M5Cardputer.Keyboard.isKeyPressed(KEY_FN) ||
        M5Cardputer.Keyboard.isKeyPressed('m')) {
      if (currentState != STATE_MENU) {
        enterMenu();
      } else {
        // ESC from menu goes back to screensaver
        enterScreensaver();
      }
      delay(200); // Debounce
    }
  }
  
  // Also check physical BtnA button
  if (M5Cardputer.BtnA.wasPressed()) {
    if (currentState != STATE_MENU) {
      enterMenu();
    } else {
      enterScreensaver();
    }
  }
  
  // Handle current state
  switch (currentState) {
    case STATE_SCREENSAVER:
      handleScreensaver();
      break;
      
    case STATE_MENU:
      handleMenu();
      break;
      
    case STATE_MP3_PLAYER:
      handleMP3Player();
      break;
      
    case STATE_WIFI_SCANNER:
      handleAdvancedWiFiScanner();
      break;
      
    case STATE_CAPTIVE_PORTAL:
      handleCaptivePortal();
      break;
      
    case STATE_CLOCK_ALARM:
      handleClockAlarm();
      break;
      
    case STATE_PORTAL_HATER:
      handlePortalHater();
      break;
      
    case STATE_TETRIS:
      handleTetris();
      break;
      
    case STATE_TETRIS_PORTRAIT:
      handleTetrisPortrait();
      break;
      
    case STATE_TETROIDS:
      handleTetroids();
      break;
      
    case STATE_CALCULATOR:
      handleCalculator();
      break;
      
    case STATE_BEACON_SPAMMER:
      handleBeaconSpammer();
      break;
  }
  
  // Auto-return to screensaver after timeout
  if (currentState == STATE_MENU && (millis() - lastActivityTime > menuTimeout)) {
    enterScreensaver();
  }
}

void enterMenu() {
  // Always reset to landscape mode when entering menu
  M5Cardputer.Display.setRotation(1);
  currentState = STATE_MENU;
  lastActivityTime = millis();
  selectedMenuItem = MENU_SCREENSAVER;
  displayMenu();
  Serial.println("Entered menu");
}

void enterScreensaver() {
  // Always reset to landscape mode when entering screensaver
  M5Cardputer.Display.setRotation(1);
  currentState = STATE_SCREENSAVER;
  lastActivityTime = millis();
  Serial.println("Returned to screensaver");
}

void displayMenu() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);
  
  // Title
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.printf("XtraCardputer");
  
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(10, 35);
  M5Cardputer.Display.printf("Multi-Function Pocket Computer");
  
  // Menu items with scrolling support
  int maxVisible = 5;  // Show max 5 items at once
  int startIndex = 0;
  
  // Calculate scroll position
  if (selectedMenuItem >= maxVisible) {
    startIndex = selectedMenuItem - maxVisible + 1;
  }
  
  int endIndex = startIndex + maxVisible;
  if (endIndex > (int)MENU_COUNT) {
    endIndex = (int)MENU_COUNT;
  }
  
  for (int i = startIndex; i < endIndex; i++) {
    int displayIndex = i - startIndex;  // Position on screen (0-4)
    int y = 55 + displayIndex * 15;
    
    if (i == selectedMenuItem) {
      // Highlight selected item
      M5Cardputer.Display.fillRect(5, y - 2, M5Cardputer.Display.width() - 10, 12, BLUE);
      M5Cardputer.Display.setTextColor(WHITE);
    } else {
      M5Cardputer.Display.setTextColor(GREEN);
    }
    
    M5Cardputer.Display.setCursor(10, y);
    M5Cardputer.Display.printf("%s", menuItems[i]);
  }
  
  // Show scroll indicators
  if (startIndex > 0) {
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(220, 55);
    M5Cardputer.Display.printf("^");
  }
  
  if (endIndex < MENU_COUNT) {
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(220, 125);
    M5Cardputer.Display.printf("v");
  }
  
  // Instructions removed for cleaner interface
}

void handleMenu() {
  static unsigned long lastKeyPress = 0;
  bool keyPressed = false;
  
  if (M5Cardputer.Keyboard.isPressed() && (millis() - lastKeyPress > 150)) {
    
    // Arrow key navigation
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_DOWN) || 
        M5Cardputer.Keyboard.isKeyPressed('s') ||
        M5Cardputer.Keyboard.isKeyPressed('.') ||
        M5Cardputer.Keyboard.isKeyPressed(0xB5)) {  // Down arrow key
      selectedMenuItem = (MenuItems)((selectedMenuItem + 1) % MENU_COUNT);
      keyPressed = true;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed(KEY_UP) ||
             M5Cardputer.Keyboard.isKeyPressed('w') ||
             M5Cardputer.Keyboard.isKeyPressed(';') ||
             M5Cardputer.Keyboard.isKeyPressed(0xB4)) {  // Up arrow key
      selectedMenuItem = (MenuItems)((selectedMenuItem + MENU_COUNT - 1) % MENU_COUNT);
      keyPressed = true;
    }
    
    // Number key selection (1-7)
    for (int i = 0; i < MENU_COUNT && i < 7; i++) {
      if (M5Cardputer.Keyboard.isKeyPressed('1' + i)) {
        selectedMenuItem = (MenuItems)i;
        keyPressed = true;
        // Auto-select after number press
        selectMenuItem();
        return;
      }
    }
    
    // Enter key selection
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) ||
        M5Cardputer.Keyboard.isKeyPressed(' ')) {
      selectMenuItem();
      return;
    }
    
    if (keyPressed) {
      lastKeyPress = millis();
      lastActivityTime = millis();
      displayMenu();
    }
  }
}

void selectMenuItem() {
  Serial.printf("Selected menu item: %d\n", selectedMenuItem);
  
  switch (selectedMenuItem) {
    case MENU_SCREENSAVER:
      enterScreensaver();
      break;
      
    case MENU_MP3_PLAYER:
      currentState = STATE_MP3_PLAYER;
      initMP3Player();
      break;
      
    case MENU_WIFI_SCANNER:
      currentState = STATE_WIFI_SCANNER;
      initAdvancedWiFiScanner();
      break;
      
    case MENU_CAPTIVE_PORTAL:
      currentState = STATE_CAPTIVE_PORTAL;
      initCaptivePortal();
      break;
      
    case MENU_CLOCK_ALARM:
      currentState = STATE_CLOCK_ALARM;
      initClockAlarm();
      break;
      
    case MENU_PORTAL_HATER:
      currentState = STATE_PORTAL_HATER;
      initPortalHater();
      break;
      
    case MENU_CUSTOM_PORTAL_HATER:  // New: Custom message portal hater
      enterCustomPortalHater();
      break;
      
    case MENU_DEAUTH_HUNTER:  // New: Deauth attack detector
      enterDeauthHunter();
      break;
      
    case MENU_PINEAP_HUNTER:  // New: PineAP device detector  
      enterPinAPHunter();
      break;
      
    case MENU_JOKE_SCROLLER:  // New: Joke scroller
      enterJokeScroller();
      break;
      
    case MENU_AUDIO_VISUALIZER:  // New: Audio visualizer
      enterAudioVisualizer();
      break;
      
    case MENU_TETRIS:  // New: Tetris game
      currentState = STATE_TETRIS;
      initTetris();
      break;
      
    case MENU_TETRIS_PORTRAIT:  // New: Portrait Tetris game
      currentState = STATE_TETRIS_PORTRAIT;
      initTetrisPortrait();
      break;
      
    case MENU_TETROIDS:  // New: Tetroids asteroids game
      currentState = STATE_TETROIDS;
      initTetroids();
      break;
      
    case MENU_CALCULATOR:  // New: Simple calculator
      currentState = STATE_CALCULATOR;
      initCalculator();
      break;
      
    case MENU_BEACON_SPAMMER:  // New: Spamtastic beacon spammer
      currentState = STATE_BEACON_SPAMMER;
      initBeaconSpammer();
      break;
  }
}

// Utility functions
bool checkEscapeKey() {
  // Check multiple ways to exit: ESC, Fn, M key, or physical button
  if (M5Cardputer.Keyboard.isPressed()) {
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ESC) || 
        M5Cardputer.Keyboard.isKeyPressed(KEY_FN) ||
        M5Cardputer.Keyboard.isKeyPressed('m')) {
      delay(200); // Debounce
      return true;
    }
  }
  
  // Also check physical BtnA
  if (M5Cardputer.BtnA.wasPressed()) {
    return true;
  }
  
  return false;
}

void displayMessage(const char* title, const char* message) {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("%s", title);
  
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(10, 40);
  M5Cardputer.Display.printf("%s", message);
  
  M5Cardputer.Display.setTextColor(GRAY);
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 15);
  M5Cardputer.Display.printf("Press M/Fn/BtnA to return to menu");
}

// ===== SCREENSAVER IMPLEMENTATION =====

void initScreensaver() {
  currentMode = BOXED;
  animationTime = 0;
  lastModeChange = millis();
  frameCount = 0;
  Serial.println("Screensaver initialized");
}

void handleScreensaver() {
  // Update animation time
  animationTime += 0.016f; // Assuming ~60 FPS
  frameCount++;
  
  // Auto-cycle through effects (only if enabled)
  if (autoScrollEnabled && millis() - lastModeChange > AUTO_SCROLL_INTERVAL) {
    currentMode = (AnimationMode)((currentMode + 1) % MODE_COUNT);
    lastModeChange = millis();
    animationTime = 0; // Reset animation time for new effect
    Serial.printf("Switched to effect %d\n", currentMode);
  }
  
  // Manual effect switching with keyboard (if not in menu)
  if (M5Cardputer.Keyboard.isPressed()) {
    if (M5Cardputer.Keyboard.isKeyPressed('g') || M5Cardputer.Keyboard.isKeyPressed(';')) {
      currentMode = (AnimationMode)((currentMode + 1) % MODE_COUNT);
      lastModeChange = millis();
      animationTime = 0;
      delay(200); // Debounce
    }
    else if (M5Cardputer.Keyboard.isKeyPressed(',')) {
      currentMode = (AnimationMode)((currentMode + MODE_COUNT - 1) % MODE_COUNT);
      lastModeChange = millis();
      animationTime = 0;
      delay(200); // Debounce
    }
    else if (M5Cardputer.Keyboard.isKeyPressed(' ')) {
      // Toggle autoscroll with spacebar
      autoScrollEnabled = !autoScrollEnabled;
      Serial.printf("Autoscroll %s\n", autoScrollEnabled ? "enabled" : "disabled");
      delay(200); // Debounce
    }
  }
  
  // Draw current effect
  drawCurrentEffect();
  
  // Show effect info occasionally
  if (frameCount % 300 == 0) { // Every 5 seconds at 60fps
    showEffectInfo();
  }
}

void drawCurrentEffect() {
  switch (currentMode) {
    case BOXED:
      drawBoxed();
      break;
    case SPIRAL:
      drawSpiral();
      break;
    case MATRIX:
      drawMatrix();
      break;
    case PLASMA:
      drawPlasma();
      break;
    case FIRE:
      drawFire();
      break;
    case STARS:
      drawStars();
      break;
    case TUNNEL:
      drawTunnel();
      break;
    case WAVE:
      drawWave();
      break;
    case LIFE:
      drawLife();
      break;
    case XJACK:
      drawXjack();
      break;
    case CRITICAL:
      drawCritical();
      break;
    case SPHERE:
      drawSphere();
      break;
    case EPICYCLE:
      drawEpicycle();
      break;
    case FLOW:
      drawFlow();
      break;
    case GLMATRIX:
      drawGlmatrix();
      break;
    case XMATRIX:
      drawXmatrix();
      break;
    case BINARYHORIZON:
      drawBinaryhorizon();
      break;
    case BINARYRING:
      drawBinaryring();
      break;
    case SWIRL:
      drawSwirl();
      break;
    case BLASTER:
      drawBlaster();
      break;
    case BLOCKTUBE:
      drawBlocktube();
      break;
    case BOUBOULE:
      drawBouboule();
      break;
    case ENERGYSTREAM:
      drawEnergystream();
      break;
    case FADEPLOT:
      drawFadeplot();
      break;
    case EXEC:
      drawExec();
      break;
    case FIREWORKX:
      drawFireworkx();
      break;
    case FPS:
      drawFps();
      break;
    case FONTGLIDE:
      drawFontglide();
      break;
  }
}

void showEffectInfo() {
  const char* effectNames[] = {
    "BOXED", "SPIRAL", "MATRIX", "PLASMA", "FIRE",
    "STARS", "TUNNEL", "WAVE", "LIFE", "XJACK",
    "CRITICAL", "SPHERE", "EPICYCLE", "FLOW", "GLMATRIX",
    "XMATRIX", "BINARYHORIZON", "BINARYRING", "SWIRL", "BLASTER",
    "BLOCKTUBE", "BOUBOULE", "ENERGYSTREAM", "FADEPLOT", "EXEC",
    "FIREWORKX", "FPS", "FONTGLIDE"
  };
  
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(5, 5);
  M5Cardputer.Display.printf("%s (%d/%d)", effectNames[currentMode], currentMode + 1, MODE_COUNT);
  
  // Show autoscroll status
  M5Cardputer.Display.setCursor(5, 15);
  M5Cardputer.Display.setTextColor(autoScrollEnabled ? GREEN : RED);
  M5Cardputer.Display.printf("Auto: %s", autoScrollEnabled ? "ON" : "OFF");
}

// Effect implementations (simplified versions for memory efficiency)

void drawBoxed() {
  M5Cardputer.Display.fillScreen(BLACK);
  for (int i = 0; i < 6; i++) {
    int x = (int)(sin(animationTime * 2 + i * 0.8) * 100 + M5Cardputer.Display.width() / 2);
    int y = (int)(cos(animationTime * 1.5 + i * 0.6) * 60 + M5Cardputer.Display.height() / 2);
    int size = (int)(sin(animationTime * 3 + i) * 8 + 15);
    
    float phase = animationTime + i * 0.7;
    int red = (int)(sin(phase) * 127 + 128);
    int green = (int)(cos(phase + 1) * 127 + 128);  
    int blue = (int)(sin(phase + 2) * 127 + 128);
    
    if (x > size && y > size && x < M5Cardputer.Display.width()-size && y < M5Cardputer.Display.height()-size) {
      M5Cardputer.Display.fillRect(x-size, y-size, size*2, size*2, M5Cardputer.Display.color565(red, green, blue));
      M5Cardputer.Display.drawRect(x-size, y-size, size*2, size*2, WHITE);
    }
  }
}

void drawSpiral() {
  M5Cardputer.Display.fillScreen(BLACK);
  int centerX = M5Cardputer.Display.width() / 2;
  int centerY = M5Cardputer.Display.height() / 2;
  
  for (int i = 0; i < 100; i++) {
    float angle = animationTime + i * 0.2;
    float radius = i * 0.8;
    
    int x = centerX + cos(angle) * radius;
    int y = centerY + sin(angle) * radius;
    
    if (x >= 0 && x < M5Cardputer.Display.width() && y >= 0 && y < M5Cardputer.Display.height()) {
      uint16_t color = M5Cardputer.Display.color565(
        (int)(sin(angle) * 127 + 128),
        (int)(cos(angle + 2) * 127 + 128),
        (int)(sin(angle + 4) * 127 + 128)
      );
      M5Cardputer.Display.fillCircle(x, y, 2, color);
    }
  }
}

void drawMatrix() {
  static uint8_t matrix[30][20];
  static bool initialized = false;
  
  if (!initialized) {
    memset(matrix, 0, sizeof(matrix));
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Update matrix
  if ((int)(animationTime * 10) % 5 == 0) {
    for (int x = 0; x < 30; x++) {
      for (int y = 19; y > 0; y--) {
        matrix[x][y] = matrix[x][y-1];
      }
      matrix[x][0] = random(0, 3) == 0 ? random(33, 127) : 0;
    }
  }
  
  // Draw matrix
  M5Cardputer.Display.setTextSize(1);
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 20; y++) {
      if (matrix[x][y] != 0) {
        uint16_t color = M5Cardputer.Display.color565(0, 255 - y * 12, 0);
        M5Cardputer.Display.setTextColor(color);
        M5Cardputer.Display.setCursor(x * 8, y * 7);
        M5Cardputer.Display.printf("%c", matrix[x][y]);
      }
    }
  }
}

void drawPlasma() {
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int x = 0; x < M5Cardputer.Display.width(); x += 2) {
    for (int y = 0; y < M5Cardputer.Display.height(); y += 2) {
      float value = sin(x * 0.1 + animationTime) + sin(y * 0.1 + animationTime * 1.3) + 
                    sin((x + y) * 0.05 + animationTime * 0.7);
      
      int red = (int)(sin(value + animationTime) * 127 + 128);
      int green = (int)(sin(value + animationTime + 2) * 127 + 128);
      int blue = (int)(sin(value + animationTime + 4) * 127 + 128);
      
      M5Cardputer.Display.fillRect(x, y, 2, 2, M5Cardputer.Display.color565(red, green, blue));
    }
  }
}

void drawFire() {
  static uint8_t fire[60][34]; // Reduced size for memory
  static bool initialized = false;
  
  if (!initialized) {
    memset(fire, 0, sizeof(fire));
    initialized = true;
  }
  
  // Generate fire bottom line
  for (int x = 0; x < 60; x++) {
    fire[x][33] = random(200, 255);
  }
  
  // Propagate fire upward
  for (int y = 32; y >= 0; y--) {
    for (int x = 1; x < 59; x++) {
      int avg = (fire[x-1][y+1] + fire[x][y+1] + fire[x+1][y+1]) / 3;
      fire[x][y] = (avg > random(0, 3)) ? (avg - random(0, 3)) : 0;
    }
  }
  
  // Draw fire (scaled up)
  M5Cardputer.Display.fillScreen(BLACK);
  for (int x = 0; x < 60; x++) {
    for (int y = 0; y < 34; y++) {
      uint8_t intensity = fire[x][y];
      if (intensity > 0) {
        uint16_t color;
        if (intensity > 200) {
          color = M5Cardputer.Display.color565(255, 255, intensity - 200);
        } else if (intensity > 100) {
          color = M5Cardputer.Display.color565(255, intensity * 2 - 200, 0);
        } else {
          color = M5Cardputer.Display.color565(intensity * 2, 0, 0);
        }
        M5Cardputer.Display.fillRect(x * 4, y * 4, 4, 4, color);
      }
    }
  }
}

void drawStars() {
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int i = 0; i < 100; i++) {
    float starTime = animationTime + i * 0.1;
    int x = (int)(sin(starTime * 0.3 + i) * 120 + M5Cardputer.Display.width() / 2);
    int y = (int)(cos(starTime * 0.4 + i * 1.3) * 60 + M5Cardputer.Display.height() / 2);
    
    if (x >= 0 && x < M5Cardputer.Display.width() && y >= 0 && y < M5Cardputer.Display.height()) {
      float twinkle = sin(starTime * 8 + i) * 0.5 + 0.5;
      uint8_t brightness = (uint8_t)(twinkle * 255);
      M5Cardputer.Display.drawPixel(x, y, M5Cardputer.Display.color565(brightness, brightness, brightness));
    }
  }
}

void drawTunnel() {
  M5Cardputer.Display.fillScreen(BLACK);
  int centerX = M5Cardputer.Display.width() / 2;
  int centerY = M5Cardputer.Display.height() / 2;
  
  for (int ring = 0; ring < 8; ring++) {
    float ringTime = animationTime + ring * 0.3;
    int radius = 20 + ring * 15 + (int)(sin(ringTime * 2) * 10);
    
    uint16_t color = M5Cardputer.Display.color565(
      255 - ring * 30,
      100 + ring * 15,
      200 - ring * 20
    );
    
    M5Cardputer.Display.drawCircle(centerX, centerY, radius, color);
  }
}

void drawWave() {
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int x = 0; x < M5Cardputer.Display.width(); x += 2) {
    int y1 = M5Cardputer.Display.height() / 2 + sin(x * 0.1 + animationTime * 2) * 30;
    int y2 = M5Cardputer.Display.height() / 2 + sin(x * 0.05 + animationTime * 1.5 + PI/2) * 20;
    
    uint16_t color1 = M5Cardputer.Display.color565(255, 100, 100);
    uint16_t color2 = M5Cardputer.Display.color565(100, 255, 100);
    
    if (y1 >= 0 && y1 < M5Cardputer.Display.height()) M5Cardputer.Display.drawPixel(x, y1, color1);
    if (y2 >= 0 && y2 < M5Cardputer.Display.height()) M5Cardputer.Display.drawPixel(x, y2, color2);
  }
}

void drawLife() {
  static bool grid[40][23];
  static bool newGrid[40][23];
  static bool initialized = false;
  static int generation = 0;
  
  if (!initialized || generation > 50) {
    for (int x = 0; x < 40; x++) {
      for (int y = 0; y < 23; y++) {
        grid[x][y] = random(0, 4) == 0;
      }
    }
    initialized = true;
    generation = 0;
  }
  
  if ((int)(animationTime * 5) % 10 == 0) {
    for (int x = 0; x < 40; x++) {
      for (int y = 0; y < 23; y++) {
        int neighbors = 0;
        for (int dx = -1; dx <= 1; dx++) {
          for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = (x + dx + 40) % 40;
            int ny = (y + dy + 23) % 23;
            if (grid[nx][ny]) neighbors++;
          }
        }
        
        if (grid[x][y]) {
          newGrid[x][y] = (neighbors == 2 || neighbors == 3);
        } else {
          newGrid[x][y] = (neighbors == 3);
        }
      }
    }
    
    memcpy(grid, newGrid, sizeof(grid));
    generation++;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  for (int x = 0; x < 40; x++) {
    for (int y = 0; y < 23; y++) {
      if (grid[x][y]) {
        M5Cardputer.Display.fillRect(x * 6, y * 6, 5, 5, GREEN);
      }
    }
  }
}

// ===== APPLICATION IMPLEMENTATIONS =====

// MP3 Player Application
void initMP3Player() {
  Serial.println("MP3 Player initialized");
  enterMP3Player();
}

void handleMP3Player() {
  if (checkEscapeKey()) {
    enterMenu();
    return;
  }
  // MP3 player logic will go here
}

// WiFi Scanner Application
void initAdvancedWiFiScanner() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("Advanced WiFi");
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.printf("Scanner");
  
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(10, 55);
  M5Cardputer.Display.printf("Analyzing networks...");
  
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setCursor(10, 110);
  M5Cardputer.Display.printf("R=Rescan  Fn=Exit");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

void handleAdvancedWiFiScanner() {
  static bool scanning = false;
  static bool scanComplete = false;
  static std::vector<WiFiNetwork> allNetworks;
  static std::vector<WiFiNetwork> finalNetworks;
  static unsigned long lastScan = 0;
  static int scrollOffset = 0;
  static bool networksInitialized = false;
  
  if (checkEscapeKey()) {
    enterMenu();
    return;
  }
  
  // Start initial scan or rescan only when requested
  if ((!networksInitialized || (millis() - lastScan > 15000)) && !scanning) {
    scanning = true;
    scanComplete = false;
    lastScan = millis();
    
    M5Cardputer.Display.fillRect(10, 55, 220, 60, BLACK);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(10, 55);
    M5Cardputer.Display.printf("Scanning networks...");
    M5Cardputer.Display.display();
    
    // Get networks and analyze them
    allNetworks = getWiFiNetworks();
    
    if (!allNetworks.empty()) {
      // Analyze network security and connectivity
      std::vector<WiFiNetwork> openNetworks = getOpenWiFiNetworks(allNetworks);
      std::vector<WiFiNetwork> closedNetworks = getClosedWiFiNetworks(allNetworks);
      std::vector<WiFiNetwork> vulnerableNetworks = getVulnerableWiFiNetworks(allNetworks);
      
      M5Cardputer.Display.setTextColor(CYAN);
      M5Cardputer.Display.setCursor(10, 70);
      M5Cardputer.Display.printf("Testing connectivity...");
      M5Cardputer.Display.display();
      
      std::vector<WiFiNetwork> webAccessNetworks = getValidatedOpenWiFiNetworks(openNetworks);
      
      // Merge and order: Web access first, then open, vulnerable, closed
      finalNetworks.clear();
      finalNetworks.insert(finalNetworks.end(), webAccessNetworks.begin(), webAccessNetworks.end());
      
      // Remove web access networks from open networks to avoid duplicates
      openNetworks.erase(std::remove_if(openNetworks.begin(), openNetworks.end(),
          [&webAccessNetworks](const WiFiNetwork& network) {
              return std::any_of(webAccessNetworks.begin(), webAccessNetworks.end(),
                  [&network](const WiFiNetwork& webNetwork) {
                      return webNetwork.ssid == network.ssid;
                  });
          }), openNetworks.end());
      
      finalNetworks.insert(finalNetworks.end(), openNetworks.begin(), openNetworks.end());
      finalNetworks.insert(finalNetworks.end(), vulnerableNetworks.begin(), vulnerableNetworks.end());
      
      // Remove vulnerable networks from closed to avoid duplicates
      closedNetworks.erase(std::remove_if(closedNetworks.begin(), closedNetworks.end(),
          [&vulnerableNetworks](const WiFiNetwork& network) {
              return std::any_of(vulnerableNetworks.begin(), vulnerableNetworks.end(),
                  [&network](const WiFiNetwork& vulnNetwork) {
                      return vulnNetwork.ssid == network.ssid;
                  });
          }), closedNetworks.end());
      
      finalNetworks.insert(finalNetworks.end(), closedNetworks.begin(), closedNetworks.end());
    }
    
    scanning = false;
    scanComplete = true;
    networksInitialized = true;
    scrollOffset = 0; // Reset scroll when refreshing
    displayAdvancedWiFiResults(finalNetworks, scrollOffset);
  }
  
  // Handle scrolling and controls
  if (M5Cardputer.Keyboard.isChange()) {
    // Scroll up
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_UP) || M5Cardputer.Keyboard.isKeyPressed('w')) {
      if (scrollOffset > 0) {
        scrollOffset--;
        displayAdvancedWiFiResults(finalNetworks, scrollOffset);
        delay(200);
      }
    }
    
    // Scroll down
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_DOWN) || M5Cardputer.Keyboard.isKeyPressed('s')) {
      if (scrollOffset < (int)finalNetworks.size() - 4) {
        scrollOffset++;
        displayAdvancedWiFiResults(finalNetworks, scrollOffset);
        delay(200);
      }
    }
    
    // Handle rescan
    if (M5Cardputer.Keyboard.isKeyPressed('r')) {
      lastScan = 0; // Force rescan
      networksInitialized = false;
      delay(200);
    }
  }
}

std::vector<WiFiNetwork> getWiFiNetworks() {
    std::vector<WiFiNetwork> networks;

    int n = WiFi.scanNetworks(false, true);
    for (int i = 0; i < n; i++) {
        WiFiNetwork network;
        network.ssid = WiFi.SSID(i).c_str();
        if (network.ssid.isEmpty()) { network.ssid = "Hidden SSID"; }
        network.encryption = WiFi.encryptionType(i);
        network.rssi = WiFi.RSSI(i);
        network.bssid = WiFi.BSSIDstr(i);
        networks.push_back(network);
    }

    return networks;
}

std::vector<WiFiNetwork> getOpenWiFiNetworks(std::vector<WiFiNetwork>& networks) {
    std::vector<WiFiNetwork> openNetworks;
    
    for (auto& network : networks) {
        if (network.encryption == WIFI_AUTH_OPEN) {
            network.open = true;
            openNetworks.push_back(network);
        }
    }

    return openNetworks;
}

std::vector<WiFiNetwork> getClosedWiFiNetworks(const std::vector<WiFiNetwork>& networks) {
    std::vector<WiFiNetwork> closedNetworks;

    for (const auto& network : networks) {
        if (network.encryption != WIFI_AUTH_OPEN) {
            closedNetworks.push_back(network);
        }
    }

    return closedNetworks;
}

std::vector<WiFiNetwork> getVulnerableWiFiNetworks(std::vector<WiFiNetwork>& networks) {
    std::vector<WiFiNetwork> vulnerableNetworks;

    for (auto& network : networks) {
        if (network.encryption == WIFI_AUTH_WEP || 
            network.encryption == WIFI_AUTH_WPA_PSK) {
            network.vulnerable = true;
            vulnerableNetworks.push_back(network);
        }
    }

    return vulnerableNetworks;
}

std::vector<WiFiNetwork> getValidatedOpenWiFiNetworks(std::vector<WiFiNetwork>& networks) {
    std::vector<WiFiNetwork> validatedNetworks;
    
    for (auto& network : networks) {
        if (network.encryption == WIFI_AUTH_OPEN) {
            if (testOpenWiFiConnection(network)) {
                network.webAccess = true;
                validatedNetworks.push_back(network);
            }
        }
    }

    return validatedNetworks;
}

bool testOpenWiFiConnection(const WiFiNetwork& network) {
    WiFi.begin(network.ssid.c_str());

    int maxRetries = 5;
    while (WiFi.status() != WL_CONNECTED && maxRetries > 0) {
        delay(500);
        maxRetries--;
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        return false;
    }

    HTTPClient http;
    http.begin("http://example.com");
    http.setTimeout(3000);
    int httpCode = http.GET();
    http.end();

    WiFi.disconnect();

    return httpCode > 0; // any http code confirms web access
}

String encryptionTypeToString(wifi_auth_mode_t encryption) {
    switch (encryption) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA+";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "ENT";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA*";
        case WIFI_AUTH_WAPI_PSK: return "WAPI";
        default: return "UNK";
    }
}

void displayAdvancedWiFiResults(std::vector<WiFiNetwork>& networks, int scrollOffset = 0) {
    M5Cardputer.Display.fillScreen(BLACK);
    
    // Title
    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(10, 5);
    M5Cardputer.Display.printf("WiFi Analysis");
    
    // Network counts
    displayWiFiCounts(networks);
    
    // Network list with scrolling
    M5Cardputer.Display.setTextSize(1);
    int startY = 55;
    int lineHeight = 12;
    int maxDisplay = 4;
    
    // Calculate visible range
    int startIndex = scrollOffset;
    int endIndex = min(startIndex + maxDisplay, (int)networks.size());
    
    for (int i = startIndex; i < endIndex; i++) {
        const WiFiNetwork& network = networks[i];
        int displayIndex = i - startIndex;
        int y = startY + (displayIndex * lineHeight);
        
        // Security status icon and color
        if (network.webAccess) {
            M5Cardputer.Display.setTextColor(GREEN);
            M5Cardputer.Display.setCursor(5, y);
            M5Cardputer.Display.printf("✓");
        } else if (network.open) {
            M5Cardputer.Display.setTextColor(WHITE);
            M5Cardputer.Display.setCursor(5, y);
            M5Cardputer.Display.printf("○");
        } else if (network.vulnerable) {
            M5Cardputer.Display.setTextColor(YELLOW);
            M5Cardputer.Display.setCursor(5, y);
            M5Cardputer.Display.printf("⚠");
        } else {
            M5Cardputer.Display.setTextColor(RED);
            M5Cardputer.Display.setCursor(5, y);
            M5Cardputer.Display.printf("●");
        }
        
        // Network name
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.setCursor(20, y);
        String displayName = network.ssid;
        if (displayName.length() > 12) {
            displayName = displayName.substring(0, 12) + "..";
        }
        M5Cardputer.Display.printf("%s", displayName.c_str());
        
        // Signal strength
        int bars = map(network.rssi, -100, -30, 1, 4);
        for (int bar = 0; bar < 4; bar++) {
            uint16_t color = (bar < bars) ? GREEN : DARKGREY;
            M5Cardputer.Display.fillRect(155 + bar * 3, y + 8 - bar * 2, 2, bar * 2 + 2, color);
        }
        
        // Encryption type
        M5Cardputer.Display.setTextColor(CYAN);
        M5Cardputer.Display.setCursor(175, y);
        M5Cardputer.Display.printf("%s", encryptionTypeToString(network.encryption).c_str());
    }
    
    // Scroll indicators
    if (scrollOffset > 0) {
        M5Cardputer.Display.setTextColor(YELLOW);
        M5Cardputer.Display.setCursor(220, 55);
        M5Cardputer.Display.printf("↑");
    }
    if (scrollOffset + maxDisplay < (int)networks.size()) {
        M5Cardputer.Display.setTextColor(YELLOW);
        M5Cardputer.Display.setCursor(220, 95);
        M5Cardputer.Display.printf("↓");
    }
    
    // Controls
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(5, 110);
    M5Cardputer.Display.printf("↕=Scroll R=Rescan Fn=Exit");
    
    // Show position indicator
    if (networks.size() > maxDisplay) {
        M5Cardputer.Display.setTextColor(DARKGREY);
        M5Cardputer.Display.setCursor(180, 110);
        M5Cardputer.Display.printf("%d/%d", startIndex + 1, (int)networks.size());
    }
    
    M5Cardputer.Display.display();
}

void displayWiFiCounts(std::vector<WiFiNetwork>& networks) {
    int webAccessCount = 0;
    int openCount = 0;
    int vulnerableCount = 0;
    int closedCount = 0;
    
    // Count networks by type
    for (const auto& network : networks) {
        if (network.webAccess) {
            webAccessCount++;
        } else if (network.open) {
            openCount++;
        } else if (network.vulnerable) {
            vulnerableCount++;
        } else {
            closedCount++;
        }
    }
    
    M5Cardputer.Display.setTextSize(1);
    int startY = 25;
    
    // Web access (green)
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(10, startY);
    M5Cardputer.Display.printf("Web:%d", webAccessCount);
    
    // Open (white)
    M5Cardputer.Display.setTextColor(WHITE);  
    M5Cardputer.Display.setCursor(55, startY);
    M5Cardputer.Display.printf("Open:%d", openCount);
    
    // Vulnerable (yellow)
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(105, startY);
    M5Cardputer.Display.printf("Weak:%d", vulnerableCount);
    
    // Closed (red)
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setCursor(155, startY);
    M5Cardputer.Display.printf("Sec:%d", closedCount);
    
    // Legend
    M5Cardputer.Display.setTextColor(DARKGREY);
    M5Cardputer.Display.setCursor(10, 40);
    M5Cardputer.Display.printf("✓=Internet ○=Open ⚠=Weak ●=Secure");
}

void displayWiFiResults(int networkCount) {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("WiFi Networks");
  
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(10, 35);
  M5Cardputer.Display.printf("Found %d networks:", networkCount);
  
  int maxDisplay = min(networkCount, 7); // Show max 7 networks
  for (int i = 0; i < maxDisplay; i++) {
    int y = 55 + i * 12;
    
    // Network strength bars
    int rssi = WiFi.RSSI(i);
    uint16_t signalColor;
    if (rssi > -50) signalColor = GREEN;
    else if (rssi > -70) signalColor = YELLOW;
    else signalColor = RED;
    
    // Draw signal strength bars
    for (int bar = 0; bar < 4; bar++) {
      if (rssi > -30 - (bar * 20)) {
        M5Cardputer.Display.fillRect(5 + bar * 3, y + 8 - bar * 2, 2, bar * 2 + 2, signalColor);
      }
    }
    
    // Network name
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(20, y);
    String ssid = WiFi.SSID(i);
    if (ssid.length() > 25) ssid = ssid.substring(0, 25) + "...";
    M5Cardputer.Display.printf("%s", ssid.c_str());
    
    // Security indicator
    M5Cardputer.Display.setTextColor((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? RED : GREEN);
    M5Cardputer.Display.setCursor(200, y);
    M5Cardputer.Display.printf("%s", (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "SEC");
  }
  
  M5Cardputer.Display.setTextColor(GRAY);
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 25);
  M5Cardputer.Display.printf("R to refresh, M/Fn/BtnA for menu");
}

// Matrix Portal Application - LAZY INITIALIZATION (no boot-time network objects)
#include <WebServer.h>    // Only included when needed
#include <DNSServer.h>    // Only included when needed

// Global pointers for lazy initialization
WebServer* matrixServer = nullptr;
DNSServer* matrixDNS = nullptr;
std::vector<String> storedMessages;
String currentMessage = "";
String currentContact = "";

void initCaptivePortal() {
  Serial.println("Starting Matrix Portal...");
  
  // LAZY INITIALIZATION - Only create network objects when explicitly requested
  if (matrixServer == nullptr) {
    matrixServer = new WebServer(80);
    matrixDNS = new DNSServer();
  }
  
  // Initialize WiFi only when entering Matrix Portal
  WiFi.mode(WIFI_AP);
  
  // Setup AP
  WiFi.softAP("Matrix Portal", "");
  delay(500);
  
  // Setup DNS server to redirect all requests
  matrixDNS->start(53, "*", WiFi.softAPIP());
  
  // Setup web server
  matrixServer->on("/", handleMatrixRoot);
  matrixServer->on("/submit", HTTP_POST, handleMatrixSubmit);
  matrixServer->onNotFound(handleMatrixRoot);
  matrixServer->begin();
  
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.println("MATRIX PORTAL ACTIVE");
  M5Cardputer.Display.println("IP: " + WiFi.softAPIP().toString());
  M5Cardputer.Display.println("SSID: Matrix Portal");
  M5Cardputer.Display.println("");
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.println("Messages in void:");
  M5Cardputer.Display.println(String(storedMessages.size()));
  M5Cardputer.Display.println("");
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.println("Press M/Fn to exit");
}

void handleMatrixRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Matrix Portal</title>";
  html += "<style>body{background:#000;color:#00ff00;font-family:'Courier New',monospace;margin:0;padding:20px;}";
  html += "h1{text-align:center;text-shadow:0 0 10px #00ff00;}";
  html += ".container{max-width:400px;margin:0 auto;padding:20px;border:2px solid #00ff00;border-radius:10px;background:rgba(0,255,0,0.1);}";
  html += "input,textarea{width:100%;padding:10px;margin:10px 0;background:#001100;color:#00ff00;border:1px solid #00ff00;border-radius:5px;}";
  html += "button{width:100%;padding:15px;background:#004400;color:#00ff00;border:2px solid #00ff00;border-radius:5px;cursor:pointer;font-size:16px;}";
  html += "button:hover{background:#006600;}</style></head><body>";
  html += "<h1>LEAVE A MESSAGE IN THE DIGITAL REALM</h1>";
  html += "<div class='container'>";
  html += "<form action='/submit' method='POST'>";
  html += "<label>Your Message:</label>";
  html += "<textarea name='message' rows='4' placeholder='Enter your message for the digital void...' required></textarea>";
  html += "<label>Contact (Optional):</label>";
  html += "<input type='text' name='contact' placeholder='Email for reply...'>";
  html += "<button type='submit'>TRANSMIT TO MATRIX</button>";
  html += "</form>";
  html += "<p style='text-align:center;margin-top:30px;color:#008800;'>Messages are stored in the digital void</p>";
  html += "</div></body></html>";
  
  matrixServer->send(200, "text/html", html);
}

void handleMatrixSubmit() {
  if (matrixServer->hasArg("message")) {
    currentMessage = matrixServer->arg("message");
    currentContact = matrixServer->hasArg("contact") ? matrixServer->arg("contact") : "Anonymous";
    
    String fullMessage = "[" + String(millis()/1000) + "s] " + currentContact + ": " + currentMessage;
    storedMessages.push_back(fullMessage);
    
    // Update display
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("NEW MESSAGE RECEIVED!");
    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.println("From: " + currentContact);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Msg: " + currentMessage);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.println("Total: " + String(storedMessages.size()));
    
    String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    response += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    response += "<style>body{background:#000;color:#00ff00;font-family:'Courier New',monospace;text-align:center;padding:50px;}</style>";
    response += "</head><body><h1>MESSAGE TRANSMITTED</h1>";
    response += "<p>Your message has been stored in the digital void...</p>";
    response += "<p>The Matrix acknowledges your transmission.</p></body></html>";
    
    matrixServer->send(200, "text/html", response);
  } else {
    matrixServer->send(400, "text/plain", "No message received");
  }
}

void handleCaptivePortal() {
  matrixDNS->processNextRequest();
  matrixServer->handleClient();
  
  if (checkEscapeKey()) {
    matrixServer->stop();
    matrixDNS->stop();
    WiFi.softAPdisconnect(true);
    enterMenu();
    return;
  }
}

// Clock & Alarm Application
// Clock settings variables
bool inClockSettings = false;
int settingHour = 12;
int settingMinute = 0;
int alarmHour = 7;
int alarmMinute = 0;
bool alarmEnabled = false;
int clockSettingIndex = 0; // 0=hour, 1=minute, 2=alarm hour, 3=alarm minute, 4=alarm enable

void initClockAlarm() {
  Serial.println("Clock & Alarm initialized");
  
  // Load saved settings
  preferences.begin("clock", false);
  alarmHour = preferences.getInt("alarmHour", 7);
  alarmMinute = preferences.getInt("alarmMin", 0);
  alarmEnabled = preferences.getBool("alarmOn", false);
  preferences.end();
}

void handleClockAlarm() {
  if (checkEscapeKey()) {
    inClockSettings = false;
    enterMenu();
    return;
  }
  
  M5Cardputer.update();
  
  // Check for settings mode toggle
  if (M5Cardputer.Keyboard.isKeyPressed('s') || M5Cardputer.Keyboard.isKeyPressed('S')) {
    inClockSettings = !inClockSettings;
    if (inClockSettings) {
      settingHour = rtc.getHour(true);
      settingMinute = rtc.getMinute();
      clockSettingIndex = 0;
    }
    delay(200);
    return;
  }
  
  if (inClockSettings) {
    handleClockSettings();
    return;
  }
  
  // Check alarm - trigger for a full minute, not just one second
  static bool alarmTriggered = false;
  if (alarmEnabled && rtc.getHour(true) == alarmHour && rtc.getMinute() == alarmMinute) {
    if (!alarmTriggered) {
      alarmTriggered = true;
    }
    
    // Check for X key to disable alarm
    if (M5Cardputer.Keyboard.isKeyPressed('x') || M5Cardputer.Keyboard.isKeyPressed('X')) {
      alarmTriggered = false;
      delay(200);
      return; // Exit alarm and continue with clock display
    }
    
    // Visual alarm display
    M5Cardputer.Display.fillScreen(millis() % 1000 < 500 ? RED : BLACK); // Flashing red
    M5Cardputer.Display.setTextSize(3);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(15, 50);
    M5Cardputer.Display.printf("TIME'S UP!");
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(60, 90);
    M5Cardputer.Display.printf("%02d:%02d", alarmHour, alarmMinute);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(30, 120);
    M5Cardputer.Display.printf("Press X to stop");
    return;
  } else {
    alarmTriggered = false;
  }
  
  // Display current time
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Digital clock display
  M5Cardputer.Display.setTextSize(3);
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setCursor(30, 40);
  M5Cardputer.Display.printf("%02d:%02d:%02d", rtc.getHour(true), rtc.getMinute(), rtc.getSecond());
  
  // Show alarm status
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(alarmEnabled ? GREEN : RED);
  M5Cardputer.Display.setCursor(10, 90);
  M5Cardputer.Display.printf("Alarm: %s %02d:%02d", 
    alarmEnabled ? "ON" : "OFF", alarmHour, alarmMinute);
  
  M5Cardputer.Display.setTextColor(GRAY);
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 15);
  M5Cardputer.Display.printf("S=Settings M/Fn=Menu");
  
  delay(1000); // Update once per second
}

void handleClockSettings() {
  M5Cardputer.update();
  
  // Navigation
  if (M5Cardputer.Keyboard.isKeyPressed(';') || M5Cardputer.Keyboard.isKeyPressed('w') || M5Cardputer.Keyboard.isKeyPressed('W')) {
    clockSettingIndex = (clockSettingIndex - 1 + 5) % 5;
    delay(200);
  }
  if (M5Cardputer.Keyboard.isKeyPressed('.') || M5Cardputer.Keyboard.isKeyPressed('s') || M5Cardputer.Keyboard.isKeyPressed('S')) {
    clockSettingIndex = (clockSettingIndex + 1) % 5;
    delay(200);
  }
  
  // Value adjustment
  if (M5Cardputer.Keyboard.isKeyPressed('a') || M5Cardputer.Keyboard.isKeyPressed('A')) {
    switch(clockSettingIndex) {
      case 0: settingHour = (settingHour - 1 + 24) % 24; break;
      case 1: settingMinute = (settingMinute - 1 + 60) % 60; break;
      case 2: alarmHour = (alarmHour - 1 + 24) % 24; break;
      case 3: alarmMinute = (alarmMinute - 1 + 60) % 60; break;
      case 4: alarmEnabled = !alarmEnabled; break;
    }
    delay(150);
  }
  if (M5Cardputer.Keyboard.isKeyPressed('d') || M5Cardputer.Keyboard.isKeyPressed('D')) {
    switch(clockSettingIndex) {
      case 0: settingHour = (settingHour + 1) % 24; break;
      case 1: settingMinute = (settingMinute + 1) % 60; break;
      case 2: alarmHour = (alarmHour + 1) % 24; break;
      case 3: alarmMinute = (alarmMinute + 1) % 60; break;
      case 4: alarmEnabled = !alarmEnabled; break;
    }
    delay(150);
  }
  
  // Save and apply
  if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    // Set RTC time using setTime method
    rtc.setTime(rtc.getSecond(), settingMinute, settingHour, rtc.getDay(), rtc.getMonth(), rtc.getYear());
    
    // Save alarm settings
    preferences.begin("clock", false);
    preferences.putInt("alarmHour", alarmHour);
    preferences.putInt("alarmMin", alarmMinute);
    preferences.putBool("alarmOn", alarmEnabled);
    preferences.end();
    
    inClockSettings = false;
    delay(200);
    return;
  }
  
  // Display settings UI
  M5Cardputer.Display.fillScreen(BLACK);
  
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setCursor(20, 10);
  M5Cardputer.Display.printf("Clock Settings");
  
  M5Cardputer.Display.setTextSize(1);
  int y = 40;
  
  // Current Time Setting
  M5Cardputer.Display.setTextColor(clockSettingIndex == 0 ? YELLOW : WHITE);
  M5Cardputer.Display.setCursor(10, y);
  M5Cardputer.Display.printf("Hour: %02d", settingHour);
  y += 15;
  
  M5Cardputer.Display.setTextColor(clockSettingIndex == 1 ? YELLOW : WHITE);
  M5Cardputer.Display.setCursor(10, y);
  M5Cardputer.Display.printf("Minute: %02d", settingMinute);
  y += 20;
  
  // Alarm Settings
  M5Cardputer.Display.setTextColor(clockSettingIndex == 2 ? YELLOW : WHITE);
  M5Cardputer.Display.setCursor(10, y);
  M5Cardputer.Display.printf("Alarm Hour: %02d", alarmHour);
  y += 15;
  
  M5Cardputer.Display.setTextColor(clockSettingIndex == 3 ? YELLOW : WHITE);
  M5Cardputer.Display.setCursor(10, y);
  M5Cardputer.Display.printf("Alarm Min: %02d", alarmMinute);
  y += 15;
  
  M5Cardputer.Display.setTextColor(clockSettingIndex == 4 ? YELLOW : WHITE);
  M5Cardputer.Display.setCursor(10, y);
  M5Cardputer.Display.printf("Alarm: %s", alarmEnabled ? "ON" : "OFF");
  
  // Instructions
  M5Cardputer.Display.setTextColor(GRAY);
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 25);
  M5Cardputer.Display.printf("A/D=Change Enter=Save");
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 15);
  M5Cardputer.Display.printf("S=Exit Settings");
}

// Portal Hater Application
void initPortalHater() {
  Serial.println("Portal Hater initialized - Scanning for evil portals...");
  portalKillCount = 0;
  lastPortalScan = 0;
  lastAttackedPortal = "";
  
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("Portal Hater");
  
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(10, 35);
  M5Cardputer.Display.printf("Scanning for evil portals...");
  
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(10, 55);
  M5Cardputer.Display.printf("Kills: %d", portalKillCount);
  
  // Start immediate scan
  scanForEvilPortals();
}

void handlePortalHater() {
  if (checkEscapeKey()) {
    portalHaterActive = false;
    enterMenu();
    return;
  }
  
  static unsigned long lastKeyPress = 0;
  static unsigned long lastUpdate = 0;
  
  // Update display every second
  if (millis() - lastUpdate > 1000) {
    M5Cardputer.Display.fillRect(0, 55, 240, 15, BLACK);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(10, 55);
    M5Cardputer.Display.printf("Kills: %d", portalKillCount);
    
    M5Cardputer.Display.fillRect(0, 75, 240, 15, BLACK);
    M5Cardputer.Display.setTextColor(scanningForPortals ? YELLOW : GREEN);
    M5Cardputer.Display.setCursor(10, 75);
    M5Cardputer.Display.printf("Status: %s", scanningForPortals ? "SCANNING" : "MONITORING");
    
    lastUpdate = millis();
  }
  
  if (M5Cardputer.Keyboard.isPressed() && (millis() - lastKeyPress > 200)) {
    
    // Manual scan trigger (S key)
    if (M5Cardputer.Keyboard.isKeyPressed('s')) {
      M5Cardputer.Display.fillRect(0, 35, 240, 15, BLACK);
      M5Cardputer.Display.setTextColor(YELLOW);
      M5Cardputer.Display.setCursor(10, 35);
      M5Cardputer.Display.printf("Manual scan triggered...");
      scanForEvilPortals();
      lastKeyPress = millis();
    }
    
    // Toggle auto-scanning (A key)
    else if (M5Cardputer.Keyboard.isKeyPressed('a')) {
      portalHaterActive = !portalHaterActive;
      M5Cardputer.Display.fillRect(0, 95, 240, 15, BLACK);
      M5Cardputer.Display.setTextColor(portalHaterActive ? GREEN : RED);
      M5Cardputer.Display.setCursor(10, 95);
      M5Cardputer.Display.printf("Auto-scan: %s", portalHaterActive ? "ON" : "OFF");
      lastKeyPress = millis();
    }
  }
  
  // Auto-scan when active
  if (portalHaterActive && (millis() - lastPortalScan > PORTAL_SCAN_INTERVAL)) {
    scanForEvilPortals();
  }
  
  // Show controls
  M5Cardputer.Display.setTextColor(GRAY);
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 35);
  M5Cardputer.Display.printf("S=Manual Scan A=Auto-Toggle");
  M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 25);
  M5Cardputer.Display.printf("M=Menu");
}

// ===== PORTAL HATER FUNCTIONS =====

void scanForEvilPortals() {
  Serial.println("=== Scanning for evil portals ===");
  scanningForPortals = true;
  lastPortalScan = millis();
  
  // Update display
  M5Cardputer.Display.fillRect(0, 35, 240, 15, BLACK);
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(10, 35);
  M5Cardputer.Display.printf("SCANNING NETWORKS...");
  
  // Scan for networks (exactly like original)
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    M5Cardputer.Display.fillRect(0, 35, 240, 15, BLACK);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.setCursor(10, 35);
    M5Cardputer.Display.printf("No networks found");
    scanningForPortals = false;
    delay(1000);
    return;
  }
  
  Serial.printf("Found %d networks\n", n);
  
  // Look for open networks (exactly like original)
  for (int i = 0; i < n; i++) {
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
      String ssid = WiFi.SSID(i);
      
      Serial.printf("Evil portal detected: %s\n", ssid.c_str());
      
      // Attack the portal
      lastAttackedPortal = ssid;
      attackPortal(ssid);
      portalKillCount++;
      
      // Update kill count display
      M5Cardputer.Display.fillRect(0, 55, 240, 15, BLACK);
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.setCursor(10, 55);
      M5Cardputer.Display.printf("Kills: %d", portalKillCount);
      
      delay(1000);
    }
  }
  
  WiFi.scanDelete();
  scanningForPortals = false;
  Serial.println("=== Scan complete ===");
}

void attackPortal(String ssid) {
  Serial.printf("=== Attacking portal: %s ===\n", ssid.c_str());
  
  // Update display
  M5Cardputer.Display.fillRect(0, 35, 240, 25, BLACK);
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.setCursor(10, 35);
  M5Cardputer.Display.printf("ATTACKING:");
  M5Cardputer.Display.setCursor(10, 50);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf(ssid.c_str());
  
  // Try to connect (exactly like original)
  WiFi.begin(ssid.c_str());
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000) {
    delay(100);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected! Launching counterattack...");
    
    // Get the actual gateway IP and attack it (this is where the portal lives)
    IPAddress gateway = WiFi.gatewayIP();
    String gatewayIP = gateway.toString();
    Serial.printf("Gateway IP detected: %s\n", gatewayIP.c_str());
    
    // Attack both common portal styles and NEMO-style portals
    for (int i = 0; i < 3; i++) {
      // Common portal endpoints (192.168.4.1 style - Bruce)
      String commonEndpoints[] = {"/login", "/auth", "/signin", "/portal", "/admin"};
      
      for (String endpoint : commonEndpoints) {
        HTTPClient http;
        String url = "http://" + gatewayIP + endpoint;
        http.begin(url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        // Send our specific message for Bruce-style portals
        String fakeData = "username=Caught Ya Slippin&password=Ya Damn Fool&email=Caught Ya Slippin";
        
        Serial.printf("Attacking %s (common portal)\n", url.c_str());
        int responseCode = http.POST(fakeData);
        Serial.printf("Response: %d\n", responseCode);
        
        http.end();
        delay(30);
      }
      
      // NEMO portal endpoints (172.0.0.1 style - expects email/password)
      String nemoEndpoints[] = {"/post", "/post", "/post"}; // Hit /post multiple times
      
      for (String endpoint : nemoEndpoints) {
        HTTPClient http;
        String url = "http://" + gatewayIP + endpoint;
        http.begin(url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        // NEMO expects "email" and "password" parameters (not username)
        String fakeData = "email=Caught Ya Slippin&password=Ya Damn Fool";
        
        Serial.printf("Attacking %s (NEMO portal)\n", url.c_str());
        int responseCode = http.POST(fakeData);
        Serial.printf("Response: %d\n", responseCode);
        
        http.end();
        delay(30);
      }
    }
    
    WiFi.disconnect();
    Serial.println("Counterattack complete!");
    
    M5Cardputer.Display.fillRect(0, 35, 240, 25, BLACK);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(10, 35);
    M5Cardputer.Display.printf("PORTAL ATTACKED!");
    
  } else {
    Serial.println("Couldn't connect to portal");
    M5Cardputer.Display.fillRect(0, 35, 240, 25, BLACK);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(10, 35);
    M5Cardputer.Display.printf("PORTAL DODGED!");
  }
  
  delay(1500);
  lastAttackedPortal = ssid;
}

void sendPortalSpam(String targetIP) {
  Serial.printf("Sending message to %s\n", targetIP.c_str());
  
  // Ultra-simple single request to prevent crashes
  HTTPClient http;
  
  // Very aggressive timeouts
  http.setTimeout(1000);
  http.setConnectTimeout(500);
  
  String url = "http://" + targetIP + "/";
  
  if (http.begin(url)) {
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String postData = "username=Caught Ya Slippin&password=Ya Damn Fool";
    
    int responseCode = http.POST(postData);
    Serial.printf("Sent message - Response: %d\n", responseCode);
    
    http.end();
  }
  
  delay(500);
  yield();
}

String getCounterMessage() {
  // Simple counter-messages (family-friendly)
  String messages[] = {
    "Stop stealing data!",
    "Security violation detected",
    "Unauthorized portal blocked",
    "Data theft prevention active",
    "Portal defense engaged",
    "Stop phishing attempts",
    "Credential theft blocked",
    "Evil portal neutralized",
    "Security research active",
    "Portal monitoring engaged"
  };
  
  int numMessages = 10;
  return messages[random(0, numMessages)];
}

// ===== CUSTOM PORTAL HATER FUNCTIONS =====

String getTextInput(String prompt, String currentText, int maxLength) {
  String input = currentText;
  bool editing = true;
  unsigned long lastBlink = 0;
  bool showCursor = true;
  
  while (editing) {
    M5Cardputer.update();
    
    // Handle ESC key to cancel
    if (checkEscapeKey()) {
      return currentText; // Return original text if cancelled
    }
    
    // Handle ENTER to confirm
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      editing = false;
      delay(200); // Debounce
      break;
    }
    
    // Handle BACKSPACE - simpler approach
    if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      if (input.length() > 0) {
        input.remove(input.length() - 1);
      }
      delay(150); // Debounce
    }
    
    // Handle character input - check each key individually
    if (M5Cardputer.Keyboard.isPressed()) {
      // Check printable characters
      for (char c = 'a'; c <= 'z'; c++) {
        if (M5Cardputer.Keyboard.isKeyPressed(c) && input.length() < maxLength) {
          input += c;
          delay(150);
          break;
        }
      }
      for (char c = 'A'; c <= 'Z'; c++) {
        if (M5Cardputer.Keyboard.isKeyPressed(c) && input.length() < maxLength) {
          input += c;
          delay(150);
          break;
        }
      }
      for (char c = '0'; c <= '9'; c++) {
        if (M5Cardputer.Keyboard.isKeyPressed(c) && input.length() < maxLength) {
          input += c;
          delay(150);
          break;
        }
      }
      // Common symbols
      char symbols[] = {' ', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '_', '+', '=', '.', ',', '?'};
      for (char c : symbols) {
        if (M5Cardputer.Keyboard.isKeyPressed(c) && input.length() < maxLength) {
          input += c;
          delay(150);
          break;
        }
      }
    }
    
    // Display
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.printf("%s:", prompt.c_str());
    
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextSize(2);  // Double the font size for better readability
    M5Cardputer.Display.setCursor(10, 30);
    M5Cardputer.Display.printf("%s", input.c_str());
    
    // Blinking cursor
    if (millis() - lastBlink > 500) {
      showCursor = !showCursor;
      lastBlink = millis();
    }
    
    if (showCursor) {
      M5Cardputer.Display.setCursor(10 + input.length() * 12, 30);  // Adjust cursor position for size 2 font
      M5Cardputer.Display.printf("_");
    }
    
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(10, 110);
    M5Cardputer.Display.printf("ENTER: Confirm  ESC: Cancel");
    
    delay(50);
  }
  
  return input;
}

void scanForEvilPortalsCustom() {
  Serial.println("=== Custom Portal Hater - Scanning ===");
  
  // Update display
  M5Cardputer.Display.fillRect(0, 35, 240, 15, BLACK);
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(10, 35);
  M5Cardputer.Display.printf("SCANNING NETWORKS...");
  
  // Scan for networks (same as original)
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    M5Cardputer.Display.fillRect(0, 35, 240, 15, BLACK);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.setCursor(10, 35);
    M5Cardputer.Display.printf("No networks found");
    delay(1000);
    return;
  }
  
  Serial.printf("Found %d networks\n", n);
  
  // Look for open networks (same as original)
  for (int i = 0; i < n; i++) {
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
      String ssid = WiFi.SSID(i);
      
      Serial.printf("Evil portal detected: %s\n", ssid.c_str());
      
      // Attack with custom messages
      attackPortalCustom(ssid);
      portalKillCount++;
      
      // Update kill count display
      M5Cardputer.Display.fillRect(0, 55, 240, 15, BLACK);
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.setCursor(10, 55);
      M5Cardputer.Display.printf("Kills: %d", portalKillCount);
      
      delay(1000);
    }
  }
  
  WiFi.scanDelete();
}

void attackPortalCustom(String ssid) {
  Serial.printf("=== Custom attacking portal: %s ===\n", ssid.c_str());
  
  // Update display
  M5Cardputer.Display.fillRect(0, 35, 240, 25, BLACK);
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.setCursor(10, 35);
  M5Cardputer.Display.printf("ATTACKING:");
  M5Cardputer.Display.setCursor(10, 50);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf(ssid.c_str());
  
  // Try to connect (same as original)
  WiFi.begin(ssid.c_str());
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000) {
    delay(100);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected! Launching custom counterattack...");
    
    // Get the actual gateway IP
    IPAddress gateway = WiFi.gatewayIP();
    String gatewayIP = gateway.toString();
    Serial.printf("Gateway IP detected: %s\n", gatewayIP.c_str());
    
    // Attack both portal styles with CUSTOM messages
    for (int i = 0; i < 3; i++) {
      // Bruce-style portals (common endpoints)
      String commonEndpoints[] = {"/login", "/auth", "/signin", "/portal", "/admin"};
      
      for (String endpoint : commonEndpoints) {
        HTTPClient http;
        String url = "http://" + gatewayIP + endpoint;
        http.begin(url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        // Use CUSTOM messages instead of fixed ones
        String fakeData = "username=" + customLine1 + "&password=" + customLine2 + "&email=" + customLine1;
        
        Serial.printf("Custom attacking %s (common portal)\n", url.c_str());
        int responseCode = http.POST(fakeData);
        Serial.printf("Response: %d\n", responseCode);
        
        http.end();
        delay(30);
      }
      
      // NEMO-style portals (/post endpoint)
      String nemoEndpoints[] = {"/post", "/post", "/post"};
      
      for (String endpoint : nemoEndpoints) {
        HTTPClient http;
        String url = "http://" + gatewayIP + endpoint;
        http.begin(url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        // NEMO expects "email" and "password" with CUSTOM messages
        String fakeData = "email=" + customLine1 + "&password=" + customLine2;
        
        Serial.printf("Custom attacking %s (NEMO portal)\n", url.c_str());
        int responseCode = http.POST(fakeData);
        Serial.printf("Response: %d\n", responseCode);
        
        http.end();
        delay(30);
      }
    }
    
    WiFi.disconnect();
    Serial.println("Custom counterattack complete!");
    
    M5Cardputer.Display.fillRect(0, 35, 240, 25, BLACK);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(10, 35);
    M5Cardputer.Display.printf("PORTAL ATTACKED!");
    
  } else {
    Serial.println("Couldn't connect to portal");
    M5Cardputer.Display.fillRect(0, 35, 240, 25, BLACK);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(10, 35);
    M5Cardputer.Display.printf("PORTAL DODGED!");
  }
  
  delay(1500);
}

void enterCustomPortalHater() {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("CUSTOM PORTAL");
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.printf("HATER SETUP");
  
  delay(1500);
  
  // Get Line 1 (email field message)
  customLine1 = getTextInput("Line 1 (Email Field)", customLine1, 50);
  
  // Get Line 2 (password field message)  
  customLine2 = getTextInput("Line 2 (Password Field)", customLine2, 50);
  
  // Show summary and enter hunting mode
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("Custom Messages Set:");
  
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.printf("Email: %s", customLine1.substring(0, 30).c_str());
  M5Cardputer.Display.setCursor(10, 45);
  M5Cardputer.Display.printf("Pass: %s", customLine2.substring(0, 30).c_str());
  
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(10, 70);
  M5Cardputer.Display.printf("Press S to start hunting");
  M5Cardputer.Display.setCursor(10, 85);
  M5Cardputer.Display.printf("Press A for auto-scan");
  M5Cardputer.Display.setCursor(10, 100);
  M5Cardputer.Display.printf("Press ESC to exit");
  
  // Enter custom portal hater mode
  customPortalHaterActive = false;
  unsigned long lastCustomScan = 0;
  
  while (true) {
    M5Cardputer.update();
    
    // Check for ESC to exit
    if (checkEscapeKey()) {
      customPortalHaterActive = false;
      enterMenu();
      return;
    }
    
    // Check for S key (manual scan)
    if (M5Cardputer.Keyboard.isKeyPressed('s')) {
      scanForEvilPortalsCustom();
      delay(300);
    }
    
    // Check for A key (toggle auto-scan)
    if (M5Cardputer.Keyboard.isKeyPressed('a')) {
      customPortalHaterActive = !customPortalHaterActive;
      
      M5Cardputer.Display.fillRect(0, 100, 240, 35, BLACK);
      M5Cardputer.Display.setTextColor(customPortalHaterActive ? RED : GREEN);
      M5Cardputer.Display.setCursor(10, 100);
      M5Cardputer.Display.printf("Custom Portal Hater: %s", 
                                  customPortalHaterActive ? "ACTIVE" : "OFF");
      
      if (customPortalHaterActive) {
        lastCustomScan = 0; // Force immediate scan
      }
      delay(300);
    }
    
    // Auto-scan if active
    if (customPortalHaterActive) {
      unsigned long now = millis();
      if (now - lastCustomScan >= PORTAL_SCAN_INTERVAL) {
        scanForEvilPortalsCustom();
        lastCustomScan = now;
      }
    }
    
    delay(100);
  }
}

// ===== MP3 PLAYER IMPLEMENTATION =====

// Persistence system for MP3 files
void saveMP3List() {
  File indexFile = SD.open("/mp3_index.txt", FILE_WRITE);
  if (!indexFile) {
    Serial.println("Failed to create MP3 index file");
    return;
  }
  
  indexFile.println(no_of_files); // Write count first
  for (int i = 0; i < no_of_files; i++) {
    indexFile.println(files[i]);
  }
  indexFile.close();
  Serial.println("MP3 index saved to SD card");
}

bool loadMP3List() {
  File indexFile = SD.open("/mp3_index.txt", FILE_READ);
  if (!indexFile) {
    Serial.println("No MP3 index found - will need to scan");
    return false;
  }
  
  String countStr = indexFile.readStringUntil('\n');
  no_of_files = countStr.toInt();
  
  if (no_of_files > 2000) no_of_files = 2000; // Safety limit
  
  for (int i = 0; i < no_of_files; i++) {
    String filename = indexFile.readStringUntil('\n');
    filename.trim(); // Remove newline
    if (filename.length() > 0) {
      files[i] = filename;
    }
  }
  indexFile.close();
  
  Serial.printf("Loaded %d MP3 files from index\n", no_of_files);
  return true;
}

static AudioFileSourceSD file;
static AudioOutputM5Speaker out(&M5Cardputer.Speaker, m5spk_virtual_channel);
static AudioGeneratorMP3 mp3;
static AudioFileSourceID3* id3 = nullptr;
static size_t fileindex = 0;
uint32_t paused_at = 0;
bool mp3Scanned = false;
float mp3Volume = 0.5; // Start at 50% volume
bool isShuffleMode = false;  // Random song selection

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  if (string[0] == 0) { return; }
  if (strcmp(type, "eof") == 0)
  {
    M5Cardputer.Display.display();
    return;
  }
  M5Cardputer.Display.setCursor(0, M5Cardputer.Display.getCursorY() + 12);
}

void stopMP3(void)
{
  if (id3 == nullptr) return;
  out.stop();
  mp3.stop();
  id3->RegisterMetadataCB(nullptr, nullptr);
  id3->close();
  file.close();
  delete id3;
  id3 = nullptr;
}

void playMP3(const char* fname)
{
  if (id3 != nullptr) { stopMP3(); }
  M5Cardputer.Display.setCursor(0, 8);
  file.open(fname);
  id3 = new AudioFileSourceID3(&file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  id3->open(fname);
  mp3.begin(id3, &out);
}

void pauseMP3(void){
  paused_at = id3->getPos();
  mp3.stop();
  isPaused = true;
}

void resumeMP3(const char* fname){
  if (id3 != nullptr) { stopMP3(); }
  M5Cardputer.Display.setCursor(0, 8);
  file.open(fname);
  id3 = new AudioFileSourceID3(&file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  id3->open(fname);
  id3->seek(paused_at, 1);
  isPaused = false;
  mp3.begin(id3, &out);
}

void enterMP3Player() {
  // Configure M5 Speaker settings like official M5Unified example
  auto spk_cfg = M5Cardputer.Speaker.config();
  spk_cfg.sample_rate = 96000; // Official example uses 96000 for good quality
  M5Cardputer.Speaker.config(spk_cfg);
  M5Cardputer.Speaker.begin();
  
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("MP3 Player");
  
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.printf("Initializing SD...");
  M5Cardputer.Display.display();
  
  // Initialize SD card with faster SPI speed for better audio quality
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  
  if (!SD.begin(SD_SPI_CS_PIN, SPI, 40000000)) {  // Increased from 25MHz to 40MHz
    M5Cardputer.Display.setCursor(10, 50);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.printf("SD card not found!");
    M5Cardputer.Display.display();
    delay(3000);
    currentState = STATE_MENU;
    return;
  }
  
  // Try to load existing MP3 index first
  if (!mp3Scanned) {
    M5Cardputer.Display.setCursor(10, 50);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.printf("Loading MP3 index...");
    M5Cardputer.Display.display();
    
    if (loadMP3List()) {
      // Successfully loaded from cache
      M5Cardputer.Display.setCursor(10, 70);
      M5Cardputer.Display.setTextColor(GREEN);
      M5Cardputer.Display.printf("Loaded %d cached MP3s", no_of_files);
      M5Cardputer.Display.display();
      delay(1000);
    } else {
      // Need to scan and save
      M5Cardputer.Display.setCursor(10, 70);
      M5Cardputer.Display.setTextColor(GREEN);
      M5Cardputer.Display.printf("First scan - please wait...");
      M5Cardputer.Display.display();
      
      no_of_files = 0;
      stop_scan = false;
      listDir(SD, "/", 2);
      
      // Save the results for next time
      saveMP3List();
      
      M5Cardputer.Display.setCursor(10, 90);
      M5Cardputer.Display.setTextColor(CYAN);
      M5Cardputer.Display.printf("Index saved! Future boots instant");
      M5Cardputer.Display.display();
      delay(2000);
    }
    mp3Scanned = true;
  }
  
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf("MP3 Player");
  
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.printf("Found %d MP3 files", no_of_files);
  
  if (no_of_files > 0) {
    M5Cardputer.Display.setCursor(10, 50);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.printf("Playing: %s", files[0].c_str());
    
    M5Cardputer.Display.setCursor(10, 70);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.printf("Controls:");
    M5Cardputer.Display.setCursor(10, 85);
    M5Cardputer.Display.printf("P=Play/Pause N=Next B=Back");
    M5Cardputer.Display.setCursor(10, 100);
    M5Cardputer.Display.printf("S=Shuffle V=Visual M=Exit ←/→=Vol R=Rescan");
    
    fileindex = 0;
    filetoplay = files[fileindex];
    filetoplay.toCharArray(f, 256);
    playMP3(f);
  } else {
    M5Cardputer.Display.setCursor(10, 50);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.printf("No MP3 files found!");
    M5Cardputer.Display.setCursor(10, 70);
    M5Cardputer.Display.printf("Check file names");
    M5Cardputer.Display.setCursor(10, 85);
    M5Cardputer.Display.printf("Must end with .mp3");
  }
  
  M5Cardputer.Display.display();
  
  // MP3 Player screensaver variables (FIXED: Moved outside while loop)
  unsigned long mp3LastActivityTime = millis();
  unsigned long mp3ScreensaverTimeout = 30000; // 30 seconds
  bool mp3ScreensaverActive = false;
  bool mp3ScreensaverEnabled = false; // NEW: Toggle for screensaver (DEFAULT OFF)
  AnimationMode mp3AnimMode = PLASMA; // Default to plasma for music visualization
  float mp3AnimTime = 0;
  
  // MP3 Player loop
  while (currentState == STATE_MP3_PLAYER) {
    M5Cardputer.update();
    
    // Handle MP3 playback
    if (mp3.isRunning()) {
      if (!mp3.loop()) { 
        mp3.stop(); 
        // Auto-next song
        if (no_of_files > 0) {
          if (isShuffleMode) {
            // Random next song
            fileindex = random(no_of_files);
          } else {
            // Sequential next song
            if (++fileindex >= no_of_files) { fileindex = 0; }
          }
          filetoplay = files[fileindex];
          filetoplay.toCharArray(f, 256);
          playMP3(f);
          
          // Update display with new song (only if not in screensaver mode)
          if (!mp3ScreensaverActive) {
            M5Cardputer.Display.fillRect(10, 50, 220, 15, BLACK);
            M5Cardputer.Display.setCursor(10, 50);
            M5Cardputer.Display.setTextColor(WHITE);
            M5Cardputer.Display.printf("Playing: %s", files[fileindex].c_str());
            M5Cardputer.Display.display();
          }
        }
      }
    }
    
    // Handle keyboard input
    if (M5Cardputer.Keyboard.isChange()) {
      mp3LastActivityTime = millis(); // Reset screensaver timer on any key press
      
      // If exiting screensaver mode, redraw interface
      if (mp3ScreensaverActive) {
        mp3ScreensaverActive = false;
        
        // Redraw MP3 player interface
        M5Cardputer.Display.clear();
        M5Cardputer.Display.setCursor(10, 10);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.printf("MP3 Player");
        M5Cardputer.Display.setCursor(10, 30);
        M5Cardputer.Display.setTextColor(GREEN);
        M5Cardputer.Display.printf("Found %d MP3 files", no_of_files);
        if (no_of_files > 0) {
          M5Cardputer.Display.setCursor(10, 50);
          M5Cardputer.Display.setTextColor(WHITE);
          M5Cardputer.Display.printf("Playing: %s", files[fileindex].c_str());
        }
        M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 15);
        M5Cardputer.Display.setTextColor(GRAY);
        M5Cardputer.Display.printf("S=Shuffle V=Visual M=Exit ←/→=Vol R=Rescan");
        M5Cardputer.Display.display();
      }
      
      size_t v = M5Cardputer.Speaker.getVolume();
      
      if (M5Cardputer.Keyboard.isKeyPressed('p')) {
        if (no_of_files > 0) {
          if (isPaused) {
            filetoplay = files[fileindex];
            filetoplay.toCharArray(f, 256);
            stopMP3();
            resumeMP3(f);
          } else {
            pauseMP3();
          }
        }
      }
      
      if (M5Cardputer.Keyboard.isKeyPressed('n')) {
        if (no_of_files > 0) {
          if (isShuffleMode) {
            fileindex = random(no_of_files);
          } else {
            if (++fileindex >= no_of_files) { fileindex = 0; }
          }
          filetoplay = files[fileindex];
          filetoplay.toCharArray(f, 256);
          playMP3(f);
          
          // Update display
          M5Cardputer.Display.fillRect(10, 50, 220, 15, BLACK);
          M5Cardputer.Display.setCursor(10, 50);
          M5Cardputer.Display.setTextColor(WHITE);
          M5Cardputer.Display.printf("Playing: %s", files[fileindex].c_str());
          M5Cardputer.Display.display();
        }
      }
      
      if (M5Cardputer.Keyboard.isKeyPressed('b')) {
        if (no_of_files > 0) {
          if (isShuffleMode) {
            fileindex = random(no_of_files);
          } else {
            if (--fileindex < 0) { fileindex = no_of_files - 1; }
          }
          filetoplay = files[fileindex];
          filetoplay.toCharArray(f, 256);
          playMP3(f);
          
          // Update display
          M5Cardputer.Display.fillRect(10, 50, 220, 15, BLACK);
          M5Cardputer.Display.setCursor(10, 50);
          M5Cardputer.Display.setTextColor(WHITE);
          M5Cardputer.Display.printf("Playing: %s", files[fileindex].c_str());
          M5Cardputer.Display.display();
        }
      }
      
      // Better volume control with arrow keys
      if (M5Cardputer.Keyboard.isKeyPressed(KEY_LEFT)) {
        if (v >= 10) {
          v -= 10;
          M5Cardputer.Speaker.setVolume(v);
          mp3Volume = (float)v / 255.0f; // Update volume tracker
        }
      } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_RIGHT)) {
        if (v <= 245) {
          v += 10;
          M5Cardputer.Speaker.setVolume(v);
          mp3Volume = (float)v / 255.0f; // Update volume tracker
        }
      }
      
      // Rescan option
      if (M5Cardputer.Keyboard.isKeyPressed('r')) {
        stopMP3();
        mp3Scanned = false; // Force rescan
        no_of_files = 0;
        stop_scan = false;
        
        M5Cardputer.Display.clear();
        M5Cardputer.Display.setCursor(10, 10);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.printf("Rescanning SD card...");
        M5Cardputer.Display.display();
        
        listDir(SD, "/", 2);
        mp3Scanned = true;
        
        // Save the updated index
        saveMP3List();
        
        // Restart if files found
        if (no_of_files > 0) {
          fileindex = 0;
          filetoplay = files[fileindex];
          filetoplay.toCharArray(f, 256);
          playMP3(f);
        }
        
        // Redraw interface
        M5Cardputer.Display.clear();
        M5Cardputer.Display.setCursor(10, 10);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.printf("MP3 Player");
        M5Cardputer.Display.setCursor(10, 30);
        M5Cardputer.Display.setTextColor(GREEN);
        M5Cardputer.Display.printf("Found %d MP3 files", no_of_files);
        if (no_of_files > 0) {
          M5Cardputer.Display.setCursor(10, 50);
          M5Cardputer.Display.setTextColor(WHITE);
          M5Cardputer.Display.printf("Playing: %s", files[0].c_str());
        }
        M5Cardputer.Display.display();
      }
      
      // Toggle shuffle mode
      if (M5Cardputer.Keyboard.isKeyPressed('s')) {
        isShuffleMode = !isShuffleMode;
        M5Cardputer.Display.fillRect(10, 115, 220, 10, BLACK);
        M5Cardputer.Display.setCursor(10, 115);
        M5Cardputer.Display.setTextColor(isShuffleMode ? GREEN : RED);
        M5Cardputer.Display.printf("Shuffle: %s", isShuffleMode ? "ON" : "OFF");
        M5Cardputer.Display.display();
        delay(1000);
        // Clear the status line
        M5Cardputer.Display.fillRect(10, 115, 220, 10, BLACK);
        M5Cardputer.Display.display();
      }
      
      // Toggle MP3 screensaver (NEW: V key)
      if (M5Cardputer.Keyboard.isKeyPressed('v')) {
        mp3ScreensaverEnabled = !mp3ScreensaverEnabled;
        // Force exit screensaver if currently active and being disabled
        if (!mp3ScreensaverEnabled && mp3ScreensaverActive) {
          mp3ScreensaverActive = false;
          // Redraw interface
          M5Cardputer.Display.clear();
          M5Cardputer.Display.setCursor(10, 10);
          M5Cardputer.Display.setTextColor(WHITE);
          M5Cardputer.Display.printf("MP3 Player");
          M5Cardputer.Display.setCursor(10, 30);
          M5Cardputer.Display.setTextColor(GREEN);
          M5Cardputer.Display.printf("Found %d MP3 files", no_of_files);
          if (no_of_files > 0) {
            M5Cardputer.Display.setCursor(10, 50);
            M5Cardputer.Display.setTextColor(WHITE);
            M5Cardputer.Display.printf("Playing: %s", files[fileindex].c_str());
          }
          M5Cardputer.Display.setCursor(10, M5Cardputer.Display.height() - 15);
          M5Cardputer.Display.setTextColor(GRAY);
          M5Cardputer.Display.printf("S=Shuffle V=Visual M=Exit ←/→=Vol R=Rescan");
          M5Cardputer.Display.display();
        }
        // Show toggle status
        M5Cardputer.Display.fillRect(10, 115, 220, 10, BLACK);
        M5Cardputer.Display.setCursor(10, 115);
        M5Cardputer.Display.setTextColor(mp3ScreensaverEnabled ? GREEN : RED);
        M5Cardputer.Display.printf("Screensaver: %s", mp3ScreensaverEnabled ? "ON" : "OFF");
        M5Cardputer.Display.display();
        delay(1000);
        // Clear the status line
        M5Cardputer.Display.fillRect(10, 115, 220, 10, BLACK);
        M5Cardputer.Display.display();
      }
    }
    
    // Screensaver activation after timeout (ONLY if enabled)
    if (mp3ScreensaverEnabled && !mp3ScreensaverActive && (millis() - mp3LastActivityTime > mp3ScreensaverTimeout)) {
      mp3ScreensaverActive = true;
      mp3AnimTime = 0;
      initSimpleMatrix(); // Initialize matrix drops
    }
    
    // Update screensaver animation if active
    if (mp3ScreensaverActive) {
      mp3AnimTime += 0.016f; // Assuming ~60 FPS
      
      // Draw simple matrix rain (much less intensive than plasma)
      drawSimpleMatrix();
      
      // Show minimal info overlay
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.setTextColor(TFT_GREEN);
      M5Cardputer.Display.setCursor(5, 5);
      if (no_of_files > 0) {
        M5Cardputer.Display.printf("♫ %s", files[fileindex].c_str());
      }
      M5Cardputer.Display.setCursor(5, M5Cardputer.Display.height() - 15);
      M5Cardputer.Display.setTextColor(TFT_WHITE);
      M5Cardputer.Display.printf("Any key to wake");
      M5Cardputer.Display.display();
    }
    
    // Return to menu with M key or physical button
    if (M5Cardputer.BtnA.wasPressed() || M5Cardputer.Keyboard.isKeyPressed('m')) {
      stopMP3();
      currentState = STATE_MENU;
      return;
    }
    
    delay(10);
  }
}

// ==================== DEAUTH HUNTER ====================

void enterDeauthHunter() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf("Deauth Hunter");
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.printf("Starting deauth detection...");
  M5Cardputer.Display.display();
  
  deauth_hunter_setup();
  start_deauth_monitoring();
  
  while (true) {
    M5Cardputer.update();
    
    // Exit with M key or ESC
    if (checkEscapeKey()) {
      deauth_hunter_cleanup();
      currentState = STATE_MENU;
      return;
    }
    
    deauth_hunter_loop();
    delay(10);
  }
}

void deauth_hunter_setup() {
  // Reset statistics
  deauth_stats.total_deauths = 0;
  deauth_stats.unique_aps = 0;
  deauth_stats.rssi_sum = 0;
  deauth_stats.rssi_count = 0;
  deauth_stats.avg_rssi = -90;
  deauth_stats.last_reset_time = millis();
  
  current_channel_idx = 0;
  last_channel_change = 0;
  scan_cycle_start = millis();
  deauth_hunter_active = true;
  seen_ap_macs.clear();
}

void start_deauth_monitoring() {
  // Stop any existing WiFi connections
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100);
  
  // Enable promiscuous mode for packet sniffing
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  
  // Set up filter for management frames only
  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(&deauth_sniffer_callback);
  
  // Start on channel 1
  esp_wifi_set_channel(WIFI_CHANNELS[current_channel_idx], WIFI_SECOND_CHAN_NONE);
  
  Serial.println("Deauth Hunter: Monitoring started");
}

void deauth_hunter_loop() {
  uint32_t current_time = millis();
  
  // Channel hopping every 250ms
  if (current_time - last_channel_change > 250) {
    hop_channel();
    last_channel_change = current_time;
  }
  
  // Reset stats every 60 seconds and update display
  if (current_time - deauth_stats.last_reset_time > 60000) {
    reset_stats_if_needed();
    deauth_stats.last_reset_time = current_time;
  }
  
  // Update display every 500ms
  static uint32_t last_display_update = 0;
  if (current_time - last_display_update > 500) {
    updateDeauthDisplay();
    last_display_update = current_time;
  }
}

void hop_channel() {
  current_channel_idx = (current_channel_idx + 1) % NUM_CHANNELS;
  esp_wifi_set_channel(WIFI_CHANNELS[current_channel_idx], WIFI_SECOND_CHAN_NONE);
}

void reset_stats_if_needed() {
  // Keep total stats but reset per-minute stats
  deauth_stats.rssi_sum = 0;
  deauth_stats.rssi_count = 0;
  deauth_stats.avg_rssi = -90;
}

void add_unique_ap(const char* mac) {
  String mac_str = String(mac);
  
  // Check if we've seen this AP before
  for (const String& seen_mac : seen_ap_macs) {
    if (seen_mac.equals(mac_str)) {
      return; // Already seen
    }
  }
  
  // Add new AP
  seen_ap_macs.push_back(mac_str);
  deauth_stats.unique_aps++;
  
  // Limit memory usage
  if (seen_ap_macs.size() > 100) {
    seen_ap_macs.erase(seen_ap_macs.begin());
    deauth_stats.unique_aps = seen_ap_macs.size();
  }
}

int calculate_rssi_bars(int rssi) {
  if (rssi > -50) return 4;
  if (rssi > -60) return 3;
  if (rssi > -70) return 2;
  if (rssi > -80) return 1;
  return 0;
}

void updateDeauthDisplay() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(1);
  
  // Title
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.printf("DEAUTH HUNTER");
  
  // Current channel
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.printf("Channel: %d", WIFI_CHANNELS[current_channel_idx]);
  
  // Statistics
  M5Cardputer.Display.setCursor(10, 50);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf("Deauth Attacks: %d", deauth_stats.total_deauths);
  
  M5Cardputer.Display.setCursor(10, 70);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf("Unique APs: %d", deauth_stats.unique_aps);
  
  // Average RSSI
  if (deauth_stats.rssi_count > 0) {
    deauth_stats.avg_rssi = deauth_stats.rssi_sum / deauth_stats.rssi_count;
  }
  
  M5Cardputer.Display.setCursor(10, 90);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.printf("Avg RSSI: %d dBm", deauth_stats.avg_rssi);
  
  // Signal strength bars
  int bars = calculate_rssi_bars(deauth_stats.avg_rssi);
  M5Cardputer.Display.setCursor(10, 110);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.printf("Signal: ");
  for (int i = 0; i < 4; i++) {
    if (i < bars) {
      M5Cardputer.Display.printf("█");
    } else {
      M5Cardputer.Display.printf("░");
    }
  }
  
  // Instructions
  M5Cardputer.Display.setCursor(10, 130);
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.printf("Press M to exit");
  
  M5Cardputer.Display.display();
}

void deauth_hunter_cleanup() {
  // Stop promiscuous mode
  esp_wifi_set_promiscuous(false);
  deauth_hunter_active = false;
  
  // Reset WiFi to normal mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.println("Deauth Hunter: Monitoring stopped");
}

void extract_mac(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", 
          data[offset], data[offset+1], data[offset+2], 
          data[offset+3], data[offset+4], data[offset+5]);
}

// Promiscuous mode callback - MUST be in IRAM
static void IRAM_ATTR deauth_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!deauth_hunter_active) return;
  
  const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
  const WifiMgmtHdr *frame = (WifiMgmtHdr*)pkt->payload;
  
  // Check if this is a deauth frame (type 12, subtype 0)
  uint16_t frame_control = frame->fctl;
  if ((frame_control & 0xFF) == 0xC0) {  // Deauth frame
    // Update statistics (thread-safe operations only)
    deauth_stats.total_deauths++;
    
    // Add RSSI data
    deauth_stats.rssi_sum += pkt->rx_ctrl.rssi;
    deauth_stats.rssi_count++;
    
    // Extract AP MAC for unique counting (simplified)
    char ap_mac[18];
    extract_mac(ap_mac, (uint8_t*)frame->bssid, 0);
    
    // Note: We can't safely call add_unique_ap from ISR context
    // The unique AP counting will be done in the main loop if needed
  }
}

// ==================== PINEAP HUNTER ====================

void enterPinAPHunter() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf("PinAP Hunter");
  M5Cardputer.Display.setCursor(10, 30);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.printf("Scanning for PineAP devices...");
  M5Cardputer.Display.display();
  
  pineap_hunter_setup();
  
  while (pineap_hunter_active) {
    pineap_hunter_loop();
    
    if (checkEscapeKey()) {
      pineap_hunter_cleanup();
      return;
    }
    
    delay(100);
  }
}

void pineap_hunter_setup() {
  pineap_hunter_active = true;
  pineap_hunter_stats = PineAPHunterStats(); // Reset stats
  
  // Ensure WiFi is in scan mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.println("PinAP Hunter: Monitoring started");
}

void pineap_hunter_loop() {
  M5Cardputer.update();
  
  // Handle input navigation
  handlePinAPHunter();
  
  // Perform WiFi scan periodically
  if (millis() - last_pineap_scan > PINEAP_SCAN_INTERVAL) {
    pineap_scan_and_analyze();
    last_pineap_scan = millis();
  }
  
  // Update display if list changed
  if (pineap_hunter_stats.list_changed) {
    pineap_update_display();
    pineap_hunter_stats.list_changed = false;
  }
}

void handlePinAPHunter() {
  if (!M5Cardputer.Keyboard.isPressed()) return;
  
  int total_items = pineap_hunter_stats.detected_pineaps.size() + 1; // +1 for Back
  
  if (pineap_hunter_stats.view_mode == 0) { // Main list
    if (M5Cardputer.Keyboard.isKeyPressed('w')) {
      pineap_cursor = (pineap_cursor - 1 + total_items) % total_items;
      pineap_hunter_stats.list_changed = true;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('s')) {
      pineap_cursor = (pineap_cursor + 1) % total_items;
      pineap_hunter_stats.list_changed = true;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      if (pineap_cursor < pineap_hunter_stats.detected_pineaps.size()) {
        // Enter SSID view for selected PineAP
        pineap_hunter_stats.selected_bssid_index = pineap_cursor;
        pineap_hunter_stats.view_mode = 2; // SSID list view
        pineap_cursor = 0; // Reset cursor
        pineap_hunter_stats.list_changed = true;
      } else {
        // Back option selected
        pineap_hunter_active = false;
      }
    }
  }
  else if (pineap_hunter_stats.view_mode == 2) { // SSID list view
    const auto& pine = pineap_hunter_stats.detected_pineaps[pineap_hunter_stats.selected_bssid_index];
    int ssid_total_items = pine.essids.size() + 1; // +1 for Back
    
    if (M5Cardputer.Keyboard.isKeyPressed('w')) {
      pineap_cursor = (pineap_cursor - 1 + ssid_total_items) % ssid_total_items;
      pineap_hunter_stats.list_changed = true;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('s')) {
      pineap_cursor = (pineap_cursor + 1) % ssid_total_items;
      pineap_hunter_stats.list_changed = true;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      if (pineap_cursor >= pine.essids.size()) {
        // Back to main list
        pineap_hunter_stats.view_mode = 0;
        pineap_cursor = pineap_hunter_stats.selected_bssid_index;
        pineap_hunter_stats.list_changed = true;
      }
    }
  }
  
  delay(200); // Debounce
}

void pineap_scan_and_analyze() {
  int n = WiFi.scanNetworks();
  
  if (n > 0) {
    Serial.printf("PinAP Hunter: Found %d networks\n", n);
    
    for (int i = 0; i < n; i++) {
      String essid = WiFi.SSID(i);
      String bssid_str = WiFi.BSSIDstr(i);
      int32_t rssi = WiFi.RSSI(i);
      
      // Only process networks with valid ESSIDs
      if (essid.length() > 0 && bssid_str.length() > 0) {
        pineap_add_scan_result(bssid_str, essid, rssi);
      }
    }
    
    pineap_hunter_stats.total_scans++;
    pineap_process_scan_results();
    pineap_maintain_buffer_size();
    
    Serial.printf("PinAP Hunter: Detected %d PineAP devices\n", 
                  pineap_hunter_stats.detected_pineaps.size());
  }
  
  WiFi.scanDelete(); // Free memory
}

void pineap_add_scan_result(const String& bssid_str, const String& essid, int32_t rssi) {
  // Add to scan buffer
  if (pineap_hunter_stats.scan_buffer.find(bssid_str) == pineap_hunter_stats.scan_buffer.end()) {
    pineap_hunter_stats.scan_buffer[bssid_str] = std::vector<SSIDRecord>();
  }
  
  // Check if this ESSID is already recorded for this BSSID
  auto& essid_list = pineap_hunter_stats.scan_buffer[bssid_str];
  bool found = false;
  for (auto& existing_record : essid_list) {
    if (existing_record.essid == essid) {
      // Update RSSI and timestamp when this SSID is seen again
      existing_record.rssi = rssi;
      existing_record.last_seen = millis();
      found = true;
      break;
    }
  }
  
  if (!found) {
    essid_list.push_back(SSIDRecord(essid, rssi));
  }
}

void pineap_process_scan_results() {
  std::vector<PineRecord> new_pineaps;
  
  // Check each BSSID in scan buffer for pineapple behavior
  for (const auto& entry : pineap_hunter_stats.scan_buffer) {
    const String& bssid_str = entry.first;
    const std::vector<SSIDRecord>& ssid_records = entry.second;
    
    if (ssid_records.size() >= ph_alert_ssids) {  // Alert threshold
      PineRecord pine;
      pineap_string_to_bssid(bssid_str, pine.bssid);
      pine.essids = ssid_records;
      pine.last_seen = millis();
      
      // Sort SSIDs by most recent seen first
      std::sort(pine.essids.begin(), pine.essids.end(),
        [](const SSIDRecord& a, const SSIDRecord& b) {
          return a.last_seen > b.last_seen;
        });
      
      new_pineaps.push_back(pine);
    }
  }
  
  // Update detected pineaps list if changed
  if (new_pineaps.size() != pineap_hunter_stats.detected_pineaps.size()) {
    pineap_hunter_stats.list_changed = true;
  } else {
    // Same count, check if any pineap has different SSID count
    for (size_t i = 0; i < new_pineaps.size(); i++) {
      const auto& old_pine = pineap_hunter_stats.detected_pineaps[i];
      const auto& new_pine = new_pineaps[i];
      
      if (memcmp(old_pine.bssid, new_pine.bssid, 6) != 0 ||
          old_pine.essids.size() != new_pine.essids.size()) {
        pineap_hunter_stats.list_changed = true;
        break;
      }
    }
  }
  
  pineap_hunter_stats.detected_pineaps = new_pineaps;
}

void pineap_maintain_buffer_size() {
  // Keep only the most recent 50 BSSID entries to prevent memory issues
  if (pineap_hunter_stats.scan_buffer.size() > 50) {
    auto it = pineap_hunter_stats.scan_buffer.begin();
    std::advance(it, pineap_hunter_stats.scan_buffer.size() - 50);
    pineap_hunter_stats.scan_buffer.erase(pineap_hunter_stats.scan_buffer.begin(), it);
  }
}

void pineap_update_display() {
  switch (pineap_hunter_stats.view_mode) {
    case 0: pineap_draw_main_list(); break;
    case 2: pineap_draw_ssid_list(); break;
  }
}

void pineap_draw_main_list() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf("PinAP Hunter");
  M5Cardputer.Display.setCursor(0, 15);
  
  int total_pineaps = pineap_hunter_stats.detected_pineaps.size();
  int startY = 30;
  
  if (total_pineaps == 0) {
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.printf("No PineAPs detected");
    M5Cardputer.Display.setCursor(0, 30);
    M5Cardputer.Display.printf("Scans: %d", pineap_hunter_stats.total_scans);
  } else {
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.printf("Found %d PineAPs:", total_pineaps);
    
    // Show scrollable list of detected PineAPs
    for (int i = 0; i < total_pineaps && i < 6; i++) {
      M5Cardputer.Display.setCursor(0, startY + (i * 12));
      
      if (pineap_cursor == i) {
        M5Cardputer.Display.setTextColor(BLACK, WHITE);
      } else {
        M5Cardputer.Display.setTextColor(WHITE, BLACK);
      }
      
      const auto& pine = pineap_hunter_stats.detected_pineaps[i];
      String bssid_str = pineap_bssid_to_string(pine.bssid);
      String bssid_short = bssid_str.substring(9); // Show last 8 chars
      M5Cardputer.Display.printf("%s %d", bssid_short.c_str(), pine.essids.size());
    }
  }
  
  // Show Back option
  int backY = startY + (min(total_pineaps, 6) * 12);
  M5Cardputer.Display.setCursor(0, backY);
  if (pineap_cursor >= total_pineaps) {
    M5Cardputer.Display.setTextColor(BLACK, WHITE);
  } else {
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
  }
  M5Cardputer.Display.printf("Back");
  
  M5Cardputer.Display.display();
}

void pineap_draw_ssid_list() {
  if (pineap_hunter_stats.selected_bssid_index >= pineap_hunter_stats.detected_pineaps.size()) {
    pineap_draw_main_list();
    return;
  }
  
  const auto& pine = pineap_hunter_stats.detected_pineaps[pineap_hunter_stats.selected_bssid_index];
  
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextSize(1);
  
  // Header with BSSID
  String bssid_str = pineap_bssid_to_string(pine.bssid);
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.printf("PineAP SSIDs:");
  M5Cardputer.Display.setCursor(0, 15);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.printf("%s", bssid_str.substring(9).c_str());
  
  int total_ssids = pine.essids.size();
  
  // Show scrollable list of SSIDs
  int startY = 30;
  for (int i = 0; i < total_ssids && i < 5; i++) {
    M5Cardputer.Display.setCursor(0, startY + (i * 12));
    
    if (pineap_cursor == i) {
      M5Cardputer.Display.setTextColor(BLACK, WHITE);
    } else {
      M5Cardputer.Display.setTextColor(WHITE, BLACK);
    }
    
    String essid = pine.essids[i].essid;
    if (essid.length() > 16) essid = essid.substring(0, 16);
    M5Cardputer.Display.printf("[%d] %s", pine.essids[i].rssi, essid.c_str());
  }
  
  // Show Back option
  int backY = startY + (min(total_ssids, 5) * 12);
  M5Cardputer.Display.setCursor(0, backY);
  if (pineap_cursor >= total_ssids) {
    M5Cardputer.Display.setTextColor(BLACK, WHITE);
  } else {
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
  }
  M5Cardputer.Display.printf("Back");
  
  M5Cardputer.Display.display();
}

void pineap_hunter_cleanup() {
  pineap_hunter_active = false;
  pineap_hunter_stats.scan_buffer.clear();
  pineap_hunter_stats.detected_pineaps.clear();
  
  Serial.println("PinAP Hunter: Monitoring stopped");
}

String pineap_bssid_to_string(const uint8_t* bssid) {
  char bssid_str[18];
  sprintf(bssid_str, "%02x:%02x:%02x:%02x:%02x:%02x",
          bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(bssid_str);
}

void pineap_string_to_bssid(const String& bssid_str, uint8_t* bssid) {
  sscanf(bssid_str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
         &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
}

// Joke Scroller Implementation
void enterJokeScroller() {
  int jokeIndex = 0;
  bool exitJokes = false;
  bool autoMode = false;
  unsigned long lastAutoTime = 0;
  const unsigned long autoInterval = 6000; // 6 seconds
  
  M5Cardputer.Display.clear();
  
  while (!exitJokes) {
    M5Cardputer.update();
    
    // Check for exit condition
    if (checkEscapeKey()) {
      exitJokes = true;
      break;
    }
    
    // Handle navigation and controls
    if (M5Cardputer.Keyboard.isPressed()) {
      if (M5Cardputer.Keyboard.isKeyPressed('w') || M5Cardputer.Keyboard.isKeyPressed('W')) {
        // Previous joke
        jokeIndex = (jokeIndex - 1 + JOKE_COUNT) % JOKE_COUNT;
        autoMode = false; // Turn off auto mode when manual navigation
        delay(200); // Debounce
      }
      else if (M5Cardputer.Keyboard.isKeyPressed('s') || M5Cardputer.Keyboard.isKeyPressed('S')) {
        // Next joke
        jokeIndex = (jokeIndex + 1) % JOKE_COUNT;
        autoMode = false; // Turn off auto mode when manual navigation
        delay(200); // Debounce
      }
      else if (M5Cardputer.Keyboard.isKeyPressed('a') || M5Cardputer.Keyboard.isKeyPressed('A')) {
        // Toggle auto mode
        autoMode = !autoMode;
        lastAutoTime = millis();
        delay(200); // Debounce
      }
    }
    
    // Auto advance in auto mode - RANDOM selection
    if (autoMode && millis() - lastAutoTime >= autoInterval) {
      jokeIndex = random(0, JOKE_COUNT); // Random joke instead of sequential
      lastAutoTime = millis();
    }
    
    // Display current joke
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
    M5Cardputer.Display.setTextSize(2); // Doubled font size
    
    // Just joke number at top in small font
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(5, 5);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.printf("Joke %d/%d", jokeIndex + 1, JOKE_COUNT);
    
    // Auto indicator - red dot instead of text
    if (autoMode) {
      M5Cardputer.Display.fillCircle(270, 10, 4, RED);
    }
    
    // Display joke starting higher with bigger font
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(5, 25); // Start higher
    M5Cardputer.Display.setTextColor(WHITE);
    
    // Get joke from PROGMEM
    String currentJoke = String((char*)pgm_read_ptr(&jokes[jokeIndex]));
    
    // Better word wrapping for larger font - start higher
    int currentY = 25; // Start higher for more room
    int wordStart = 0;
    const int charWidth = 12; // Approximate width for size 2 font
    const int lineHeight = 18; // Line spacing for size 2 font
    
    for (int i = 0; i <= currentJoke.length(); i++) {
      if (i == currentJoke.length() || currentJoke.charAt(i) == ' ') {
        String word = currentJoke.substring(wordStart, i);
        
        // Check if we need to wrap
        int cursorX = M5Cardputer.Display.getCursorX();
        if (cursorX + (word.length() * charWidth) > M5Cardputer.Display.width() - 10) {
          currentY += lineHeight;
          M5Cardputer.Display.setCursor(5, currentY);
        }
        
        M5Cardputer.Display.print(word);
        if (i < currentJoke.length()) M5Cardputer.Display.print(" ");
        
        wordStart = i + 1;
        
        // Stop if we run out of screen space - more room now
        if (currentY > M5Cardputer.Display.height() - 25) break;
      }
    }
    
    // Simplified controls at bottom
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(5, M5Cardputer.Display.height() - 15);
    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.printf("W=Prev S=Next A=Auto");
    
    M5Cardputer.Display.display();
    delay(50);
  }
  
  // Return to menu
  currentState = STATE_MENU;
}

// Audio Visualizer - Chromatic Cascade audio effects
void enterAudioVisualizer() {
  Serial.println("Starting Audio Visualizer...");
  
  // Constants for Cardputer
  const int SAMPLES = 1024;
  const int READ_LEN = 2 * SAMPLES;
  const int BANDS = 8;
  const int TFT_WIDTH = 240;
  const int TFT_HEIGHT = 135;
  const int NOISE_FLOOR = 1;
  const int AMPLIFIER = 4;
  const int MAGNIFY = 3;
  const int RSHIFT = 13;
  const int RSHIFT2 = 1;
  
  // Audio visualization mode
  enum VisualizerMode {
    MODE_SPECTRUM,
    MODE_PLASMA,
    MODE_CIRCLES,
    MODE_WAVES,
    MODE_SPIRAL,
    MODE_MATRIX_RAIN,
    MODE_WATER_RIPPLES,
    MODE_LIGHTNING_STORM,
    MODE_COUNT
  };
  
  VisualizerMode currentMode = MODE_SPECTRUM;
  
  // Audio buffers and variables
  static int vTemp[2][SAMPLES * 2];
  static uint8_t curbuf = 0;
  static uint16_t colormap[TFT_HEIGHT];
  static float phase = 0;
  static float hue = 0;
  
  // Initialize M5Cardputer microphone using built-in config
  auto mic_cfg = M5Cardputer.Mic.config();
  mic_cfg.sample_rate = 22050;
  mic_cfg.over_sampling = 2;
  mic_cfg.magnification = 16;
  mic_cfg.noise_filter_level = 0;
  M5Cardputer.Mic.config(mic_cfg);
  M5Cardputer.Mic.begin();
  
  // Initialize color map (rainbow gradient)
  for (uint8_t i = 0; i < TFT_HEIGHT; i++) {
    float ratio = (float)i / TFT_HEIGHT;
    float r = sin(ratio * 3.14159) * 255;
    float g = sin(ratio * 3.14159 + 2.094) * 255;  
    float b = sin(ratio * 3.14159 + 4.188) * 255;
    colormap[i] = M5Cardputer.Display.color565((uint8_t)r, (uint8_t)g, (uint8_t)b);
  }
  
  // HSV to RGB conversion function
  auto getColorFromHSV = [](float h, float s, float v) -> uint16_t {
    float c = v * s;
    float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
    float m = v - c;
    
    float r, g, b;
    if (h >= 0 && h < 60) {
      r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
      r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
      r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
      r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
      r = x; g = 0; b = c;
    } else {
      r = c; g = 0; b = x;
    }
    
    r = (r + m) * 255;
    g = (g + m) * 255;
    b = (b + m) * 255;
    
    return M5Cardputer.Display.color565((uint8_t)r, (uint8_t)g, (uint8_t)b);
  };
  
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(5, 5);
  M5Cardputer.Display.print("Audio Visualizer - W/S=Mode M=Exit");
  
  Serial.println("Audio Visualizer initialized, starting main loop");
  
  while (true) {
    M5Cardputer.update();
    
    // Check for exit
    if (checkEscapeKey()) {
      Serial.println("Exiting Audio Visualizer");
      break;
    }
    
    // Mode switching
    if (M5Cardputer.Keyboard.isKeyPressed('w') || M5Cardputer.Keyboard.isKeyPressed('W')) {
      currentMode = (VisualizerMode)((currentMode + 1) % MODE_COUNT);
      delay(200); // Debounce
      Serial.printf("Switched to mode %d\n", currentMode);
    }
    if (M5Cardputer.Keyboard.isKeyPressed('s') || M5Cardputer.Keyboard.isKeyPressed('S')) {
      currentMode = (VisualizerMode)((currentMode - 1 + MODE_COUNT) % MODE_COUNT);
      delay(200); // Debounce
      Serial.printf("Switched to mode %d\n", currentMode);
    }
    
    // Read audio data using M5Cardputer microphone
    int16_t adcBuffer[SAMPLES];
    bool micDataAvailable = M5Cardputer.Mic.record(adcBuffer, SAMPLES, 44100);
    
    if (micDataAvailable) {
      // Calculate DC offset
      int32_t dc = 0;
      for (int i = 0; i < SAMPLES; i++) {
        dc += adcBuffer[i];
      }
      dc /= SAMPLES;
      
      // Prepare data for FFT
      for (int i = 0; i < SAMPLES; i++) {
        vTemp[curbuf][i] = (int)adcBuffer[i] - dc;
        vTemp[curbuf][i + SAMPLES] = 0;
      }
      
      // Apply window and calculate FFT
      Fixed15FFT::apply_window(vTemp[curbuf]);
      Fixed15FFT::calc_fft(vTemp[curbuf], vTemp[curbuf] + SAMPLES);
      
      // Calculate frequency bands
      int bands[BANDS] = {0};
      for (int i = 2; i < (SAMPLES/2); i++) {
        int ampsq = vTemp[curbuf][i] * vTemp[curbuf][i] + vTemp[curbuf][i + SAMPLES] * vTemp[curbuf][i + SAMPLES];
        if (ampsq > NOISE_FLOOR) {
          int bandNum = 8; // Default to invalid
          if (i >= 2   && i < 4  ) bandNum = 0;
          if (i >= 4   && i < 8  ) bandNum = 1;
          if (i >= 8   && i < 16 ) bandNum = 2;
          if (i >= 16  && i < 32 ) bandNum = 3;
          if (i >= 32  && i < 64 ) bandNum = 4;
          if (i >= 64  && i < 128) bandNum = 5;
          if (i >= 128 && i < 256) bandNum = 6;
          if (i >= 256 && i < 512) bandNum = 7;
          
          if (bandNum < BANDS && ampsq > bands[bandNum]) {
            bands[bandNum] = ampsq;
          }
        }
      }
      
      // Clear screen for new frame (except title area)
      M5Cardputer.Display.fillRect(0, 15, TFT_WIDTH, TFT_HEIGHT - 15, BLACK);
      
      // Draw visualization based on current mode
      switch (currentMode) {
        case MODE_SPECTRUM: {
          // Classic spectrum analyzer bars with enhanced sensitivity
          int barWidth = TFT_WIDTH / BANDS;
          for (int band = 0; band < BANDS; band++) {
            if (bands[band] > 0) {
              int log2val = 0;
              int temp = bands[band];
              while (temp > 1) {
                temp >>= 1;
                log2val++;
              }
              log2val -= RSHIFT;
              
              if (log2val > -AMPLIFIER) {
                int height = ((log2val + AMPLIFIER) * MAGNIFY * 4) >> RSHIFT2; // 4x more sensitive
                height = constrain(height, 0, TFT_HEIGHT - 20);
                
                for (int y = 0; y < height; y += 2) {
                  int yPos = TFT_HEIGHT - y - 2;
                  // Rainbow spectrum colors instead of colormap
                  float bandHue = (band * 45.0f + millis() * 0.05f);
                  if (bandHue >= 360) bandHue = fmod(bandHue, 360);
                  float brightness = 0.5f + (y / (float)height) * 0.5f;
                  uint16_t color = getColorFromHSV(bandHue, 1.0f, brightness);
                  
                  M5Cardputer.Display.fillRect(band * barWidth + 1, yPos, barWidth - 2, 2, color);
                }
              }
            }
          }
          break;
        }
        
        case MODE_PLASMA: {
          // Audio-reactive plasma effect
          float audioLevel = 0;
          for (int i = 0; i < BANDS; i++) {
            if (bands[i] > 0) audioLevel += sqrt(bands[i]);
          }
          audioLevel = constrain(audioLevel / 1000.0, 0.2, 3.0);
          
          int centerX = TFT_WIDTH / 2;
          int centerY = TFT_HEIGHT / 2;
          
          for (int y = 20; y < TFT_HEIGHT - 5; y += 4) {
            for (int x = 5; x < TFT_WIDTH - 5; x += 4) {
              float dx = (x - centerX) * 0.03;
              float dy = (y - centerY) * 0.03;
              
              float value = sin(dx + phase) + cos(dy + phase * 0.7) + sin((dx + dy) * 0.5 + phase * 1.3);
              value *= audioLevel;
              
              float colorHue = fmod((value * 60) + phase * 30 + hue, 360);
              uint16_t color = getColorFromHSV(colorHue, 1.0, constrain(audioLevel * 0.8, 0.3, 1.0));
              
              M5Cardputer.Display.fillRect(x, y, 3, 3, color);
            }
          }
          break;
        }
        
        case MODE_CIRCLES: {
          // Audio-reactive circles
          int centerX = TFT_WIDTH / 2;
          int centerY = TFT_HEIGHT / 2;
          
          for (int band = 0; band < BANDS; band++) {
            if (bands[band] > 0) {
              float intensity = sqrt(bands[band]) / 50.0;
              intensity = constrain(intensity, 2, 40);
              
              int radius = 5 + intensity + band * 6;
              uint16_t color = getColorFromHSV(hue + band * 45, 1.0, 0.8);
              M5Cardputer.Display.drawCircle(centerX, centerY, radius, color);
            }
          }
          break;
        }
        
        case MODE_WAVES: {
          // Audio-reactive sine waves
          int centerY = TFT_HEIGHT / 2;
          for (int band = 0; band < min(4, BANDS); band++) {
            if (bands[band] > 0) {
              float amplitude = sqrt(bands[band]) / 100.0;
              amplitude = constrain(amplitude, 2, 20);
              
              uint16_t color = getColorFromHSV(hue + band * 90, 1.0, 0.8);
              uint16_t lastY = centerY + amplitude * sin(phase + band * 0.5);
              
              for (int x = 1; x < TFT_WIDTH - 1; x++) {
                uint16_t currentY = centerY + amplitude * sin(x * 0.05 + phase + band * 0.5);
                currentY = constrain(currentY, 20, TFT_HEIGHT - 5);
                lastY = constrain(lastY, 20, TFT_HEIGHT - 5);
                
                M5Cardputer.Display.drawLine(x - 1, lastY, x, currentY, color);
                lastY = currentY;
              }
            }
          }
          break;
        }
        
        case MODE_SPIRAL: {
          // Audio-reactive spiral
          int centerX = TFT_WIDTH / 2;
          int centerY = TFT_HEIGHT / 2;
          
          float audioIntensity = 0;
          for (int i = 0; i < BANDS; i++) {
            if (bands[i] > 0) audioIntensity += sqrt(bands[i]);
          }
          audioIntensity = constrain(audioIntensity / 500.0, 1, 10);
          
          float lastX = centerX, lastY = centerY;
          for (float angle = 0; angle < 6 * 3.14159; angle += 0.2) {
            float radius = audioIntensity * (1 + sin(angle * 0.3 + phase));
            float x = centerX + radius * cos(angle + phase * 0.5);
            float y = centerY + radius * sin(angle + phase * 0.5);
            
            if (x >= 0 && x < TFT_WIDTH && y >= 20 && y < TFT_HEIGHT && angle > 0.2) {
              uint16_t color = getColorFromHSV(fmod(angle * 57.3 + hue, 360), 1.0, 0.8);
              M5Cardputer.Display.drawLine(lastX, lastY, x, y, color);
            }
            lastX = x;
            lastY = y;
          }
          break;
        }
        
        case MODE_MATRIX_RAIN: {
          // Matrix digital rain effect
          static char matrix[30][20];
          static float positions[30] = {0};
          static float speeds[30] = {0};
          static int lengths[30] = {0};
          static bool matrix_initialized = false;
          
          if (!matrix_initialized) {
            for (int col = 0; col < 30; col++) {
              speeds[col] = 0.5 + (rand() % 3) * 0.5;
              lengths[col] = 5 + rand() % 10;
              positions[col] = -(lengths[col] * 8);
              for (int row = 0; row < 20; row++) {
                if (rand() % 4 == 0) {
                  matrix[col][row] = '0' + rand() % 10;
                } else if (rand() % 4 == 1) {
                  matrix[col][row] = 'A' + rand() % 26;
                } else {
                  char matrixChars[] = {'@', '#', '$', '%', '&', '*', '+', '='};
                  matrix[col][row] = matrixChars[rand() % 8];
                }
              }
            }
            matrix_initialized = true;
          }
          
          float audioSpeed = 1.0;
          for (int i = 0; i < BANDS; i++) {
            if (bands[i] > 0) audioSpeed += sqrt(bands[i]) / 200.0;
          }
          audioSpeed = constrain(audioSpeed, 0.5, 4.0);
          
          for (int col = 0; col < 30; col++) {
            int x = col * 8;
            if (x >= TFT_WIDTH) continue;
            
            positions[col] += speeds[col] * audioSpeed;
            
            if (positions[col] > TFT_HEIGHT + lengths[col] * 8) {
              positions[col] = -(lengths[col] * 8);
            }
            
            for (int i = 0; i < lengths[col]; i++) {
              int y = positions[col] + i * 8;
              if (y >= 20 && y < TFT_HEIGHT - 8) {
                float brightness = constrain(1.0 - (i / (float)lengths[col]), 0.2, 1.0);
                uint16_t color = getColorFromHSV(120, 1.0, brightness);
                
                M5Cardputer.Display.setTextColor(color);
                M5Cardputer.Display.setCursor(x, y);
                M5Cardputer.Display.print(matrix[col][i % 20]);
              }
            }
          }
          break;
        }
        
        case MODE_WATER_RIPPLES: {
          // Water ripples effect
          static float ripples[10][3]; // x, y, age
          static bool ripples_initialized = false;
          
          if (!ripples_initialized) {
            for (int i = 0; i < 10; i++) {
              ripples[i][2] = -1; // inactive
            }
            ripples_initialized = true;
          }
          
          // Create new ripples based on audio
          for (int band = 0; band < BANDS; band++) {
            if (bands[band] > 0) {
              float intensity = sqrt(bands[band]) / 50.0;
              if (intensity > 15 && rand() % 100 < 20) {
                for (int i = 0; i < 10; i++) {
                  if (ripples[i][2] < 0) {
                    ripples[i][0] = 40 + rand() % (TFT_WIDTH - 80);
                    ripples[i][1] = 40 + rand() % (TFT_HEIGHT - 60);
                    ripples[i][2] = 0; // age
                    break;
                  }
                }
              }
            }
          }
          
          // Update and draw ripples
          for (int i = 0; i < 10; i++) {
            if (ripples[i][2] >= 0) {
              float age = ripples[i][2];
              float radius = age * 3;
              float brightness = constrain(1.0 - age / 30.0, 0.1, 1.0);
              
              if (radius < 60 && brightness > 0.1) {
                uint16_t color = getColorFromHSV(hue + i * 36, 0.8, brightness);
                M5Cardputer.Display.drawCircle(ripples[i][0], ripples[i][1], radius, color);
                if (radius > 5) {
                  M5Cardputer.Display.drawCircle(ripples[i][0], ripples[i][1], radius - 2, color);
                }
              }
              
              ripples[i][2] += 1.2;
              if (ripples[i][2] > 35) {
                ripples[i][2] = -1; // deactivate
              }
            }
          }
          break;
        }
        
        case MODE_LIGHTNING_STORM: {
          // Lightning storm effect
          static float lightning[10][5]; // x1, y1, x2, y2, life
          static bool lightning_initialized = false;
          static int flashTimer = 0;
          static bool isFlashing = false;
          
          if (!lightning_initialized) {
            for (int i = 0; i < 10; i++) {
              lightning[i][4] = -1; // inactive
            }
            lightning_initialized = true;
          }
          
          // Focus on bass frequencies for lightning triggers
          float bassLevel = 0;
          for (int i = 0; i < 3; i++) {
            if (bands[i] > 0) bassLevel += sqrt(bands[i]);
          }
          
          // Screen flash effect for strong bass
          if (bassLevel > 100 && !isFlashing) {
            isFlashing = true;
            flashTimer = 15;
          }
          
          if (isFlashing) {
            M5Cardputer.Display.fillRect(0, 15, TFT_WIDTH, TFT_HEIGHT - 15, WHITE);
            flashTimer--;
            if (flashTimer <= 0) {
              isFlashing = false;
            }
          }
          
          // Create new lightning bolts on strong bass
          if (bassLevel > 80 && rand() % 100 < 40) {
            for (int i = 0; i < 10; i++) {
              if (lightning[i][4] < 0) {
                lightning[i][0] = 20 + rand() % (TFT_WIDTH - 40); // x1 (top)
                lightning[i][1] = 20; // y1 (top)
                lightning[i][2] = lightning[i][0] + (rand() % 80) - 40; // x2 (bottom, jagged)
                lightning[i][3] = (TFT_HEIGHT/2) + rand() % (TFT_HEIGHT/2 - 10); // y2 (bottom)
                lightning[i][4] = 20 + rand() % 20; // life
                break;
              }
            }
          }
          
          // Update and draw lightning bolts
          for (int i = 0; i < 10; i++) {
            if (lightning[i][4] > 0) {
              float life = lightning[i][4];
              float brightness = constrain(life / 30.0, 0.3, 1.0);
              
              uint16_t boltColor;
              if (isFlashing) {
                boltColor = M5Cardputer.Display.color565(0, 0, 255); // Blue on white
              } else {
                boltColor = getColorFromHSV(60 - life, 0.8, brightness); // Blue to yellow
              }
              
              // Draw main bolt
              M5Cardputer.Display.drawLine(lightning[i][0], lightning[i][1], 
                                         lightning[i][2], lightning[i][3], boltColor);
              
              // Add branches for realism
              if (life > 15) {
                int midX = (lightning[i][0] + lightning[i][2]) / 2;
                int midY = (lightning[i][1] + lightning[i][3]) / 2;
                int branchX = midX + (rand() % 60) - 30;
                int branchY = midY + 10 + rand() % 30;
                
                if (branchX >= 0 && branchX < TFT_WIDTH && branchY < TFT_HEIGHT) {
                  M5Cardputer.Display.drawLine(midX, midY, branchX, branchY, boltColor);
                }
              }
              
              lightning[i][4] -= 2;
            }
          }
          break;
        }
      }
      
      // Update animation variables
      phase += 0.1;
      hue += 1;
      if (hue >= 360) hue = 0;
      
      curbuf ^= 1;
    }
    
    delay(20); // ~50fps
  }
  
  // Cleanup I2S
  i2s_driver_uninstall(I2S_NUM_0);
  Serial.println("I2S driver uninstalled");
  
  // Return to menu
  currentState = STATE_MENU;
}

// Additional screensaver effects from XScreensaver

void drawXjack() {
  static float jackTime = 0;
  jackTime += 0.05;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // "All work and no play makes Jack a dull boy" typing effect
  const char* jackText[] = {
    "All work and no play makes Jack a dull boy.",
    "All work and no play makes Jack a dull boy.",
    "All WORK and no PLAY makes Jack a dull boy.",
    "ALL WORK AND NO PLAY MAKES JACK A DULL BOY!",
    "All work and no play makes Jack a dull boy.",
    "All work and no play makes Jack a dull boy."
  };
  int numLines = sizeof(jackText) / sizeof(jackText[0]);
  
  // Typing animation
  float typingSpeed = jackTime * 10;
  int currentLine = (int)(typingSpeed / 50) % numLines;
  int currentChar = ((int)typingSpeed % 50);
  
  M5Cardputer.Display.setTextSize(1);
  
  // Draw previous lines
  for (int line = 0; line < currentLine && line < 10; line++) {
    M5Cardputer.Display.setCursor(5, 10 + line * 12);
    
    // Glitch effect on random lines
    if (sin(jackTime * 3 + line) > 0.8) {
      M5Cardputer.Display.setTextColor(RED); // Red glitch
    } else {
      M5Cardputer.Display.setTextColor(0x7BEF); // Light gray
    }
    
    int lineIndex = line % numLines;
    M5Cardputer.Display.printf("%s", jackText[lineIndex]);
  }
  
  // Draw current line being typed
  if (currentLine < 10) {
    M5Cardputer.Display.setCursor(5, 10 + currentLine * 12);
    M5Cardputer.Display.setTextColor(YELLOW); // Bright typing
    
    char buffer[50];
    int lineLength = strlen(jackText[currentLine % numLines]);
    int charsToShow = min(currentChar, lineLength);
    
    strncpy(buffer, jackText[currentLine % numLines], charsToShow);
    buffer[charsToShow] = '\0';
    M5Cardputer.Display.printf("%s", buffer);
    
    // Blinking cursor
    if (((int)(jackTime * 8)) % 2 == 0) {
      M5Cardputer.Display.printf("_");
    }
  }
}

void drawCritical() {
  static float criticalTime = 0;
  criticalTime += 0.1;
  
  // Flashing red background for urgency
  bool flash = (int)(criticalTime * 8) % 2 == 0;
  if (flash) {
    M5Cardputer.Display.fillScreen(0x1800); // Dark red
  } else {
    M5Cardputer.Display.fillScreen(BLACK);
  }
  
  // Warning text
  M5Cardputer.Display.setTextColor(flash ? WHITE : RED);
  M5Cardputer.Display.setTextSize(2);
  
  int textY = 20;
  M5Cardputer.Display.setCursor(20, textY);
  M5Cardputer.Display.printf("CRITICAL");
  
  textY += 25;
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(10, textY);
  M5Cardputer.Display.printf("SYSTEM FAILURE IMMINENT");
  
  textY += 15;
  M5Cardputer.Display.setCursor(10, textY);
  M5Cardputer.Display.printf("Temperature: %d C", 150 + (int)(sin(criticalTime * 4) * 50));
  
  textY += 15;
  M5Cardputer.Display.setCursor(10, textY);
  M5Cardputer.Display.printf("Memory: %d%% FULL", 95 + (int)(sin(criticalTime * 2) * 5));
  
  textY += 15;
  M5Cardputer.Display.setCursor(10, textY);
  M5Cardputer.Display.printf("Disk: %d%% FULL", 98 + (int)(sin(criticalTime * 3) * 2));
  
  // Fake error codes scrolling
  textY += 20;
  M5Cardputer.Display.setTextColor(0x07E0); // Green
  for (int i = 0; i < 3; i++) {
    M5Cardputer.Display.setCursor(10, textY + i * 10);
    int errorCode = 0x8000 + (int)(criticalTime * 100 + i * 123) % 0x1000;
    M5Cardputer.Display.printf("ERR 0x%04X", errorCode);
  }
}

void drawSphere() {
  static float sphereTime = 0;
  sphereTime += 0.06;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  int centerX = 240 / 2;
  int centerY = 135 / 2;
  
  // 3D sphere with latitude/longitude lines
  float rotX = sphereTime * 0.8;
  float rotY = sphereTime * 1.2;
  
  int sphereRadius = 50;
  
  // Draw latitude lines
  for (int lat = -3; lat <= 3; lat++) {
    float latAngle = lat * PI / 6;
    float latRadius = cos(latAngle) * sphereRadius;
    float latY = sin(latAngle) * sphereRadius;
    
    // Color based on depth
    uint16_t lineColor = M5Cardputer.Display.color565(
      100 + abs(lat) * 20,
      150 + abs(lat) * 15,
      200 + abs(lat) * 10
    );
    
    // Draw longitude segments
    for (int segment = 0; segment < 24; segment++) {
      float lon1 = segment * 2 * PI / 24;
      float lon2 = (segment + 1) * 2 * PI / 24;
      
      // Calculate 3D points
      float x1 = cos(lon1) * latRadius;
      float z1 = sin(lon1) * latRadius;
      float x2 = cos(lon2) * latRadius;
      float z2 = sin(lon2) * latRadius;
      
      // Apply rotation
      float rx1 = x1 * cos(rotY) - z1 * sin(rotY);
      float rz1 = x1 * sin(rotY) + z1 * cos(rotY);
      float ry1 = latY * cos(rotX) - rz1 * sin(rotX);
      
      float rx2 = x2 * cos(rotY) - z2 * sin(rotY);
      float rz2 = x2 * sin(rotY) + z2 * cos(rotY);
      float ry2 = latY * cos(rotX) - rz2 * sin(rotX);
      
      // Project to screen (simple perspective)
      if (rz1 > -sphereRadius && rz2 > -sphereRadius) {
        int screenX1 = centerX + (int)rx1;
        int screenY1 = centerY + (int)ry1;
        int screenX2 = centerX + (int)rx2;
        int screenY2 = centerY + (int)ry2;
        
        M5Cardputer.Display.drawLine(screenX1, screenY1, screenX2, screenY2, lineColor);
      }
    }
  }
}

void drawEpicycle() {
  static float epicycleTime = 0;
  epicycleTime += 0.08;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  int centerX = 240 / 2;
  int centerY = 135 / 2;
  
  // Multiple epicycles (circles rolling on circles)
  float radius1 = 40;
  float radius2 = 20;
  float radius3 = 10;
  
  // Main circle
  float angle1 = epicycleTime;
  int x1 = centerX + radius1 * cos(angle1);
  int y1 = centerY + radius1 * sin(angle1);
  
  M5Cardputer.Display.drawCircle(centerX, centerY, radius1, BLUE);
  
  // Second epicycle
  float angle2 = epicycleTime * 3;
  int x2 = x1 + radius2 * cos(angle2);
  int y2 = y1 + radius2 * sin(angle2);
  
  M5Cardputer.Display.drawCircle(x1, y1, radius2, GREEN);
  
  // Third epicycle
  float angle3 = epicycleTime * -5;
  int x3 = x2 + radius3 * cos(angle3);
  int y3 = y2 + radius3 * sin(angle3);
  
  M5Cardputer.Display.drawCircle(x2, y2, radius3, RED);
  
  // Draw connecting lines
  M5Cardputer.Display.drawLine(centerX, centerY, x1, y1, WHITE);
  M5Cardputer.Display.drawLine(x1, y1, x2, y2, WHITE);
  M5Cardputer.Display.drawLine(x2, y2, x3, y3, WHITE);
  
  // Draw trail of the final point
  static int trailX[50], trailY[50];
  static int trailIndex = 0;
  
  trailX[trailIndex] = x3;
  trailY[trailIndex] = y3;
  trailIndex = (trailIndex + 1) % 50;
  
  // Draw trail with fading colors
  for (int i = 0; i < 50; i++) {
    int age = (trailIndex - i + 50) % 50;
    uint8_t brightness = (50 - age) * 5;
    uint16_t trailColor = M5Cardputer.Display.color565(brightness, brightness/2, brightness);
    M5Cardputer.Display.drawPixel(trailX[i], trailY[i], trailColor);
  }
  
  // Draw current point
  M5Cardputer.Display.fillCircle(x3, y3, 3, YELLOW);
}

void drawFlow() {
  static float flowTime = 0;
  flowTime += 0.05;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Flowing particle system
  int numParticles = 20;
  
  for (int i = 0; i < numParticles; i++) {
    // Each particle follows a sine wave path
    float phase = flowTime + i * 0.3;
    float x = fmod(phase * 20, 240 + 40) - 20; // Wrap around screen
    float y = 135/2 + sin(phase * 2 + i) * 40;
    
    // Color changes based on position and time
    uint8_t hue = ((int)(phase * 50 + i * 30)) % 360;
    uint8_t r = (sin(hue * PI / 180) + 1) * 127;
    uint8_t g = (sin((hue + 120) * PI / 180) + 1) * 127;
    uint8_t b = (sin((hue + 240) * PI / 180) + 1) * 127;
    
    uint16_t color = M5Cardputer.Display.color565(r, g, b);
    
    // Draw particle with trail
    if (x >= 0 && x < 240 && y >= 0 && y < 135) {
      M5Cardputer.Display.fillCircle(x, y, 3, color);
      
      // Add some trailing dots
      for (int trail = 1; trail <= 3; trail++) {
        int trailX = x - trail * 8;
        if (trailX >= 0 && trailX < 240) {
          uint16_t fadeColor = M5Cardputer.Display.color565(r/trail/2, g/trail/2, b/trail/2);
          M5Cardputer.Display.drawPixel(trailX, y, fadeColor);
        }
      }
    }
  }
  
  // Add some vertical flow lines
  for (int x = 0; x < 240; x += 30) {
    float linePhase = flowTime * 2 + x * 0.01;
    for (int y = 0; y < 135; y += 5) {
      float brightness = (sin(linePhase + y * 0.1) + 1) * 0.3;
      uint16_t lineColor = M5Cardputer.Display.color565(
        brightness * 100,
        brightness * 150,
        brightness * 255
      );
      M5Cardputer.Display.drawPixel(x, y, lineColor);
    }
  }
}

void drawGlmatrix() {
  static char matrix[30][15];
  static int drops[30];
  static bool initialized = false;
  
  if (!initialized) {
    for (int x = 0; x < 30; x++) {
      drops[x] = random(135);
      for (int y = 0; y < 15; y++) {
        matrix[x][y] = random(32, 127);
      }
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 15; y++) {
      uint16_t color = GREEN;
      if (y == drops[x] / 9) color = 0x07FF; // Bright green
      else if (abs(y - drops[x] / 9) < 3) color = 0x03E0; // Medium green
      
      M5Cardputer.Display.setCursor(x * 8, y * 9);
      M5Cardputer.Display.setTextColor(color);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.print((char)matrix[x][y]);
      
      if (random(20) == 0) matrix[x][y] = random(32, 127);
    }
    
    drops[x]++;
    if (drops[x] * 9 > 135 && random(100) < 10) {
      drops[x] = 0;
    }
  }
}

void drawXmatrix() {
  static struct {
    int x, y, speed;
    char chars[20];
    uint16_t color;
  } streams[12];
  static bool initialized = false;
  
  if (!initialized) {
    for (int i = 0; i < 12; i++) {
      streams[i].x = random(240);
      streams[i].y = random(-200, 0);
      streams[i].speed = random(2, 6);
      streams[i].color = random(2) ? GREEN : 0x07E0;
      for (int j = 0; j < 20; j++) {
        streams[i].chars[j] = random(48, 90); // Numbers and letters
      }
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 20; j++) {
      int drawY = streams[i].y + j * 8;
      if (drawY >= 0 && drawY < 135) {
        uint16_t color = streams[i].color;
        if (j < 3) color = 0x07FF; // Bright head
        else if (j < 8) color = streams[i].color;
        else color = 0x0320; // Dim tail
        
        M5Cardputer.Display.setCursor(streams[i].x, drawY);
        M5Cardputer.Display.setTextColor(color);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.print((char)streams[i].chars[j]);
      }
    }
    
    streams[i].y += streams[i].speed;
    if (streams[i].y > 155) {
      streams[i].y = random(-200, -50);
      streams[i].x = random(240);
      streams[i].speed = random(2, 6);
    }
    
    if (random(30) == 0) {
      streams[i].chars[random(20)] = random(48, 90);
    }
  }
}

void drawBinaryhorizon() {
  static int offset = 0;
  static uint8_t binary[30][17];
  static bool initialized = false;
  
  if (!initialized) {
    for (int x = 0; x < 30; x++) {
      for (int y = 0; y < 17; y++) {
        binary[x][y] = random(2);
      }
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Draw flowing binary horizon
  for (int x = 0; x < 30; x++) {
    for (int y = 0; y < 17; y++) {
      uint16_t color;
      int brightness = 255 - (y * 15);
      if (binary[x][y]) {
        color = M5Cardputer.Display.color565(0, brightness, brightness/2);
      } else {
        color = M5Cardputer.Display.color565(0, brightness/4, 0);
      }
      
      M5Cardputer.Display.setCursor(x * 8, y * 8);
      M5Cardputer.Display.setTextColor(color);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.print(binary[x][y] ? '1' : '0');
    }
  }
  
  // Scroll effect
  offset++;
  if (offset % 3 == 0) {
    for (int x = 0; x < 29; x++) {
      for (int y = 0; y < 17; y++) {
        binary[x][y] = binary[x+1][y];
      }
    }
    for (int y = 0; y < 17; y++) {
      binary[29][y] = random(2);
    }
  }
}

void drawBinaryring() {
  static float angle = 0;
  static struct {
    float r, theta, speed;
    uint8_t bit;
    uint16_t color;
  } bits[60];
  static bool initialized = false;
  
  if (!initialized) {
    for (int i = 0; i < 60; i++) {
      bits[i].r = 30 + random(40);
      bits[i].theta = (i * 6) * PI / 180;
      bits[i].speed = 0.02 + random(50) / 1000.0;
      bits[i].bit = random(2);
      bits[i].color = random(2) ? CYAN : BLUE;
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  int centerX = 120;
  int centerY = 67;
  
  for (int i = 0; i < 60; i++) {
    int x = centerX + bits[i].r * cos(bits[i].theta + angle);
    int y = centerY + bits[i].r * sin(bits[i].theta + angle);
    
    if (x >= 0 && x < 240 && y >= 0 && y < 135) {
      M5Cardputer.Display.setCursor(x-4, y-4);
      M5Cardputer.Display.setTextColor(bits[i].color);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.print(bits[i].bit ? '1' : '0');
    }
    
    bits[i].theta += bits[i].speed;
    if (random(100) < 3) bits[i].bit = random(2);
  }
  
  angle += 0.01;
}

void drawSwirl() {
  static float angle = 0;
  static float radius = 20;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  int centerX = 240 / 2;
  int centerY = 135 / 2;
  
  for (int i = 0; i < 50; i++) {
    float a = angle + i * 0.2;
    float r = radius + i * 2;
    int x = centerX + r * cos(a);
    int y = centerY + r * sin(a);
    
    if (x >= 0 && x < 240 && y >= 0 && y < 135) {
      uint16_t color = M5Cardputer.Display.color565(
        (sin(a) + 1) * 127,
        (cos(a * 1.3) + 1) * 127,
        (sin(a * 0.7) + 1) * 127
      );
      M5Cardputer.Display.drawPixel(x, y, color);
    }
  }
  
  angle += 0.05;
  if (angle > PI * 2) angle = 0;
}

void drawBlaster() {
  static int stars[50][4]; // x, y, z, color
  static bool initialized = false;
  
  if (!initialized) {
    for (int i = 0; i < 50; i++) {
      stars[i][0] = random(-240, 240); // x
      stars[i][1] = random(-135, 135); // y
      stars[i][2] = random(1, 100); // z depth
      stars[i][3] = random(0, 3); // color type
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int i = 0; i < 50; i++) {
    // Project 3D to 2D
    int x = 240/2 + (stars[i][0] * 100) / stars[i][2];
    int y = 135/2 + (stars[i][1] * 100) / stars[i][2];
    
    if (x >= 0 && x < 240 && y >= 0 && y < 135) {
      uint16_t color = WHITE;
      switch (stars[i][3]) {
        case 0: color = RED; break;
        case 1: color = GREEN; break;
        case 2: color = BLUE; break;
        default: color = WHITE; break;
      }
      
      int size = 100 / stars[i][2];
      if (size < 1) size = 1;
      M5Cardputer.Display.fillCircle(x, y, size, color);
    }
    
    // Move stars forward
    stars[i][2] -= 2;
    if (stars[i][2] <= 0) {
      stars[i][0] = random(-240, 240);
      stars[i][1] = random(-135, 135);
      stars[i][2] = 100;
      stars[i][3] = random(0, 3);
    }
  }
}

void drawBlocktube() {
  static float rotation = 0;
  static int blocks[30][3]; // x, y, color
  static bool initialized = false;
  
  if (!initialized) {
    for (int i = 0; i < 30; i++) {
      blocks[i][0] = random(240);
      blocks[i][1] = random(135);
      blocks[i][2] = random(0, 6); // color index
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  uint16_t colors[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA};
  
  for (int i = 0; i < 30; i++) {
    // Rotating block effect
    float angle = rotation + i * 0.2;
    int size = 5 + sin(angle) * 3;
    
    int x = blocks[i][0] + cos(angle) * 10;
    int y = blocks[i][1] + sin(angle) * 10;
    
    if (x >= 0 && x < 240-size && y >= 0 && y < 135-size) {
      M5Cardputer.Display.fillRect(x, y, size, size, colors[blocks[i][2]]);
    }
    
    // Move blocks
    blocks[i][1] += 1;
    if (blocks[i][1] > 135) {
      blocks[i][1] = -10;
      blocks[i][0] = random(240);
      blocks[i][2] = random(0, 6);
    }
  }
  
  rotation += 0.1;
}

void drawBouboule() {
  static float time = 0;
  static int bubbles[20][3]; // x, y, size
  static bool initialized = false;
  
  if (!initialized) {
    for (int i = 0; i < 20; i++) {
      bubbles[i][0] = random(240);
      bubbles[i][1] = random(135);
      bubbles[i][2] = random(5, 20);
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int i = 0; i < 20; i++) {
    float pulse = sin(time + i * 0.5) * 0.5 + 0.5;
    int size = bubbles[i][2] * pulse;
    
    uint16_t color = M5Cardputer.Display.color565(
      pulse * 255,
      (1 - pulse) * 255,
      sin(time + i) * 127 + 127
    );
    
    M5Cardputer.Display.drawCircle(bubbles[i][0], bubbles[i][1], size, color);
    
    // Floating effect
    bubbles[i][1] -= 1;
    if (bubbles[i][1] < -bubbles[i][2]) {
      bubbles[i][1] = 135 + bubbles[i][2];
      bubbles[i][0] = random(240);
      bubbles[i][2] = random(5, 20);
    }
  }
  
  time += 0.1;
}

void drawEnergystream() {
  static int streams[10][4]; // x, y, length, color
  static bool initialized = false;
  
  if (!initialized) {
    for (int i = 0; i < 10; i++) {
      streams[i][0] = random(240);
      streams[i][1] = random(135);
      streams[i][2] = random(10, 30);
      streams[i][3] = random(0, 3);
    }
    initialized = true;
  }
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  for (int i = 0; i < 10; i++) {
    uint16_t color = CYAN;
    switch (streams[i][3]) {
      case 0: color = CYAN; break;
      case 1: color = YELLOW; break;
      case 2: color = MAGENTA; break;
    }
    
    // Draw energy stream
    for (int j = 0; j < streams[i][2]; j++) {
      int x = streams[i][0] + j * 2;
      int y = streams[i][1] + sin((millis() * 0.01) + j * 0.5) * 5;
      
      if (x >= 0 && x < 240 && y >= 0 && y < 135) {
        uint8_t brightness = 255 - (j * 255 / streams[i][2]);
        uint16_t fadeColor = M5Cardputer.Display.color565(
          (color >> 11) * brightness / 255,
          ((color >> 5) & 0x3F) * brightness / 255,
          (color & 0x1F) * brightness / 255
        );
        M5Cardputer.Display.drawPixel(x, y, fadeColor);
      }
    }
    
    // Move stream
    streams[i][0] += 2;
    if (streams[i][0] > 240) {
      streams[i][0] = -streams[i][2] * 2;
      streams[i][1] = random(135);
      streams[i][2] = random(10, 30);
      streams[i][3] = random(0, 3);
    }
  }
}

void drawFadeplot() {
  static float plotTime = 0;
  static int plotHistory[240];
  plotTime += 0.1;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Generate new plot value
  float newValue = sin(plotTime) * cos(plotTime * 0.7) * sin(plotTime * 0.3);
  int plotY = 67 + newValue * 45;
  
  // Shift history and add new point
  for (int i = 0; i < 239; i++) {
    plotHistory[i] = plotHistory[i + 1];
  }
  plotHistory[239] = plotY;
  
  // Draw fading plot lines
  for (int x = 1; x < 240; x++) {
    if (plotHistory[x] != 0 && plotHistory[x-1] != 0) {
      // Fade color based on age
      int fade = (x * 255) / 240;
      uint16_t color = M5Cardputer.Display.color565(fade, fade/2, 255-fade);
      M5Cardputer.Display.drawLine(x-1, plotHistory[x-1], x, plotHistory[x], color);
    }
  }
  
  // Grid lines
  for (int y = 20; y < 135; y += 20) {
    M5Cardputer.Display.drawFastHLine(0, y, 240, M5Cardputer.Display.color565(30, 30, 30));
  }
}

void drawExec() {
  static float execTime = 0;
  execTime += 0.12;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Terminal window
  M5Cardputer.Display.drawRect(5, 5, 230, 125, M5Cardputer.Display.color565(100, 100, 100));
  M5Cardputer.Display.fillRect(6, 6, 228, 123, M5Cardputer.Display.color565(10, 10, 10));
  
  // Process bars
  for (int proc = 0; proc < 6; proc++) {
    int y = 15 + proc * 18;
    float activity = sin(execTime + proc * 0.5) * 0.5 + 0.5;
    int barWidth = activity * 200;
    
    // Process name
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(10, y);
    M5Cardputer.Display.printf("proc%d", proc);
    
    // Activity bar
    uint16_t barColor = M5Cardputer.Display.color565(0, 255 * activity, 100);
    M5Cardputer.Display.fillRect(50, y, barWidth, 12, barColor);
    
    // CPU percentage
    M5Cardputer.Display.setCursor(190, y);
    M5Cardputer.Display.printf("%.0f%%", activity * 100);
  }
  
  // Command prompt
  M5Cardputer.Display.setCursor(10, 120);
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.print("$ exec ");
  
  // Blinking cursor
  if ((int)(execTime * 2) % 2) {
    M5Cardputer.Display.print("_");
  }
}

void drawFireworkx() {
  static float fireworkTime = 0;
  fireworkTime += 0.1;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Multiple fireworks at different stages
  for (int firework = 0; firework < 3; firework++) {
    float fwPhase = fireworkTime + firework * 2.0;
    float fwAge = fmod(fwPhase, 5.0);
    
    // Firework center position
    int fwX = 60 + firework * 60;
    int fwY = 40 + sin(fwPhase * 0.3) * 15;
    
    if (fwAge < 1.0) {
      // Launch phase - rocket trail
      int trailHeight = fwAge * 80;
      for (int trail = 0; trail < trailHeight; trail += 2) {
        int trailY = 135 - trail;
        if (trailY > 0) {
          uint16_t trailColor = M5Cardputer.Display.color565(255, 255 - trail * 2, 0);
          M5Cardputer.Display.drawPixel(fwX + sin(trail * 0.1) * 2, trailY, trailColor);
        }
      }
    } else if (fwAge < 3.0) {
      // Explosion phase
      float explodeAge = fwAge - 1.0;
      int radius = explodeAge * 25;
      
      // Explosion particles
      for (int angle = 0; angle < 360; angle += 15) {
        float rad = angle * PI / 180.0;
        int px = fwX + cos(rad) * radius;
        int py = fwY + sin(rad) * radius + explodeAge * explodeAge * 10; // gravity
        
        if (px >= 0 && px < 240 && py >= 0 && py < 135) {
          uint16_t sparkColor = M5Cardputer.Display.color565(
            255 - explodeAge * 100,
            255 - explodeAge * 150,
            255 - explodeAge * 50
          );
          M5Cardputer.Display.fillCircle(px, py, 2, sparkColor);
        }
      }
    }
  }
}

void drawFps() {
  static float fpsTime = 0;
  static float fpsHistory[60];
  static int historyIndex = 0;
  static bool fpsInit = false;
  
  fpsTime += 0.016;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  if (!fpsInit) {
    for (int i = 0; i < 60; i++) {
      fpsHistory[i] = 60;
    }
    fpsInit = true;
  }
  
  // Simulate varying FPS
  float performanceVariation = sin(fpsTime * 0.5) * 20 + sin(fpsTime * 2) * 10;
  float currentFps = 60 + performanceVariation;
  
  // Update history
  fpsHistory[historyIndex] = currentFps;
  historyIndex = (historyIndex + 1) % 60;
  
  // FPS display
  M5Cardputer.Display.setTextSize(2);
  uint16_t fpsColor;
  if (currentFps >= 50) fpsColor = GREEN;
  else if (currentFps >= 30) fpsColor = YELLOW;
  else fpsColor = RED;
  
  M5Cardputer.Display.setTextColor(fpsColor);
  M5Cardputer.Display.setCursor(10, 10);
  M5Cardputer.Display.printf("FPS: %.1f", currentFps);
  
  // Performance graph
  int graphY = 50;
  int graphHeight = 60;
  
  // Draw graph background
  M5Cardputer.Display.drawRect(10, graphY, 220, graphHeight, WHITE);
  
  // Draw FPS history as line graph
  for (int i = 1; i < 60; i++) {
    int x1 = 10 + (i-1) * 220 / 60;
    int x2 = 10 + i * 220 / 60;
    int histIdx1 = (historyIndex + i - 1) % 60;
    int histIdx2 = (historyIndex + i) % 60;
    
    int y1 = graphY + graphHeight - (fpsHistory[histIdx1] / 100.0) * graphHeight;
    int y2 = graphY + graphHeight - (fpsHistory[histIdx2] / 100.0) * graphHeight;
    
    uint16_t lineColor;
    if (fpsHistory[histIdx1] >= 50) lineColor = GREEN;
    else if (fpsHistory[histIdx1] >= 30) lineColor = YELLOW;
    else lineColor = RED;
    
    M5Cardputer.Display.drawLine(x1, y1, x2, y2, lineColor);
  }
  
  // Statistics
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(10, 120);
  M5Cardputer.Display.printf("Performance Monitor");
}

void drawFontglide() {
  static float glideTime = 0;
  glideTime += 0.06;
  
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Text to display
  const char* glideText = "GLIDE";
  int textLen = strlen(glideText);
  
  // Character gliding
  for (int i = 0; i < textLen; i++) {
    char c = glideText[i];
    
    // Character position with wave motion
    float charPhase = glideTime + i * 0.8;
    int baseX = 30 + i * 35;
    int baseY = 67;
    
    // Gliding motion - sine wave path
    int glideX = baseX + sin(charPhase) * 20;
    int glideY = baseY + cos(charPhase * 1.3) * 25;
    
    // Size variation
    float sizeVar = 1.0 + sin(charPhase * 2) * 0.5;
    int textSize = sizeVar * 2;
    
    // Color cycling
    uint16_t charColor = M5Cardputer.Display.color565(
      128 + sin(charPhase) * 127,
      128 + sin(charPhase + PI/3) * 127,
      128 + sin(charPhase + 2*PI/3) * 127
    );
    
    // Draw character with effects
    M5Cardputer.Display.setTextColor(charColor);
    M5Cardputer.Display.setTextSize(textSize);
    M5Cardputer.Display.setCursor(glideX, glideY);
    M5Cardputer.Display.print(c);
    
    // Trail effect
    for (int trail = 1; trail < 4; trail++) {
      int trailX = glideX - sin(charPhase) * trail * 8;
      int trailY = glideY - cos(charPhase * 1.3) * trail * 8;
      uint16_t trailColor = M5Cardputer.Display.color565(
        (128 + sin(charPhase) * 127) / (trail + 1),
        (128 + sin(charPhase + PI/3) * 127) / (trail + 1),
        (128 + sin(charPhase + 2*PI/3) * 127) / (trail + 1)
      );
      M5Cardputer.Display.setTextColor(trailColor);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.setCursor(trailX, trailY);
      M5Cardputer.Display.print(c);
    }
  }
}

// ===== TETRIS GAME IMPLEMENTATION =====

void initTetris() {
  tetrisGame.init();
  Serial.println("Tetris initialized");
}

void handleTetris() {
  if (checkEscapeKey()) {
    enterScreensaver();
    return;
  }
  
  tetrisGame.update();
  tetrisGame.draw();
  
  if (tetrisGame.isGameOver()) {
    // Show game over screen - centered for landscape
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(70, 40);
    M5Cardputer.Display.print("GAME OVER");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(85, 65);
    M5Cardputer.Display.printf("Score: %d", tetrisGame.getScore());
    M5Cardputer.Display.setCursor(60, 85);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.print("ENTER=Restart ESC=Exit");
    
    // Handle restart
    if (M5Cardputer.Keyboard.isPressed()) {
      if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || 
          M5Cardputer.Keyboard.isKeyPressed(' ')) {
        tetrisGame.reset();
        delay(200);
      }
    }
  }
}

// TetrisGame class implementation
void TetrisGame::init() {
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Initialize field
  for (int y = 0; y < TETRIS_FIELD_HEIGHT; y++) {
    for (int x = 0; x < TETRIS_FIELD_WIDTH; x++) {
      field[y][x] = 0;
    }
  }
  
  // CORRECTED piece definitions from PERFECT tetris
  int tempPieces[7][4][2][4] = {
    // O piece - square (doesn't rotate)
    {{{0,1,0,1},{0,0,1,1}},{{0,1,0,1},{0,0,1,1}},{{0,1,0,1},{0,0,1,1}},{{0,1,0,1},{0,0,1,1}}},
    // I piece - line
    {{{0,0,0,0},{-1,0,1,2}},{{-1,0,1,2},{0,0,0,0}},{{0,0,0,0},{-1,0,1,2}},{{-1,0,1,2},{0,0,0,0}}},
    // T piece - CORRECTED rotation
    {{{0,0,0,1},{-1,0,1,0}},{{1,0,-1,0},{0,0,0,-1}},{{0,0,0,-1},{-1,0,1,0}},{{1,0,-1,0},{0,0,0,1}}},
    // S piece
    {{{0,-1,0,1},{0,0,1,1}},{{0,1,1,0},{0,0,-1,1}},{{0,-1,0,1},{0,0,1,1}},{{0,1,1,0},{0,0,-1,1}}},
    // Z piece
    {{{0,-1,0,1},{0,0,-1,-1}},{{0,0,1,1},{0,-1,0,1}},{{0,-1,0,1},{0,0,-1,-1}},{{0,0,1,1},{0,-1,0,1}}},
    // J piece
    {{{1,0,-1,1},{0,0,0,-1}},{{0,-1,0,0},{0,0,1,2}},{{0,1,2,0},{0,0,0,1}},{{1,0,0,0},{1,1,0,-1}}},
    // L piece - CORRECTED rotation
    {{{0,0,1,2},{-1,0,0,0}},{{-1,0,0,0},{1,1,0,-1}},{{1,1,0,-1},{1,0,0,0}},{{1,0,0,0},{-1,-1,0,1}}}
  };
  memcpy(pieces, tempPieces, sizeof(pieces));
  
  // Colors for pieces
  pieceColors[0] = YELLOW;
  pieceColors[1] = CYAN;
  pieceColors[2] = 0xF81F; // Purple
  pieceColors[3] = GREEN;
  pieceColors[4] = RED;
  pieceColors[5] = BLUE;
  pieceColors[6] = 0xFD20; // Orange
  
  score = 0;
  level = 1;
  linesCleared = 0;
  dropSpeed = 500;
  gameOver = false;
  lastDropTime = 0;
  lockDelayActive = false;
  
  // Modern features
  heldPiece = -1;
  nextPiece = random(0, 7);
  canHold = true;
  
  newPiece(false);
}

void TetrisGame::update() {
  handleTetrisInput();
  
  if (millis() - lastDropTime > dropSpeed) {
    posY++;
    if (test(posY, posX, currentPiece, currentRot)) {
      posY--;
      // Start lock delay when piece hits bottom
      if (!lockDelayActive) {
        lockDelayActive = true;
        lockDelayStart = millis();
      }
      
      // Check if lock delay has expired (500ms)
      if (millis() - lockDelayStart >= 500) {
        placePiece();
        clearLines();
        newPiece(true);
        if (test(posY, posX, currentPiece, currentRot)) {
          gameOver = true;
        }
      }
    } else {
      lockDelayActive = false;
    }
    lastDropTime = millis();
  }
}

void TetrisGame::draw() {
  static int lastScore = -1;
  static bool firstDraw = true;
  static bool forceRedraw = false;
  
  // Only clear screen on first draw or when forced
  if (firstDraw || forceRedraw) {
    M5Cardputer.Display.fillScreen(BLACK);
    // Draw border around field (only once)
    M5Cardputer.Display.drawRect(TETRIS_OFFSET_X-1, TETRIS_OFFSET_Y-1, 
                                 TETRIS_FIELD_WIDTH*TETRIS_BLOCK_SIZE+1, 
                                 TETRIS_FIELD_HEIGHT*TETRIS_BLOCK_SIZE+1, WHITE);
    firstDraw = false;
    forceRedraw = false;
  }
  
  // Check if we need to force a redraw (game just restarted)
  if (score == 0 && lastScore != 0) {
    forceRedraw = true;
  }
  
  // Draw field blocks (only draw empty spaces as black to reduce flicker)
  for (int y = 0; y < TETRIS_FIELD_HEIGHT; y++) {
    for (int x = 0; x < TETRIS_FIELD_WIDTH; x++) {
      int px = TETRIS_OFFSET_X + x * TETRIS_BLOCK_SIZE;
      int py = TETRIS_OFFSET_Y + y * TETRIS_BLOCK_SIZE;
      if (field[y][x] > 0) {
        M5Cardputer.Display.fillRect(px, py, TETRIS_BLOCK_SIZE-1, TETRIS_BLOCK_SIZE-1, 
                                    pieceColors[field[y][x]-1]);
        M5Cardputer.Display.drawRect(px, py, TETRIS_BLOCK_SIZE-1, TETRIS_BLOCK_SIZE-1, WHITE);
      } else {
        M5Cardputer.Display.fillRect(px, py, TETRIS_BLOCK_SIZE-1, TETRIS_BLOCK_SIZE-1, BLACK);
      }
    }
  }
  
  // Draw ghost piece
  drawGhostPiece();
  
  // Draw current piece
  for (int i = 0; i < 4; i++) {
    int x = posX + pieces[currentPiece][currentRot][1][i];
    int y = posY + pieces[currentPiece][currentRot][0][i];
    if (y >= 0 && y < TETRIS_FIELD_HEIGHT && x >= 0 && x < TETRIS_FIELD_WIDTH) {
      int px = TETRIS_OFFSET_X + x * TETRIS_BLOCK_SIZE;
      int py = TETRIS_OFFSET_Y + y * TETRIS_BLOCK_SIZE;
      M5Cardputer.Display.fillRect(px, py, TETRIS_BLOCK_SIZE-1, TETRIS_BLOCK_SIZE-1, 
                                  pieceColors[currentPiece]);
      M5Cardputer.Display.drawRect(px, py, TETRIS_BLOCK_SIZE-1, TETRIS_BLOCK_SIZE-1, WHITE);
    }
  }
  
  // Draw hold and next pieces
  drawHoldPiece();
  drawNextPiece();
  
  // Update lastScore tracking for restart detection
  lastScore = score;
}

void TetrisGame::handleTetrisInput() {
  static unsigned long lastMove = 0;
  static unsigned long lastLeftRight = 0;
  static bool buttonHeld = false;
  
  if (!M5Cardputer.Keyboard.isPressed()) {
    buttonHeld = false;
    return;
  }
  
  // Left movement - W key (slowed down to 120ms like portrait version) 
  if (M5Cardputer.Keyboard.isKeyPressed('w') && millis() - lastLeftRight > 120) {
    posX--;
    if (test(posY, posX, currentPiece, currentRot)) posX++;
    else if (lockDelayActive) lockDelayStart = millis(); // Reset lock delay
    lastLeftRight = millis();
  }
  // Right movement - E key (slowed down to 120ms)
  else if (M5Cardputer.Keyboard.isKeyPressed('e') && millis() - lastLeftRight > 120) {
    posX++;
    if (test(posY, posX, currentPiece, currentRot)) posX--;
    else if (lockDelayActive) lockDelayStart = millis();
    lastLeftRight = millis();
  }
  
  if (millis() - lastMove < 100) return;
  
  // Hard drop - 2 or 3 keys
  if ((M5Cardputer.Keyboard.isKeyPressed('2') || 
       M5Cardputer.Keyboard.isKeyPressed('3')) && !buttonHeld) {
    while(!test(posY + 1, posX, currentPiece, currentRot)) {
      posY++;
      score += 2; // Award more points for hard drop
    }
    lockDelayActive = false;
    lastDropTime = 0; // Force immediate lock
    lastMove = millis();
    buttonHeld = true;
  }
  // Soft drop - A or S keys
  else if (M5Cardputer.Keyboard.isKeyPressed('a') || 
           M5Cardputer.Keyboard.isKeyPressed('s')) {
    posY++;
    if (test(posY, posX, currentPiece, currentRot)) {
      posY--;
    } else {
      score++; // Award points for soft drop
      lockDelayActive = false;
    }
    lastMove = millis();
    buttonHeld = true;
  }
  // Rotate clockwise - O key
  else if (M5Cardputer.Keyboard.isKeyPressed('o') && !buttonHeld) {
    int newRot = (currentRot + 1) % 4;
    if (!test(posY, posX, currentPiece, newRot)) {
      currentRot = newRot;
      if (lockDelayActive) lockDelayStart = millis();
    }
    lastMove = millis();
    buttonHeld = true;
  }
  // Rotate counter-clockwise - I key
  else if (M5Cardputer.Keyboard.isKeyPressed('i') && !buttonHeld) {
    int newRot = (currentRot + 3) % 4; // +3 = -1 in mod 4
    if (!test(posY, posX, currentPiece, newRot)) {
      currentRot = newRot;
      if (lockDelayActive) lockDelayStart = millis();
    }
    lastMove = millis();
    buttonHeld = true;
  }
  // Hold piece - 9 or 8 keys
  else if ((M5Cardputer.Keyboard.isKeyPressed('9') || 
            M5Cardputer.Keyboard.isKeyPressed('8')) && !buttonHeld) {
    holdPiece();
    lastMove = millis();
    buttonHeld = true;
  }
}

bool TetrisGame::test(int y, int x, int piece, int rot) {
  for (int i = 0; i < 4; i++) {
    int px = x + pieces[piece][rot][1][i];
    int py = y + pieces[piece][rot][0][i];
    
    if (px < 0 || px >= TETRIS_FIELD_WIDTH || py >= TETRIS_FIELD_HEIGHT) return true;
    if (py >= 0 && field[py][px] > 0) return true;
  }
  return false;
}

void TetrisGame::placePiece() {
  for (int i = 0; i < 4; i++) {
    int x = posX + pieces[currentPiece][currentRot][1][i];
    int y = posY + pieces[currentPiece][currentRot][0][i];
    if (y >= 0 && y < TETRIS_FIELD_HEIGHT && x >= 0 && x < TETRIS_FIELD_WIDTH) {
      field[y][x] = currentPiece + 1;
    }
  }
}

void TetrisGame::clearLines() {
  int linesThisClear = 0;
  
  for (int y = TETRIS_FIELD_HEIGHT - 1; y >= 0; y--) {
    bool fullLine = true;
    for (int x = 0; x < TETRIS_FIELD_WIDTH; x++) {
      if (field[y][x] == 0) {
        fullLine = false;
        break;
      }
    }
    
    if (fullLine) {
      linesThisClear++;
      score += 100;
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < TETRIS_FIELD_WIDTH; x++) {
          field[yy][x] = field[yy-1][x];
        }
      }
      for (int x = 0; x < TETRIS_FIELD_WIDTH; x++) {
        field[0][x] = 0;
      }
      y++;
    }
  }
  
  // Update lines cleared and check for level up
  if (linesThisClear > 0) {
    linesCleared += linesThisClear;
    
    // Level up every 10 lines
    int newLevel = 1 + (linesCleared / 10);
    if (newLevel > level) {
      level = newLevel;
      // Speed up (reduce dropSpeed by 10% per level)
      dropSpeed = max(100, 500 - (level - 1) * 40);
    }
  }
}

void TetrisGame::newPiece(bool setPiece) {
  if (setPiece) {
    canHold = true; // Reset hold ability
  }
  
  // Use next piece, generate new next
  if (nextPiece >= 0) {
    currentPiece = nextPiece;
    nextPiece = random(0, 7);
  } else {
    currentPiece = random(0, 7);
    nextPiece = random(0, 7);
  }
  
  currentRot = 0;
  posX = TETRIS_FIELD_WIDTH / 2 - 1;
  posY = 0;
  
  lockDelayActive = false;
}

void TetrisGame::holdPiece() {
  if (!canHold) return;
  
  if (heldPiece == -1) {
    heldPiece = currentPiece;
    newPiece(false);
  } else {
    int temp = currentPiece;
    currentPiece = heldPiece;
    heldPiece = temp;
    currentRot = 0;
    posX = TETRIS_FIELD_WIDTH / 2 - 1;
    posY = 0;
  }
  
  canHold = false;
  lockDelayActive = false;
}

int TetrisGame::calculateDropDistance() {
  int dropDist = 0;
  for (int testY = posY + 1; testY < TETRIS_FIELD_HEIGHT; testY++) {
    if (!test(testY, posX, currentPiece, currentRot)) {
      dropDist = testY - posY;
    } else {
      break;
    }
  }
  return dropDist;
}

void TetrisGame::drawGhostPiece() {
  int dropDist = calculateDropDistance();
  if (dropDist > 0) {
    int ghostY = posY + dropDist;
    for (int i = 0; i < 4; i++) {
      int x = posX + pieces[currentPiece][currentRot][1][i];
      int y = ghostY + pieces[currentPiece][currentRot][0][i];
      if (y >= 0 && y < TETRIS_FIELD_HEIGHT && x >= 0 && x < TETRIS_FIELD_WIDTH) {
        int px = TETRIS_OFFSET_X + x * TETRIS_BLOCK_SIZE;
        int py = TETRIS_OFFSET_Y + y * TETRIS_BLOCK_SIZE;
        M5Cardputer.Display.drawRect(px, py, TETRIS_BLOCK_SIZE-1, TETRIS_BLOCK_SIZE-1, 0x4208); // Gray
      }
    }
  }
}

void TetrisGame::drawMiniPiece(int pieceType, int x, int y, int scale) {
  if (pieceType < 0 || pieceType > 6) return;
  for (int i = 0; i < 4; i++) {
    int px = x + (pieces[pieceType][0][1][i] * scale);
    int py = y + (pieces[pieceType][0][0][i] * scale);
    M5Cardputer.Display.fillRect(px, py, scale-1, scale-1, pieceColors[pieceType]);
  }
}

void TetrisGame::drawHoldPiece() {
  // Position to the right of the game field (field is 80px wide starting at x=5)
  M5Cardputer.Display.fillRect(95, 10, 35, 35, BLACK);
  M5Cardputer.Display.drawRect(94, 9, 37, 37, WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(105, 47);
  M5Cardputer.Display.print("HOLD");
  
  if (heldPiece >= 0) {
    drawMiniPiece(heldPiece, 105, 18, 4);
  }
}

void TetrisGame::drawNextPiece() {
  // Position below the hold piece
  M5Cardputer.Display.fillRect(95, 65, 35, 35, BLACK);
  M5Cardputer.Display.drawRect(94, 64, 37, 37, WHITE);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(105, 102);
  M5Cardputer.Display.print("NEXT");
  
  if (nextPiece >= 0) {
    drawMiniPiece(nextPiece, 105, 73, 4);
  }
}

// ===== PORTRAIT TETRIS GAME IMPLEMENTATION =====

void initTetrisPortrait() {
  tetrisPortraitGame.init();
  Serial.println("Portrait Tetris initialized");
}

void handleTetrisPortrait() {
  if (checkEscapeKey()) {
    M5Cardputer.Display.setRotation(1); // Reset to landscape mode
    enterScreensaver();
    return;
  }
  
  tetrisPortraitGame.update();
  tetrisPortraitGame.draw();
  
  if (tetrisPortraitGame.isGameOver()) {
    // Show game over screen - centered for portrait  
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(15, 100);
    M5Cardputer.Display.print("GAME OVER");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(25, 130);
    M5Cardputer.Display.printf("Score: %d", tetrisPortraitGame.getScore());
    M5Cardputer.Display.setCursor(10, 150);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.print("ENTER=Restart ESC=Exit");
    
    // Handle restart
    if (M5Cardputer.Keyboard.isPressed()) {
      if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || 
          M5Cardputer.Keyboard.isKeyPressed(' ')) {
        tetrisPortraitGame.reset();
        delay(200);
      }
    }
  }
}

// TetrisPortraitGame class implementation
void TetrisPortraitGame::init() {
  M5Cardputer.Display.setRotation(0); // Portrait mode (rotate counter-clockwise from default)
  M5Cardputer.Display.fillScreen(BLACK);
  
  // Initialize field
  for (int y = 0; y < TETRIS_P_FIELD_HEIGHT; y++) {
    for (int x = 0; x < TETRIS_P_FIELD_WIDTH; x++) {
      field[y][x] = 0;
    }
  }
  
  // CORRECTED piece definitions from PERFECT tetris
  int tempPieces[7][4][2][4] = {
    // O piece - square (doesn't rotate)
    {{{0,1,0,1},{0,0,1,1}},{{0,1,0,1},{0,0,1,1}},{{0,1,0,1},{0,0,1,1}},{{0,1,0,1},{0,0,1,1}}},
    // I piece - line
    {{{0,0,0,0},{-1,0,1,2}},{{-1,0,1,2},{0,0,0,0}},{{0,0,0,0},{-1,0,1,2}},{{-1,0,1,2},{0,0,0,0}}},
    // T piece - CORRECTED rotation
    {{{0,0,0,1},{-1,0,1,0}},{{1,0,-1,0},{0,0,0,-1}},{{0,0,0,-1},{-1,0,1,0}},{{1,0,-1,0},{0,0,0,1}}},
    // S piece
    {{{0,-1,0,1},{0,0,1,1}},{{0,1,1,0},{0,0,-1,1}},{{0,-1,0,1},{0,0,1,1}},{{0,1,1,0},{0,0,-1,1}}},
    // Z piece
    {{{0,-1,0,1},{0,0,-1,-1}},{{0,0,1,1},{0,-1,0,1}},{{0,-1,0,1},{0,0,-1,-1}},{{0,0,1,1},{0,-1,0,1}}},
    // J piece
    {{{1,0,-1,1},{0,0,0,-1}},{{0,-1,0,0},{0,0,1,2}},{{0,1,2,0},{0,0,0,1}},{{1,0,0,0},{1,1,0,-1}}},
    // L piece - CORRECTED rotation
    {{{0,0,1,2},{-1,0,0,0}},{{-1,0,0,0},{1,1,0,-1}},{{1,1,0,-1},{1,0,0,0}},{{1,0,0,0},{-1,-1,0,1}}}
  };
  memcpy(pieces, tempPieces, sizeof(pieces));
  
  // Colors for pieces
  pieceColors[0] = YELLOW;
  pieceColors[1] = CYAN;
  pieceColors[2] = 0xF81F; // Purple
  pieceColors[3] = GREEN;
  pieceColors[4] = RED;
  pieceColors[5] = BLUE;
  pieceColors[6] = 0xFD20; // Orange
  
  score = 0;
  level = 1;
  linesCleared = 0;
  dropSpeed = 500;
  gameOver = false;
  lastDropTime = 0;
  lockDelayActive = false;
  
  // Modern features
  heldPiece = -1;
  nextPiece = random(0, 7);
  canHold = true;
  
  newPiece(false);
}

void TetrisPortraitGame::update() {
  handleTetrisPortraitInput();
  
  if (millis() - lastDropTime > dropSpeed) {
    posY++;
    if (test(posY, posX, currentPiece, currentRot)) {
      posY--;
      // Start lock delay when piece hits bottom
      if (!lockDelayActive) {
        lockDelayActive = true;
        lockDelayStart = millis();
      }
      
      // Check if lock delay has expired (500ms)
      if (millis() - lockDelayStart >= 500) {
        placePiece();
        clearLines();
        newPiece(true);
        if (test(posY, posX, currentPiece, currentRot)) {
          gameOver = true;
        }
      }
    } else {
      lockDelayActive = false;
    }
    lastDropTime = millis();
  }
}

void TetrisPortraitGame::draw() {
  static bool firstDraw = true;
  static bool forceRedraw = false;
  
  // Only clear screen on first draw or when forced
  if (firstDraw || forceRedraw) {
    M5Cardputer.Display.fillScreen(BLACK);
    // Draw clean border around game field
    M5Cardputer.Display.drawRect(TETRIS_P_OFFSET_X-1, TETRIS_P_OFFSET_Y-1, 
                                 TETRIS_P_FIELD_WIDTH*TETRIS_P_BLOCK_SIZE+1, 
                                 TETRIS_P_FIELD_HEIGHT*TETRIS_P_BLOCK_SIZE+1, WHITE);
    firstDraw = false;
    forceRedraw = false;
  }
  
  // Check if we need to force a redraw (game just restarted)
  static int lastScore = -1;
  if (score == 0 && lastScore != 0) {
    forceRedraw = true;
  }
  
  // Draw field blocks
  for (int y = 0; y < TETRIS_P_FIELD_HEIGHT; y++) {
    for (int x = 0; x < TETRIS_P_FIELD_WIDTH; x++) {
      int px = TETRIS_P_OFFSET_X + x * TETRIS_P_BLOCK_SIZE;
      int py = TETRIS_P_OFFSET_Y + y * TETRIS_P_BLOCK_SIZE;
      if (field[y][x] > 0) {
        M5Cardputer.Display.fillRect(px, py, TETRIS_P_BLOCK_SIZE-1, TETRIS_P_BLOCK_SIZE-1, 
                                    pieceColors[field[y][x]-1]);
        M5Cardputer.Display.drawRect(px, py, TETRIS_P_BLOCK_SIZE-1, TETRIS_P_BLOCK_SIZE-1, WHITE);
      } else {
        M5Cardputer.Display.fillRect(px, py, TETRIS_P_BLOCK_SIZE-1, TETRIS_P_BLOCK_SIZE-1, BLACK);
      }
    }
  }
  
  // Draw ghost piece
  drawGhostPiece();
  
  // Draw current piece
  for (int i = 0; i < 4; i++) {
    int x = posX + pieces[currentPiece][currentRot][1][i];
    int y = posY + pieces[currentPiece][currentRot][0][i];
    if (y >= 0 && y < TETRIS_P_FIELD_HEIGHT && x >= 0 && x < TETRIS_P_FIELD_WIDTH) {
      int px = TETRIS_P_OFFSET_X + x * TETRIS_P_BLOCK_SIZE;
      int py = TETRIS_P_OFFSET_Y + y * TETRIS_P_BLOCK_SIZE;
      M5Cardputer.Display.fillRect(px, py, TETRIS_P_BLOCK_SIZE-1, TETRIS_P_BLOCK_SIZE-1, 
                                  pieceColors[currentPiece]);
      M5Cardputer.Display.drawRect(px, py, TETRIS_P_BLOCK_SIZE-1, TETRIS_P_BLOCK_SIZE-1, WHITE);
    }
  }
  
  // Draw hold and next pieces - clean layout
  drawHoldPiece();
  drawNextPiece();
  
  // Update lastScore tracking for restart detection
  lastScore = score;
}

void TetrisPortraitGame::handleTetrisPortraitInput() {
  static unsigned long lastMove = 0;
  static unsigned long lastLeftRight = 0;
  static bool buttonHeld = false;
  
  if (!M5Cardputer.Keyboard.isPressed()) {
    buttonHeld = false;
    return;
  }
  
  // Left movement - B key (slowed down to 120ms like Quindectris)
  if (M5Cardputer.Keyboard.isKeyPressed('b') && millis() - lastLeftRight > 120) {
    posX--;
    if (test(posY, posX, currentPiece, currentRot)) posX++;
    else if (lockDelayActive) lockDelayStart = millis(); // Reset lock delay
    lastLeftRight = millis();
  }
  // Right movement - H key (slowed down to 120ms)
  else if (M5Cardputer.Keyboard.isKeyPressed('h') && millis() - lastLeftRight > 120) {
    posX++;
    if (test(posY, posX, currentPiece, currentRot)) posX--;
    else if (lockDelayActive) lockDelayStart = millis();
    lastLeftRight = millis();
  }
  
  if (millis() - lastMove < 100) return;
  
  // Hard drop - G or V keys
  if ((M5Cardputer.Keyboard.isKeyPressed('g') || 
       M5Cardputer.Keyboard.isKeyPressed('v')) && !buttonHeld) {
    while(!test(posY + 1, posX, currentPiece, currentRot)) {
      posY++;
      score += 2; // Award more points for hard drop
    }
    lockDelayActive = false;
    lastDropTime = 0; // Force immediate lock
    lastMove = millis();
    buttonHeld = true;
  }
  // Soft drop - J or N keys
  else if (M5Cardputer.Keyboard.isKeyPressed('j') || 
           M5Cardputer.Keyboard.isKeyPressed('n')) {
    posY++;
    if (test(posY, posX, currentPiece, currentRot)) {
      posY--;
    } else {
      score++; // Award points for soft drop
      lockDelayActive = false;
    }
    lastMove = millis();
    buttonHeld = true;
  }
  // Rotate clockwise - 9 key
  else if (M5Cardputer.Keyboard.isKeyPressed('9') && !buttonHeld) {
    int newRot = (currentRot + 1) % 4;
    if (!test(posY, posX, currentPiece, newRot)) {
      currentRot = newRot;
      if (lockDelayActive) lockDelayStart = millis();
    }
    lastMove = millis();
    buttonHeld = true;
  }
  // Rotate counter-clockwise - O key
  else if (M5Cardputer.Keyboard.isKeyPressed('o') && !buttonHeld) {
    int newRot = (currentRot + 3) % 4; // +3 = -1 in mod 4
    if (!test(posY, posX, currentPiece, newRot)) {
      currentRot = newRot;
      if (lockDelayActive) lockDelayStart = millis();
    }
    lastMove = millis();
    buttonHeld = true;
  }
  // Hold piece - 8 or I keys
  else if ((M5Cardputer.Keyboard.isKeyPressed('8') || 
            M5Cardputer.Keyboard.isKeyPressed('i')) && !buttonHeld) {
    holdPiece();
    lastMove = millis();
    buttonHeld = true;
  }
}

bool TetrisPortraitGame::test(int y, int x, int piece, int rot) {
  for (int i = 0; i < 4; i++) {
    int px = x + pieces[piece][rot][1][i];
    int py = y + pieces[piece][rot][0][i];
    
    if (px < 0 || px >= TETRIS_P_FIELD_WIDTH || py >= TETRIS_P_FIELD_HEIGHT) return true;
    if (py >= 0 && field[py][px] > 0) return true;
  }
  return false;
}

void TetrisPortraitGame::placePiece() {
  for (int i = 0; i < 4; i++) {
    int x = posX + pieces[currentPiece][currentRot][1][i];
    int y = posY + pieces[currentPiece][currentRot][0][i];
    if (y >= 0 && y < TETRIS_P_FIELD_HEIGHT && x >= 0 && x < TETRIS_P_FIELD_WIDTH) {
      field[y][x] = currentPiece + 1;
    }
  }
}

void TetrisPortraitGame::clearLines() {
  int linesThisClear = 0;
  
  for (int y = TETRIS_P_FIELD_HEIGHT - 1; y >= 0; y--) {
    bool fullLine = true;
    for (int x = 0; x < TETRIS_P_FIELD_WIDTH; x++) {
      if (field[y][x] == 0) {
        fullLine = false;
        break;
      }
    }
    
    if (fullLine) {
      linesThisClear++;
      score += 100;
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < TETRIS_P_FIELD_WIDTH; x++) {
          field[yy][x] = field[yy-1][x];
        }
      }
      for (int x = 0; x < TETRIS_P_FIELD_WIDTH; x++) {
        field[0][x] = 0;
      }
      y++;
    }
  }
  
  // Update lines cleared and check for level up
  if (linesThisClear > 0) {
    linesCleared += linesThisClear;
    
    // Level up every 10 lines
    int newLevel = 1 + (linesCleared / 10);
    if (newLevel > level) {
      level = newLevel;
      // Speed up (reduce dropSpeed by 10% per level)
      dropSpeed = max(100, 500 - (level - 1) * 40);
    }
  }
}

void TetrisPortraitGame::newPiece(bool setPiece) {
  if (setPiece) {
    canHold = true; // Reset hold ability
  }
  
  // Use next piece, generate new next
  if (nextPiece >= 0) {
    currentPiece = nextPiece;
    nextPiece = random(0, 7);
  } else {
    currentPiece = random(0, 7);
    nextPiece = random(0, 7);
  }
  
  currentRot = 0;
  posX = TETRIS_P_FIELD_WIDTH / 2 - 1;
  posY = 0;
  
  lockDelayActive = false;
}

void TetrisPortraitGame::holdPiece() {
  if (!canHold) return;
  
  if (heldPiece == -1) {
    heldPiece = currentPiece;
    newPiece(false);
  } else {
    int temp = currentPiece;
    currentPiece = heldPiece;
    heldPiece = temp;
    currentRot = 0;
    posX = TETRIS_P_FIELD_WIDTH / 2 - 1;
    posY = 0;
  }
  
  canHold = false;
  lockDelayActive = false;
}

int TetrisPortraitGame::calculateDropDistance() {
  int dropDist = 0;
  for (int testY = posY + 1; testY < TETRIS_P_FIELD_HEIGHT; testY++) {
    if (!test(testY, posX, currentPiece, currentRot)) {
      dropDist = testY - posY;
    } else {
      break;
    }
  }
  return dropDist;
}

void TetrisPortraitGame::drawGhostPiece() {
  int dropDist = calculateDropDistance();
  if (dropDist > 0) {
    int ghostY = posY + dropDist;
    for (int i = 0; i < 4; i++) {
      int x = posX + pieces[currentPiece][currentRot][1][i];
      int y = ghostY + pieces[currentPiece][currentRot][0][i];
      if (y >= 0 && y < TETRIS_P_FIELD_HEIGHT && x >= 0 && x < TETRIS_P_FIELD_WIDTH) {
        int px = TETRIS_P_OFFSET_X + x * TETRIS_P_BLOCK_SIZE;
        int py = TETRIS_P_OFFSET_Y + y * TETRIS_P_BLOCK_SIZE;
        M5Cardputer.Display.drawRect(px, py, TETRIS_P_BLOCK_SIZE-1, TETRIS_P_BLOCK_SIZE-1, 0x4208); // Gray
      }
    }
  }
}

void TetrisPortraitGame::drawMiniPiece(int pieceType, int x, int y, int scale) {
  if (pieceType < 0 || pieceType > 6) return;
  for (int i = 0; i < 4; i++) {
    int px = x + (pieces[pieceType][0][1][i] * scale);
    int py = y + (pieces[pieceType][0][0][i] * scale);
    M5Cardputer.Display.fillRect(px, py, scale-1, scale-1, pieceColors[pieceType]);
  }
}

void TetrisPortraitGame::drawHoldPiece() {
  // Position to the right of the game field (field is 80px wide starting at x=5)
  M5Cardputer.Display.fillRect(90, 10, 35, 35, BLACK);
  M5Cardputer.Display.drawRect(89, 9, 37, 37, WHITE);
  
  if (heldPiece >= 0) {
    drawMiniPiece(heldPiece, 100, 18, 4);
  }
  
  // Label under the box
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(100, 47);
  M5Cardputer.Display.print("HOLD");
}

void TetrisPortraitGame::drawNextPiece() {
  // Below Hold box - Next box  
  M5Cardputer.Display.fillRect(90, 65, 35, 35, BLACK);
  M5Cardputer.Display.drawRect(89, 64, 37, 37, WHITE);
  
  if (nextPiece >= 0) {
    drawMiniPiece(nextPiece, 100, 73, 4);
  }
  
  // Label under the box
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(100, 102);
  M5Cardputer.Display.print("NEXT");
}

// ============================================================================
// TETROIDS ASTEROIDS GAME IMPLEMENTATION
// ============================================================================

// Game constants adapted for M5Cardputer landscape (240x135)
#define TETROIDS_SCREEN_WIDTH 240
#define TETROIDS_SCREEN_HEIGHT 135
#define TETROIDS_PLAY_AREA_TOP 12
#define TETROIDS_PLAY_AREA_BOTTOM 125
#define TETROIDS_BLOCK_SIZE 6

// Game limits
#define TETROIDS_MAX_BULLETS 4
#define TETROIDS_MAX_ASTEROIDS 12
#define TETROIDS_MAX_BLOCKS 25

// Colors (Tetris themed)
#define TETROIDS_COLOR_PLAYER 0xF81F    // Magenta (T-piece)
#define TETROIDS_COLOR_BULLET 0xFFFF    // White
#define TETROIDS_COLOR_I 0x07FF         // Cyan
#define TETROIDS_COLOR_O 0xFFE0         // Yellow
#define TETROIDS_COLOR_L 0xFD20         // Orange
#define TETROIDS_COLOR_J 0x001F         // Blue
#define TETROIDS_COLOR_S 0x07E0         // Green
#define TETROIDS_COLOR_Z 0xF800         // Red

// Physics constants
#define TETROIDS_THRUST_POWER 0.3f
#define TETROIDS_FRICTION 0.98f
#define TETROIDS_MAX_SPEED 3.0f
#define TETROIDS_BULLET_SPEED 5.0f
#define TETROIDS_ASTEROID_SPEED 1.0f

// Tetris piece shapes [type][rotation][block][x/y]
// 0=I, 1=O, 2=L, 3=J, 4=S, 5=Z (NOT T - that's the player)
const int8_t TETROIDS_ASTEROID_SHAPES[6][4][4][2] = {
  // I-piece (4 rotations)
  {{{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}, {{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}},
  // O-piece (all same)
  {{{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}}},
  // L-piece
  {{{-1,0},{0,0},{1,0},{1,-1}}, {{0,-1},{0,0},{0,1},{1,1}}, {{-1,1},{-1,0},{0,0},{1,0}}, {{-1,-1},{0,-1},{0,0},{0,1}}},
  // J-piece
  {{{-1,-1},{-1,0},{0,0},{1,0}}, {{0,-1},{1,-1},{0,0},{0,1}}, {{-1,0},{0,0},{1,0},{1,1}}, {{0,-1},{0,0},{-1,1},{0,1}}},
  // S-piece
  {{{-1,0},{0,0},{0,-1},{1,-1}}, {{0,-1},{0,0},{1,0},{1,1}}, {{-1,0},{0,0},{0,-1},{1,-1}}, {{0,-1},{0,0},{1,0},{1,1}}},
  // Z-piece
  {{{-1,-1},{0,-1},{0,0},{1,0}}, {{1,-1},{1,0},{0,0},{0,1}}, {{-1,-1},{0,-1},{0,0},{1,0}}, {{1,-1},{1,0},{0,0},{0,1}}}
};

const uint16_t TETROIDS_ASTEROID_COLORS[6] = {TETROIDS_COLOR_I, TETROIDS_COLOR_O, TETROIDS_COLOR_L, TETROIDS_COLOR_J, TETROIDS_COLOR_S, TETROIDS_COLOR_Z};

// Player ship (T-piece) base shape [block][x/y]
const int8_t TETROIDS_PLAYER_SHAPE[4][2] = {{0,-1},{-1,0},{0,0},{1,0}};

// Game objects
struct TetroidsBullet {
  float x, y;
  float vx, vy;
  bool active;
};

struct TetroidsAsteroid {
  float x, y;
  float vx, vy;
  int type;        // 0-5 (I,O,L,J,S,Z)
  int rotation;    // 0-3
  float rotSpeed;  // Rotation speed
  bool active;
  bool isLarge;    // true = full piece, false = single block
};

struct TetroidsPlayer {
  float x, y;
  float vx, vy;
  float angle;     // 0-360 degrees (smooth rotation)
  int lives;
  bool alive;
  unsigned long invincibleUntil;  // Timestamp for invincibility
  unsigned long lastWarpTime;     // Cooldown for warp
};

// Tetroids game class
class TetroidsGame {
private:
  TetroidsPlayer player;
  TetroidsBullet bullets[TETROIDS_MAX_BULLETS];
  TetroidsAsteroid asteroids[TETROIDS_MAX_ASTEROIDS];
  
  int score;
  int wave;
  unsigned long lastShootTime;
  unsigned long gameStartTime;
  bool gameOver;
  bool autoFire;
  
  // Utility functions
  float wrapX(float x) {
    if (x < 0) return TETROIDS_SCREEN_WIDTH + x;
    if (x >= TETROIDS_SCREEN_WIDTH) return x - TETROIDS_SCREEN_WIDTH;
    return x;
  }
  
  float wrapY(float y) {
    if (y < TETROIDS_PLAY_AREA_TOP) return TETROIDS_PLAY_AREA_BOTTOM - (TETROIDS_PLAY_AREA_TOP - y);
    if (y >= TETROIDS_PLAY_AREA_BOTTOM) return TETROIDS_PLAY_AREA_TOP + (y - TETROIDS_PLAY_AREA_BOTTOM);
    return y;
  }
  
  float randomFloat(float min, float max) {
    return min + (float)random(10000) / 10000.0f * (max - min);
  }
  
  // Drawing functions
  void drawBlock(int x, int y, uint16_t color) {
    M5Cardputer.Display.fillRect(x, y, TETROIDS_BLOCK_SIZE, TETROIDS_BLOCK_SIZE, color);
    M5Cardputer.Display.drawRect(x, y, TETROIDS_BLOCK_SIZE, TETROIDS_BLOCK_SIZE, 0x2104); // Dark outline
  }
  
  void drawPlayer() {
    if (!player.alive) return;
    
    // Blink during invincibility
    bool invincible = (millis() < player.invincibleUntil);
    if (invincible && (millis() / 200) % 2 == 0) return;  // Blink effect
    
    // Convert angle to radians
    float rad = player.angle * PI / 180.0f;
    float cosA = cos(rad);
    float sinA = sin(rad);
    
    // Draw T-piece rotated smoothly
    for (int i = 0; i < 4; i++) {
      // Rotate each block around center
      float bx = TETROIDS_PLAYER_SHAPE[i][0] * TETROIDS_BLOCK_SIZE;
      float by = TETROIDS_PLAYER_SHAPE[i][1] * TETROIDS_BLOCK_SIZE;
      
      float rotX = bx * cosA - by * sinA;
      float rotY = bx * sinA + by * cosA;
      
      drawBlock(player.x + rotX, player.y + rotY, TETROIDS_COLOR_PLAYER);
    }
  }
  
  void drawBullets() {
    for (int i = 0; i < TETROIDS_MAX_BULLETS; i++) {
      if (bullets[i].active) {
        M5Cardputer.Display.fillCircle((int)bullets[i].x, (int)bullets[i].y, 2, TETROIDS_COLOR_BULLET);
      }
    }
  }
  
  void drawAsteroid(TetroidsAsteroid* ast) {
    if (!ast->active) return;
    
    uint16_t color = TETROIDS_ASTEROID_COLORS[ast->type];
    
    if (ast->isLarge) {
      // Draw full tetris piece
      for (int i = 0; i < 4; i++) {
        int bx = ast->x + TETROIDS_ASTEROID_SHAPES[ast->type][ast->rotation][i][0] * TETROIDS_BLOCK_SIZE;
        int by = ast->y + TETROIDS_ASTEROID_SHAPES[ast->type][ast->rotation][i][1] * TETROIDS_BLOCK_SIZE;
        drawBlock(bx, by, color);
      }
    } else {
      // Draw single block
      drawBlock((int)ast->x, (int)ast->y, color);
    }
  }
  
  void drawHUD() {
    // Clear top bar and draw HUD
    M5Cardputer.Display.fillRect(0, 0, TETROIDS_SCREEN_WIDTH, 10, BLACK);
    
    // Score on left
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(2, 2);
    M5Cardputer.Display.printf("Score:%d Wave:%d", score, wave);
    
    // Lives on right
    for (int i = 0; i < player.lives; i++) {
      M5Cardputer.Display.fillRect(210 + i * 8, 2, 6, 6, TETROIDS_COLOR_PLAYER);
    }
    
    // Auto-fire indicator
    if (autoFire) {
      M5Cardputer.Display.setCursor(160, 2);
      M5Cardputer.Display.setTextColor(TETROIDS_COLOR_O);
      M5Cardputer.Display.print("AUTO");
    }
  }
  
public:
  void init() {
    // Initialize player
    player.x = TETROIDS_SCREEN_WIDTH / 2;
    player.y = (TETROIDS_PLAY_AREA_TOP + TETROIDS_PLAY_AREA_BOTTOM) / 2;
    player.vx = 0;
    player.vy = 0;
    player.angle = 0;  // Facing up
    player.lives = 3;
    player.alive = true;
    player.invincibleUntil = millis() + 2000;  // 2 seconds invincibility
    player.lastWarpTime = 0;
    
    // Clear bullets and asteroids
    for (int i = 0; i < TETROIDS_MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < TETROIDS_MAX_ASTEROIDS; i++) asteroids[i].active = false;
    
    score = 0;
    wave = 1;
    lastShootTime = 0;
    gameStartTime = millis();
    gameOver = false;
    autoFire = false;
    
    spawnWave();
    
    M5Cardputer.Display.fillScreen(BLACK);
  }
  
  void shootBullet() {
    unsigned long now = millis();
    if (now - lastShootTime < 200) return; // Rate limit
    
    // Calculate forward direction based on player angle
    float rad = player.angle * PI / 180.0f;
    float dirX = sin(rad);
    float dirY = -cos(rad);  // Negative because Y increases downward
    
    // Find inactive bullet
    for (int i = 0; i < TETROIDS_MAX_BULLETS; i++) {
      if (!bullets[i].active) {
        // Spawn bullet from the nose of the T-piece
        bullets[i].x = player.x + dirX * (TETROIDS_BLOCK_SIZE * 1.5f);
        bullets[i].y = player.y + dirY * (TETROIDS_BLOCK_SIZE * 1.5f);
        bullets[i].vx = dirX * TETROIDS_BULLET_SPEED;
        bullets[i].vy = dirY * TETROIDS_BULLET_SPEED;
        bullets[i].active = true;
        lastShootTime = now;
        return;
      }
    }
  }
  
  void spawnAsteroid(float x, float y, int type, bool isLarge) {
    for (int i = 0; i < TETROIDS_MAX_ASTEROIDS; i++) {
      if (!asteroids[i].active) {
        asteroids[i].x = x;
        asteroids[i].y = y;
        asteroids[i].type = type;
        asteroids[i].rotation = random(4);
        asteroids[i].rotSpeed = randomFloat(-0.05f, 0.05f);
        asteroids[i].isLarge = isLarge;
        asteroids[i].active = true;
        
        // Random velocity
        float angle = randomFloat(0, 2 * PI);
        float speed = randomFloat(0.5f, TETROIDS_ASTEROID_SPEED);
        asteroids[i].vx = cos(angle) * speed;
        asteroids[i].vy = sin(angle) * speed;
        return;
      }
    }
  }
  
  void spawnWave() {
    int count = 3 + wave / 2;
    if (count > 6) count = 6;  // Less asteroids in landscape
    
    for (int i = 0; i < count; i++) {
      int type = random(6);
      float x, y;
      
      // Spawn at edges
      if (random(2) == 0) {
        x = random(2) == 0 ? 10 : TETROIDS_SCREEN_WIDTH - 10;
        y = randomFloat(TETROIDS_PLAY_AREA_TOP + 20, TETROIDS_PLAY_AREA_BOTTOM - 20);
      } else {
        x = randomFloat(20, TETROIDS_SCREEN_WIDTH - 20);
        y = random(2) == 0 ? TETROIDS_PLAY_AREA_TOP + 10 : TETROIDS_PLAY_AREA_BOTTOM - 10;
      }
      
      spawnAsteroid(x, y, type, true);
    }
  }
  
  void breakAsteroid(TetroidsAsteroid* ast) {
    if (!ast->isLarge) return;
    
    // Spawn 4 small blocks at piece positions
    for (int i = 0; i < 4; i++) {
      float bx = ast->x + TETROIDS_ASTEROID_SHAPES[ast->type][ast->rotation][i][0] * TETROIDS_BLOCK_SIZE;
      float by = ast->y + TETROIDS_ASTEROID_SHAPES[ast->type][ast->rotation][i][1] * TETROIDS_BLOCK_SIZE;
      
      // Scatter outward
      float angle = atan2(by - ast->y, bx - ast->x);
      float speed = 1.2f;
      
      spawnAsteroid(bx, by, ast->type, false);
      // Set velocity for the block we just spawned
      for (int j = TETROIDS_MAX_ASTEROIDS - 1; j >= 0; j--) {
        if (asteroids[j].active && !asteroids[j].isLarge) {
          asteroids[j].vx = cos(angle) * speed;
          asteroids[j].vy = sin(angle) * speed;
          break;
        }
      }
    }
    
    score += 50;
    ast->active = false;
  }
  
  void warpPlayer() {
    unsigned long now = millis();
    if (now - player.lastWarpTime < 1000) return;  // 1 second cooldown
    
    // Random teleport location
    player.x = randomFloat(30, TETROIDS_SCREEN_WIDTH - 30);
    player.y = randomFloat(TETROIDS_PLAY_AREA_TOP + 30, TETROIDS_PLAY_AREA_BOTTOM - 30);
    player.vx = 0;  // Cancel momentum
    player.vy = 0;
    
    // Brief invincibility after warp
    player.invincibleUntil = millis() + 1000;
    player.lastWarpTime = now;
  }
  
  void update() {
    if (gameOver) return;
    
    // Handle input - D/F for rotation, K for thrust, L for fire, P for warp, O for auto-fire toggle
    if (M5Cardputer.Keyboard.isPressed()) {
      // Rotation
      if (M5Cardputer.Keyboard.isKeyPressed('d')) {
        player.angle -= 4.0f;  // Counter-clockwise
      }
      if (M5Cardputer.Keyboard.isKeyPressed('f')) {
        player.angle += 4.0f;  // Clockwise
      }
      
      // Keep angle in 0-360 range
      if (player.angle < 0) player.angle += 360;
      if (player.angle >= 360) player.angle -= 360;
      
      // Thrust forward
      if (M5Cardputer.Keyboard.isKeyPressed('k')) {
        float rad = player.angle * PI / 180.0f;
        float dirX = sin(rad);
        float dirY = -cos(rad);
        
        player.vx += dirX * TETROIDS_THRUST_POWER;
        player.vy += dirY * TETROIDS_THRUST_POWER;
        
        // Limit speed
        float speed = sqrt(player.vx * player.vx + player.vy * player.vy);
        if (speed > TETROIDS_MAX_SPEED) {
          player.vx = (player.vx / speed) * TETROIDS_MAX_SPEED;
          player.vy = (player.vy / speed) * TETROIDS_MAX_SPEED;
        }
      }
      
      // Fire bullet
      if (M5Cardputer.Keyboard.isKeyPressed('l')) {
        shootBullet();
      }
      
      // Warp
      static bool lastP = false;
      if (M5Cardputer.Keyboard.isKeyPressed('p') && !lastP) {
        warpPlayer();
      }
      lastP = M5Cardputer.Keyboard.isKeyPressed('p');
      
      // Toggle auto-fire
      static bool lastO = false;
      if (M5Cardputer.Keyboard.isKeyPressed('o') && !lastO) {
        autoFire = !autoFire;
      }
      lastO = M5Cardputer.Keyboard.isKeyPressed('o');
    }
    
    // Auto-fire when enabled
    if (autoFire) {
      shootBullet();
    }
    
    // Apply friction and move player
    player.vx *= TETROIDS_FRICTION;
    player.vy *= TETROIDS_FRICTION;
    player.x += player.vx;
    player.y += player.vy;
    player.x = wrapX(player.x);
    player.y = wrapY(player.y);
    
    // Update bullets
    for (int i = 0; i < TETROIDS_MAX_BULLETS; i++) {
      if (!bullets[i].active) continue;
      
      bullets[i].x += bullets[i].vx;
      bullets[i].y += bullets[i].vy;
      
      // Wrap bullets
      bullets[i].x = wrapX(bullets[i].x);
      bullets[i].y = wrapY(bullets[i].y);
    }
    
    // Update asteroids
    for (int i = 0; i < TETROIDS_MAX_ASTEROIDS; i++) {
      if (!asteroids[i].active) continue;
      
      asteroids[i].x += asteroids[i].vx;
      asteroids[i].y += asteroids[i].vy;
      asteroids[i].x = wrapX(asteroids[i].x);
      asteroids[i].y = wrapY(asteroids[i].y);
      
      // Rotate large asteroids
      if (asteroids[i].isLarge) {
        asteroids[i].rotation += asteroids[i].rotSpeed;
        if (asteroids[i].rotation < 0) asteroids[i].rotation += 4;
        if (asteroids[i].rotation >= 4) asteroids[i].rotation -= 4;
      }
    }
    
    // Check collisions
    checkCollisions();
    
    // Check wave complete
    if (waveComplete()) {
      wave++;
      score += 100 * wave;
      spawnWave();
      delay(500);
    }
  }
  
  void checkCollisions() {
    // Bullet vs Asteroid
    for (int b = 0; b < TETROIDS_MAX_BULLETS; b++) {
      if (!bullets[b].active) continue;
      
      for (int a = 0; a < TETROIDS_MAX_ASTEROIDS; a++) {
        if (!asteroids[a].active) continue;
        
        float dx = bullets[b].x - asteroids[a].x;
        float dy = bullets[b].y - asteroids[a].y;
        float dist = sqrt(dx*dx + dy*dy);
        
        if (dist < TETROIDS_BLOCK_SIZE * 2) {
          bullets[b].active = false;
          
          if (asteroids[a].isLarge) {
            breakAsteroid(&asteroids[a]);
          } else {
            asteroids[a].active = false;
            score += 10;
          }
          break;
        }
      }
    }
    
    // Player vs Asteroid (only if not invincible)
    if (player.alive && millis() >= player.invincibleUntil) {
      for (int a = 0; a < TETROIDS_MAX_ASTEROIDS; a++) {
        if (!asteroids[a].active) continue;
        
        float dx = player.x - asteroids[a].x;
        float dy = player.y - asteroids[a].y;
        float dist = sqrt(dx*dx + dy*dy);
        
        if (dist < TETROIDS_BLOCK_SIZE * 2.0f) {
          player.lives--;
          
          if (player.lives <= 0) {
            gameOver = true;
          } else {
            // Respawn
            player.x = TETROIDS_SCREEN_WIDTH / 2;
            player.y = (TETROIDS_PLAY_AREA_TOP + TETROIDS_PLAY_AREA_BOTTOM) / 2;
            player.vx = 0;
            player.vy = 0;
            player.invincibleUntil = millis() + 2000;
          }
          return;
        }
      }
    }
  }
  
  bool waveComplete() {
    for (int i = 0; i < TETROIDS_MAX_ASTEROIDS; i++) {
      if (asteroids[i].active) return false;
    }
    return true;
  }
  
  void draw() {
    // Clear play area
    M5Cardputer.Display.fillRect(0, 10, TETROIDS_SCREEN_WIDTH, TETROIDS_SCREEN_HEIGHT - 10, BLACK);
    
    if (gameOver) {
      M5Cardputer.Display.setTextSize(2);
      M5Cardputer.Display.setTextColor(RED);
      M5Cardputer.Display.setCursor(70, 50);
      M5Cardputer.Display.print("GAME OVER");
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.setCursor(80, 75);
      M5Cardputer.Display.printf("Score: %d", score);
      M5Cardputer.Display.setCursor(60, 90);
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.print("ENTER=Restart ESC=Exit");
      return;
    }
    
    // Draw game objects
    for (int i = 0; i < TETROIDS_MAX_ASTEROIDS; i++) {
      drawAsteroid(&asteroids[i]);
    }
    drawBullets();
    drawPlayer();
    drawHUD();
  }
  
  bool isGameOver() { return gameOver; }
  
  void reset() {
    init();
  }
  
  int getScore() { return score; }
};

TetroidsGame tetroidsGame;

void initTetroids() {
  tetroidsGame.init();
  Serial.println("Tetroids initialized");
}

void handleTetroids() {
  if (checkEscapeKey()) {
    enterMenu();
    return;
  }
  
  tetroidsGame.update();
  tetroidsGame.draw();
  
  if (tetroidsGame.isGameOver()) {
    // Handle restart
    if (M5Cardputer.Keyboard.isPressed()) {
      if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) || 
          M5Cardputer.Keyboard.isKeyPressed(' ')) {
        tetroidsGame.reset();
        delay(200);
      }
    }
  }
}

// ============================================================================
// SIMPLE CALCULATOR IMPLEMENTATION
// ============================================================================

class SimpleCalculator {
private:
  String currentInput;
  String displayValue;
  double result;
  double operand1;
  char operation;
  bool waitingForOperand;
  bool hasError;
  bool justCalculated;
  
  void drawSineWaveBorder() {
    // Draw animated sine wave border around display area
    static float phase = 0;
    phase += 0.1f;
    
    uint16_t color = 0x07E0; // Green
    
    // Top and bottom borders
    for (int x = 20; x < 220; x++) {
      float wave = sin((x * 0.1f) + phase) * 3;
      M5Cardputer.Display.drawPixel(x, 25 + (int)wave, color);
      M5Cardputer.Display.drawPixel(x, 65 + (int)wave, color);
    }
    
    // Left and right borders  
    for (int y = 25; y < 65; y++) {
      float wave = sin((y * 0.15f) + phase) * 2;
      M5Cardputer.Display.drawPixel(20 + (int)wave, y, color);
      M5Cardputer.Display.drawPixel(220 + (int)wave, y, color);
    }
  }
  
  void updateDisplay() {
    // Clear screen
    M5Cardputer.Display.fillScreen(BLACK);
    
    // Title
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.setCursor(5, 5);
    M5Cardputer.Display.print("Calculator");
    
    // Draw sine wave border
    drawSineWaveBorder();
    
    // Clear display area
    M5Cardputer.Display.fillRect(22, 27, 196, 36, BLACK);
    
    // Display current value
    M5Cardputer.Display.setTextSize(2);
    if (hasError) {
      M5Cardputer.Display.setTextColor(RED);
    } else {
      M5Cardputer.Display.setTextColor(WHITE);
    }
    
    // Simple right-align calculation (approximate)
    int textWidth = displayValue.length() * 12; // Rough estimate for size 2 text
    int cursorX = 215 - textWidth;
    if (cursorX < 25) cursorX = 25; // Don't go off left edge
    
    M5Cardputer.Display.setCursor(cursorX, 35);
    M5Cardputer.Display.print(displayValue);
    
    // Show current operation
    if (operation != 0 && !waitingForOperand && !justCalculated) {
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.setTextColor(YELLOW);
      M5Cardputer.Display.setCursor(200, 50);
      M5Cardputer.Display.print(operation);
    }
    
    // Instructions
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(5, 75);
    M5Cardputer.Display.print("0-9: Numbers  =:+ _:- X:* D:/");
    M5Cardputer.Display.setCursor(5, 90);
    M5Cardputer.Display.print("ENTER: Calculate  .: Decimal");
    M5Cardputer.Display.setCursor(5, 105);
    M5Cardputer.Display.print("DEL: Clear  ESC: Exit");
  }
  
  void handleNumber(char digit) {
    if (hasError) {
      clear();
    }
    
    if (waitingForOperand || justCalculated) {
      currentInput = String(digit);
      waitingForOperand = false;
      justCalculated = false;
    } else {
      if (currentInput.length() < 15) { // Limit input length
        currentInput += digit;
      }
    }
    
    displayValue = currentInput;
    if (displayValue == "") displayValue = "0";
  }
  
  void handleDecimal() {
    if (hasError) {
      clear();
    }
    
    if (waitingForOperand || justCalculated) {
      currentInput = "0.";
      waitingForOperand = false;
      justCalculated = false;
    } else {
      if (currentInput.indexOf('.') == -1 && currentInput.length() < 14) {
        currentInput += '.';
      }
    }
    
    displayValue = currentInput;
  }
  
  void handleOperation(char op) {
    if (hasError) {
      clear();
      return;
    }
    
    double inputValue = currentInput.toDouble();
    
    if (operation != 0 && !waitingForOperand && !justCalculated) {
      // Chain operations
      calculate();
      if (hasError) return;
      inputValue = result;
    }
    
    operand1 = inputValue;
    operation = op;
    waitingForOperand = true;
    justCalculated = false;
    
    displayValue = String(operand1, 6);
    // Remove trailing zeros
    while (displayValue.endsWith("0") && displayValue.indexOf('.') != -1) {
      displayValue = displayValue.substring(0, displayValue.length() - 1);
    }
    if (displayValue.endsWith(".")) {
      displayValue = displayValue.substring(0, displayValue.length() - 1);
    }
  }
  
  void calculate() {
    if (hasError || operation == 0 || waitingForOperand) {
      return;
    }
    
    double operand2 = currentInput.toDouble();
    
    switch (operation) {
      case '+':
        result = operand1 + operand2;
        break;
      case '-':
        result = operand1 - operand2;
        break;
      case '*':
        result = operand1 * operand2;
        break;
      case '/':
        if (operand2 == 0) {
          displayValue = "Error: Div by 0";
          hasError = true;
          return;
        }
        result = operand1 / operand2;
        break;
      default:
        return;
    }
    
    // Format result
    displayValue = String(result, 6);
    // Remove trailing zeros for cleaner display
    while (displayValue.endsWith("0") && displayValue.indexOf('.') != -1) {
      displayValue = displayValue.substring(0, displayValue.length() - 1);
    }
    if (displayValue.endsWith(".")) {
      displayValue = displayValue.substring(0, displayValue.length() - 1);
    }
    
    currentInput = displayValue;
    operation = 0;
    waitingForOperand = true;
    justCalculated = true;
  }
  
  void clear() {
    currentInput = "";
    displayValue = "0";  // Show 0 instead of empty
    result = 0;
    operand1 = 0;
    operation = 0;
    waitingForOperand = false;
    hasError = false;
    justCalculated = false;
  }
  
public:
  void init() {
    clear();
    M5Cardputer.Display.fillScreen(BLACK);
    updateDisplay();
  }
  
  void handleInput() {
    if (!M5Cardputer.Keyboard.isPressed()) return;
    
    static unsigned long lastKeyTime = 0;
    if (millis() - lastKeyTime < 200) return; // Debounce
    lastKeyTime = millis();
    
    // Numbers
    for (char c = '0'; c <= '9'; c++) {
      if (M5Cardputer.Keyboard.isKeyPressed(c)) {
        handleNumber(c);
        updateDisplay();
        return;
      }
    }
    
    // Operations - remapped for M5Cardputer keyboard compatibility
    // Debug: Try different ways to detect = key
    if (M5Cardputer.Keyboard.isKeyPressed('=') || M5Cardputer.Keyboard.isKeyPressed('+')) {
      Serial.println("Addition key detected");
      handleOperation('+');  // = key for addition
      updateDisplay();
    } else if (M5Cardputer.Keyboard.isKeyPressed('_') || M5Cardputer.Keyboard.isKeyPressed('-')) {
      Serial.println("Subtraction key detected");
      handleOperation('-');  // _ key for subtraction
      updateDisplay();
    } else if (M5Cardputer.Keyboard.isKeyPressed('X') || M5Cardputer.Keyboard.isKeyPressed('x') || M5Cardputer.Keyboard.isKeyPressed('*')) {
      Serial.println("Multiplication key detected");
      handleOperation('*');  // X key for multiplication
      updateDisplay();
    } else if (M5Cardputer.Keyboard.isKeyPressed('D') || M5Cardputer.Keyboard.isKeyPressed('d') || M5Cardputer.Keyboard.isKeyPressed('/')) {
      Serial.println("Division key detected");
      handleOperation('/');  // D key for division
      updateDisplay();
    }
    // Decimal point
    else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
      handleDecimal();
      updateDisplay();
    }
    // Calculate
    else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
      calculate();
      updateDisplay();
    }
    // Clear
    else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
      clear();
      updateDisplay();
    }
  }
  
  void update() {
    // Update sine wave animation
    updateDisplay();
  }
};

SimpleCalculator calculator;

void initCalculator() {
  calculator.init();
  Serial.println("Calculator initialized");
}

void handleCalculator() {
  if (checkEscapeKey()) {
    enterMenu();
    return;
  }
  
  calculator.handleInput();
  
  // Update display every 100ms for smooth sine wave animation
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 100) {
    calculator.update();
    lastUpdate = millis();
  }
}

// ============================================
// BEACON SPAMMER APPLICATION (Spamtastic)
// ============================================

// Use weak symbol to avoid linker conflicts
extern "C" int __attribute__((weak)) ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    if (arg == 31337) return 1;
    else return 0;
}

// Beacon frame template (exact copy from Bruce)
uint8_t beaconPacket[109] = {
    0x80, 0x00, 0x00, 0x00,             // Type/Subtype: management beacon frame
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast  
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source MAC
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // BSSID
    0x00, 0x00,                         // Fragment & sequence number
    
    // Fixed parameters (timestamp + beacon interval + capability)
    0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // timestamp
    0x64, 0x00,                                       // beacon interval
    0x01, 0x04,                                       // capability info
    
    // Tagged parameters - SSID element
    0x00, 0x20,  // Element ID: SSID, Length: 32 bytes
    // SSID (32 bytes, filled dynamically)
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    
    // Supported rates element
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
    
    // DS Parameter set element  
    0x03, 0x01, 0x06
};

// Simplified 10-category beacon system
const char* satanSSIDs[] = {
    "😈 Infernal WiFi Network 😈",
    "🔥 Hellfire Broadband 🔥",
    "👹 Demon Spawn Internet 👹",
    "⚡ Lucifer's Connection ⚡",
    "🖤 Dark Lord Network 🖤",
    "🔱 Pitchfork WiFi 🔱",
    "💀 Skull & Brimstone 💀",
    "🌋 Lake of Fire WiFi 🌋",
    "👺 Beelzebub's Net 👺",
    "⛪ Blasphemy Connection ⛪",
    "🐍 Serpent's Network 🐍",
    "🗡️ Fallen Angel WiFi 🗡️",
    "🌑 Eclipse of Evil 🌑",
    "🦇 Bat Cave Network 🦇",
    "⚰️ Coffin WiFi ⚰️",
    "🕷️ Web of Sin 🕷️",
    "👻 Phantom Network 👻",
    "🔮 Crystal Ball WiFi 🔮",
    "🎭 Mask of Deception 🎭",
    "⚡ Wrath Connection ⚡"
};

const char* funnySSIDs[] = {
    "🤵 Abraham Linksys 🤵",
    "🛩️ Pretty Fly for a WiFi 🛩️", 
    "👶 Hide Your Kids, Hide Your WiFi 👶",
    "🥋 Wu Tang LAN 🥋",
    "😏 Router? I Barely Know Here! 😏",
    "🦕 The LAN Before Time 🦕",
    "🐑 Silence of the LANs 🐑",
    "🏰 House LANnister 🏰",
    "🎵 This LAN Is My LAN 🎵",
    "🚫 No More Mister WiFi 🚫",
    "👨‍🚀 LAN Solo 👨‍🚀", 
    "🚶‍♂️ Get Off My LAN 🚶‍♂️",
    "🔬 Bill Wi The Science Fi 🔬",
    "🌐 I Am The Internet 🌐",
    "🔥 Drop It Like Its Hotspot 🔥",
    "👑 Martin Router King 👑",
    "❄️ Winternet Is Coming ❄️",
    "💕 Tell My WiFi Love Her 💕",
    "💔 It Hurts When IP 💔",
    "🌮 Nacho WiFi 🌮"
};

const char* governmentSSIDs[] = {
    "🕵️ NSA Mobile Monitoring 🕵️", 
    "🎯 CIA Field Office WiFi 🎯",
    "💊 DEA Surveillance Node 💊",
    "🚔 Police_Van_#3 🚔", 
    "📡 Mobile Command Center 📡",
    "⚔️ SWAT Team Alpha ⚔️",
    "🛡️ DHS Mobile Unit 🛡️",
    "💣 ATF Tactical Van 💣",
    "🕴️ Secret Service Mobile 🕴️",
    "🛂 Border Patrol Unit 🛂",
    "💰 IRS Investigation Van 💰",
    "🏛️ Homeland Security Net 🏛️",
    "⭐ Federal Marshal Unit ⭐",
    "🏦 US Treasury Mobile 🏦",
    "🛃 Customs Border WiFi 🛃",
    "🔍 FBI Evidence Unit 🔍",
    "⚖️ DOJ Investigation ⚖️",
    "✈️ TSA Security Check ✈️",
    "🧊 ICE Mobile Command 🧊",
    "🏛️ Federal Court WiFi 🏛️"
};

const char* hackerSSIDs[] = {
    "🏴‍☠️ Anonymous Network 🏴‍☠️",
    "💻 Hackerman WiFi 💻",
    "⚡ Elite Hacker Club ⚡",
    "🔓 Zero Day Exploit 🔓",
    "🐧 Kali Linux Network 🐧",
    "🎯 Penetration Testing 🎯",
    "🧠 Social Engineering 🧠",
    "💉 SQL Injection Here 💉",
    "💥 Buffer Overflow Net 💥",
    "🎩 BlackHat Convention 🎩",
    "🤖 DefCon Official 🤖",
    "📱 2600 Meeting Point 📱",
    "💿 Exploit Database 💿",
    "🔍 Packet Sniffing Here 🔍",
    "🦈 Wireshark Capture 🦈",
    "🗺️ Nmap Scan Network 🗺️",
    "🔑 John The Ripper 🔑",
    "📡 Aircrack-ng Ready 📡",
    "🕳️ Burp Suite Pro 🕳️",
    "🛡️ Red Team Network 🛡️"
};

const char* gamerSSIDs[] = {
    "🏆 Achievement Unlocked 🏆",
    "🎮 Press Start To Connect 🎮",
    "💀 Game Over Man Game Over 💀",
    "🎯 All Your Base 🎯",
    "🍰 The Cake Is A LIE 🍰",
    "🥇 Xbox Live Gold Required 🥇",
    "🎮 PlayerOne Has Joined 🎮",
    "🔄 Respawn Point Alpha 🔄",
    "👹 Final Boss WiFi 👹",
    "🪙 Insert Coin To Continue 🪙",
    "⚔️ Critical Hit Network ⚔️",
    "🗡️ Legendary Loot WiFi 🗡️",
    "💥 Noob Tube Network 💥",
    "🎯 Headshot WiFi 🎯",
    "💥 Epic Fail Network 💥",
    "🏴‍☠️ Pwned Network 🏴‍☠️",
    "⏰ Lag Spike Central ⏰",
    "⛺ Camping Spot WiFi ⛺",
    "💀 Spawn Kill Network 💀",
    "😡 Rage Quit WiFi 😡"
};

const char* horrorSSIDs[] = {
    "🦇 Haunted House WiFi 🦇",
    "🎃 Pumpkin Patch Network 🎃",
    "💀 Spooky Skeleton Net 💀",
    "🍬 Trick Or Treat WiFi 🍬",
    "🧙‍♀️ Witch's Brew Network 🧙‍♀️",
    "🧟‍♂️ Zombie Apocalypse 🧟‍♂️",
    "👻 Ghost Buster WiFi 👻",
    "🧛‍♂️ Vampire's Lair 🧛‍♂️",
    "⚡ Frankenstein's Lab ⚡",
    "🏺 Mummy's Tomb WiFi 🏺",
    "🕷️ Creepy Crawlers Net 🕷️",
    "😱 Boo! Scared You 😱",
    "🎬 Horror Movie Marathon 🎬",
    "👹 Monster Mash WiFi 👹",
    "⚰️ Graveyard Shift ⚰️",
    "💀 Nightmare Network 💀",
    "🔥 Salem Witch Trial 🔥",
    "🌙 Full Moon Rising 🌙",
    "🌃 Midnight Horror 🌃",
    "😈 Scream Factory WiFi 😈"
};

const char* offensiveSSIDs[] = {
    "😠 YellAtYourKidsNotMe 😠",
    "💩 Your WiFi Sucks 💩",
    "🚫 Stop Using My WiFi 🚫",
    "🖕 Password Is Go F Yourself 🖕",
    "😡 I Hate My Neighbors 😡",
    "💸 Get Your Own Internet 💸",
    "🎒 Router Of Holding 🎒",
    "💪 WiFi So Hard 💪",
    "🚨 Bandwidth Thief Detected 🚨",
    "👵 Mom Click Here For WiFi 👵",
    "⛔ Stop Stealing My WiFi ⛔",
    "💰 Pay For Your Own Internet 💰",
    "🏴‍☠️ Neighbor WiFi Thief 🏴‍☠️",
    "🛒 Buy Your Own Router 🛒",
    "🚨 WiFi Leech Detected 🚨",
    "💼 Get A Job Buy WiFi 💼",
    "🤏 Stop Being Cheap 🤏",
    "📡 WiFi Moocher Alert 📡",
    "🏠 Freeloading Neighbor 🏠",
    "🔄 Password Changed Again 🔄"
};

const char* techSSIDs[] = {
    "⏳ Loading... ⏳",
    "❌ 404 Network Not Found ❌",
    "🔄 CONNECTING... 🔄",
    "🚫 Access Denied 🚫",
    "⏰ Connection Timeout ⏰",
    "🔍 Network Error 404 🔍",
    "📡 SSID Not Found 📡",
    "⏳ Please Wait... ⏳",
    "🎬 Buffering... 🎬",
    "🔥 System Overload 🔥",
    "💀 Fatal Error 💀",
    "💙 Blue Screen WiFi 💙",
    "⚠️ Kernel Panic ⚠️",
    "💥 Segmentation Fault 💥",
    "🧠 Memory Leak Detected 🧠",
    "📚 Stack Overflow 📚",
    "🚫 Null Pointer Exception 🚫",
    "⚡ Runtime Error ⚡",
    "🔨 Compilation Failed 🔨",
    "📝 Syntax Error 📝"
};

const char* alienSSIDs[] = {
    "🛸 Area 51 Guest Network 🛸",
    "👽 UFO Landing Zone WiFi 👽",
    "🛸 Alien Abduction Center 🛸",
    "👽 Roswell Incident WiFi 👽",
    "📞 ET Phone Home 📞",
    "👽 Greys Communication Hub 👽",
    "🔴 Mars Base Alpha 🔴",
    "🌌 Galactic Federation Net 🌌",
    "🌟 Intergalactic WiFi 🌟",
    "⚡ Beam Me Up Scotty ⚡",
    "👽 Close Encounters Net 👽",
    "🕵️ X-Files Investigation 🕵️",
    "🛸 Flying Saucer WiFi 🛸",
    "🌾 Crop Circle Network 🌾",
    "🔬 Alien Autopsy Lab 🔬",
    "🛰️ Space Probe WiFi 🛰️",
    "👽 Extraterrestrial Net 👽",
    "📡 UFO Sighting Report 📡",
    "🏢 Alien Embassy WiFi 🏢",
    "🚀 Mothership Command 🚀"
};

const char* zombieSSIDs[] = {
    "🧟‍♂️ Zombie Outbreak WiFi 🧟‍♂️",
    "💀 Walking Dead Network 💀",
    "🧟‍♀️ Undead Connection 🧟‍♀️",
    "🏥 CDC Quarantine Zone 🏥",
    "⚠️ ZOMBIE ALERT WiFi ⚠️",
    "🧟‍♂️ Brain Eaters Network 🧟‍♂️",
    "💉 Infection Control WiFi 💉",
    "🏚️ Safe House Network 🏚️",
    "🔫 Zombie Hunters WiFi 🔫",
    "💀 Resident Evil Net 💀",
    "🧟‍♀️ Shambling Horde WiFi 🧟‍♀️",
    "⚠️ Biohazard Network ⚠️",
    "🏥 T-Virus Outbreak 🏥",
    "💀 Left 4 Dead WiFi 💀",
    "🧟‍♂️ Dead Rising Network 🧟‍♂️",
    "⚠️ Zombie Survival WiFi ⚠️",
    "💉 Patient Zero Network 💉",
    "🔥 Burn The Infected 🔥",
    "🏚️ Barricade Network 🏚️",
    "💀 Day Of The Dead WiFi 💀"
};

// Beacon category structure
struct BeaconCategory {
    const char* name;
    const char** ssids;
    int count;
};

BeaconCategory beaconCategories[] = {
    {"Satan & Dark", satanSSIDs, sizeof(satanSSIDs)/sizeof(satanSSIDs[0])},
    {"Funny Names", funnySSIDs, sizeof(funnySSIDs)/sizeof(funnySSIDs[0])},
    {"Government/LEO", governmentSSIDs, sizeof(governmentSSIDs)/sizeof(governmentSSIDs[0])},
    {"Hacker/Security", hackerSSIDs, sizeof(hackerSSIDs)/sizeof(hackerSSIDs[0])},
    {"Gaming", gamerSSIDs, sizeof(gamerSSIDs)/sizeof(gamerSSIDs[0])},
    {"Horror", horrorSSIDs, sizeof(horrorSSIDs)/sizeof(horrorSSIDs[0])},
    {"Offensive", offensiveSSIDs, sizeof(offensiveSSIDs)/sizeof(offensiveSSIDs[0])},
    {"Tech/Error", techSSIDs, sizeof(techSSIDs)/sizeof(techSSIDs[0])},
    {"Aliens & UFOs", alienSSIDs, sizeof(alienSSIDs)/sizeof(alienSSIDs[0])},
    {"Zombie Apocalypse", zombieSSIDs, sizeof(zombieSSIDs)/sizeof(zombieSSIDs[0])}
};

// WiFi channels to cycle through
const uint8_t wifiChannels[] = {1, 6, 11, 3, 8, 13};
int currentWifiChannel = 0;

// Beacon spammer states
enum BeaconSpammerState {
    BEACON_CATEGORY_MENU,
    BEACON_SPAMMING  
};

BeaconSpammerState beaconState = BEACON_CATEGORY_MENU;
int selectedBeaconCategory = 0;
int currentBeaconSSIDIndex = 0;
int totalBeaconCategories = sizeof(beaconCategories) / sizeof(beaconCategories[0]);
bool beaconSpamming = false;
unsigned long lastBeaconSpam = 0;
unsigned long beaconSpamCount = 0;
BeaconCategory* currentBeaconCategory = nullptr;
int beaconMenuStartIndex = 0;

// Scrolling beacon display
String currentlySpammingBeacon = "";
unsigned long lastBeaconScrollUpdate = 0;
int beaconScrollPosition = 0;
const int BEACON_SCROLL_SPEED = 300;
const int BEACON_SCROLL_AREA_WIDTH = 20;

// Forward declarations for beacon functions
void showBeaconCategoryMenu();
void showBeaconSpamming(); 
void spamAllBeaconsInCategory();
void handleBeaconCategoryInput();
void handleBeaconSpammingInput();
void updateScrollingBeaconDisplay();
void drawScrollingBeaconText(String text, int y, uint16_t color);

void initBeaconSpammer() {
  Serial.println("Beacon Spammer initialized - Spamtastic!");
  beaconState = BEACON_CATEGORY_MENU;
  selectedBeaconCategory = 0;
  beaconSpamming = false;
  beaconSpamCount = 0;
  currentlySpammingBeacon = "";
  beaconScrollPosition = 0;
  
  // Reset WiFi for beacon mode
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  
  // Initialize WiFi in AP mode for beacon injection
  WiFi.mode(WIFI_AP);
  esp_wifi_start();
  delay(100);
  
  // Set to a clean channel
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  
  showBeaconCategoryMenu();
}

void handleBeaconSpammer() {
  // Update scrolling display for spamming beacons
  updateScrollingBeaconDisplay();
  
  switch (beaconState) {
    case BEACON_CATEGORY_MENU:
      handleBeaconCategoryInput();
      break;
    case BEACON_SPAMMING:
      handleBeaconSpammingInput();
      // Handle beacon spamming - Bruce style: spam ALL beacons in category rapidly
      if (beaconSpamming && millis() - lastBeaconSpam > 50) {  // Every 50ms, spam entire category
        spamAllBeaconsInCategory();
        lastBeaconSpam = millis();
        beaconSpamCount++;
        
        // Change channel after each full cycle through all beacons
        currentWifiChannel++;
        if (currentWifiChannel >= sizeof(wifiChannels) / sizeof(wifiChannels[0])) {
          currentWifiChannel = 0;
        }
        
        // Update display every cycle to show progress
        showBeaconSpamming();
      }
      break;
  }
}

void handleBeaconCategoryInput() {
  M5Cardputer.update();
  
  if (checkEscapeKey()) {
    beaconSpamming = false;
    currentlySpammingBeacon = "";
    beaconScrollPosition = 0;
    enterMenu();
    return;
  }
  
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto keys = M5Cardputer.Keyboard.keysState();
    String key = "";
    for (auto k : keys.word) {
      if (k) key += (char)k;
    }
    
    // Up arrow: Navigate up in category menu
    if (key.indexOf(";") >= 0) {
      selectedBeaconCategory--;
      if (selectedBeaconCategory < 0) {
        selectedBeaconCategory = totalBeaconCategories - 1;
      }
      showBeaconCategoryMenu();
    }
    // Down arrow: Navigate down in category menu  
    else if (key.indexOf(".") >= 0) {
      selectedBeaconCategory++;
      if (selectedBeaconCategory >= totalBeaconCategories) {
        selectedBeaconCategory = 0;
      }
      showBeaconCategoryMenu();
    }
    // Enter: Start spamming entire selected category
    else if (keys.enter || key.indexOf('\n') >= 0 || key.indexOf('\r') >= 0) {
      currentBeaconCategory = &beaconCategories[selectedBeaconCategory];
      currentBeaconSSIDIndex = 0;
      beaconSpamming = true;
      beaconSpamCount = 0;
      beaconState = BEACON_SPAMMING;
      
      // Change to a different channel immediately to avoid conflicts
      currentWifiChannel = random(0, sizeof(wifiChannels)/sizeof(wifiChannels[0]));
      esp_wifi_set_channel(wifiChannels[currentWifiChannel], WIFI_SECOND_CHAN_NONE);
      
      showBeaconSpamming();
    }
  }
}

void handleBeaconSpammingInput() {
  M5Cardputer.update();
  
  if (checkEscapeKey()) {
    beaconSpamming = false;
    currentlySpammingBeacon = "";
    beaconScrollPosition = 0;
    enterMenu();
    return;
  }
  
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    // Any key: Stop spamming and go back to category menu
    beaconSpamming = false;
    currentlySpammingBeacon = "";
    beaconScrollPosition = 0;
    beaconState = BEACON_CATEGORY_MENU;
    showBeaconCategoryMenu();
  }
}

void showBeaconCategoryMenu() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(1);
  
  // Title
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.setCursor(10, 5);
  M5Cardputer.Display.println("Beacon Categories");
  
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(20, 20);
  M5Cardputer.Display.println("SPAMTASTIC!");
  
  // Ensure selected category is visible in the window
  if (selectedBeaconCategory < beaconMenuStartIndex) {
    beaconMenuStartIndex = selectedBeaconCategory;
  } else if (selectedBeaconCategory >= beaconMenuStartIndex + 4) {
    beaconMenuStartIndex = selectedBeaconCategory - 3;
  }
  
  // Show visible categories (4 at a time to fit screen better)
  int y = 45;
  int maxVisible = min(4, totalBeaconCategories - beaconMenuStartIndex);
  
  for (int i = 0; i < maxVisible; i++) {
    int categoryIndex = beaconMenuStartIndex + i;
    
    if (categoryIndex == selectedBeaconCategory) {
      // Highlight selected item
      M5Cardputer.Display.fillRect(0, y-3, M5Cardputer.Display.width(), 28, BLUE);
      M5Cardputer.Display.setTextColor(WHITE);
      M5Cardputer.Display.setCursor(2, y);
      M5Cardputer.Display.print(">");
    } else {
      M5Cardputer.Display.setTextColor(CYAN);
    }
    
    // Truncate long category names
    String catName = String(beaconCategories[categoryIndex].name);
    if (catName.length() > 14) {
      catName = catName.substring(0, 11) + "...";
    }
    
    M5Cardputer.Display.setTextSize(2);  // Double font size
    M5Cardputer.Display.setCursor(12, y);
    M5Cardputer.Display.print(catName);
    M5Cardputer.Display.setTextSize(1);  // Reset to normal for count
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(200, y+5);
    M5Cardputer.Display.print("(");
    M5Cardputer.Display.print(beaconCategories[categoryIndex].count);
    M5Cardputer.Display.print(")");
    y += 30;
  }
  
  // Show scroll indicators
  if (beaconMenuStartIndex > 0) {
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(80, 30);
    M5Cardputer.Display.print("^ More above");
  }
  if (beaconMenuStartIndex + 4 < totalBeaconCategories) {
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.setCursor(80, 175);
    M5Cardputer.Display.print("v More below");
  }
  
  // Show current selection info
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setCursor(5, 190);
  M5Cardputer.Display.print("Selected: ");
  M5Cardputer.Display.print(selectedBeaconCategory + 1);
  M5Cardputer.Display.print("/");
  M5Cardputer.Display.print(totalBeaconCategories);
  
  // Button instructions
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setCursor(5, 205);
  M5Cardputer.Display.print("Arrows:Navigate Enter:SPAM!");
  
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setCursor(5, 220);
  M5Cardputer.Display.print("M/Fn: Back to Menu");
}

void showBeaconSpamming() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(1);
  
  M5Cardputer.Display.setTextColor(RED);
  M5Cardputer.Display.setCursor(5, 10);
  M5Cardputer.Display.print("FULL CATEGORY SPAM!");
  
  // Show category being spammed
  M5Cardputer.Display.setTextSize(2);  // Double font size
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(5, 30);
  M5Cardputer.Display.print("Spamming Category:");
  
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setCursor(5, 55);
  M5Cardputer.Display.print(currentBeaconCategory->name);
  
  // Show current SSID being transmitted (last one in cycle)
  M5Cardputer.Display.setTextColor(WHITE);
  M5Cardputer.Display.setCursor(5, 85);
  M5Cardputer.Display.print("Last SSID:");
  
  String ssid = String(currentBeaconCategory->ssids[currentBeaconSSIDIndex]);
  if (ssid.length() > 12) {  // Shorter length for bigger font
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(5, 110);
    M5Cardputer.Display.print(ssid.substring(0, 12));
    if (ssid.length() > 12) {
      M5Cardputer.Display.setCursor(5, 135);
      M5Cardputer.Display.print(ssid.substring(12));
    }
  } else {
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(5, 110);
    M5Cardputer.Display.print(ssid);
  }
  
  // Show spam stats
  M5Cardputer.Display.setTextSize(1);  // Reset to normal for stats
  M5Cardputer.Display.setTextColor(YELLOW);
  M5Cardputer.Display.setCursor(5, 165);
  M5Cardputer.Display.print("Full Cycles: ");
  M5Cardputer.Display.print(beaconSpamCount);
  
  M5Cardputer.Display.setTextColor(MAGENTA);
  M5Cardputer.Display.setCursor(5, 180);
  M5Cardputer.Display.print("Total SSIDs: ");
  M5Cardputer.Display.print(currentBeaconCategory->count);
  
  M5Cardputer.Display.setTextColor(MAGENTA);
  M5Cardputer.Display.setCursor(5, 195);
  M5Cardputer.Display.print("Channel: ");
  M5Cardputer.Display.print(wifiChannels[currentWifiChannel]);
  
  M5Cardputer.Display.setTextColor(CYAN);
  M5Cardputer.Display.setCursor(5, 210);
  M5Cardputer.Display.print("Rate: Bruce Style Fast!");
  
  // Instructions
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.setCursor(25, 225);
  M5Cardputer.Display.print("Any Key: Stop");
  
  // Show scrolling beacon display at bottom
  drawScrollingBeaconText("LIVE: " + currentlySpammingBeacon, 200, RED);
}

void spamAllBeaconsInCategory() {
  // Ensure we have a valid category
  if (!currentBeaconCategory) {
    return;
  }
  
  // Set current channel
  esp_wifi_set_channel(wifiChannels[currentWifiChannel], WIFI_SECOND_CHAN_NONE);
  
  // Bruce-style: Rapidly send ALL beacons in the category
  for (int ssidIndex = 0; ssidIndex < currentBeaconCategory->count; ssidIndex++) {
    String ssid = String(currentBeaconCategory->ssids[ssidIndex]);
    
    // Create a fresh beacon packet for each SSID
    uint8_t freshBeacon[109];
    memcpy(freshBeacon, beaconPacket, sizeof(beaconPacket));
    
    // Fill SSID into fresh beacon packet
    int ssidLen = min(32, (int)ssid.length());
    
    // COMPLETELY clear SSID area first - fill with spaces (0x20)
    memset(&freshBeacon[38], 0x20, 32);
    
    // Copy SSID characters directly 
    for (int i = 0; i < ssidLen; i++) {
      freshBeacon[38 + i] = ssid.c_str()[i];
    }
    
    // Set SSID length in the packet (CRITICAL!)
    freshBeacon[37] = (uint8_t)ssidLen;
    
    // Generate completely random MAC addresses for each beacon
    uint32_t macSeed = millis() + ssidIndex + random(1000);
    randomSeed(macSeed);
    
    for (int i = 10; i < 16; i++) {
      freshBeacon[i] = random(256);        // Source MAC
      freshBeacon[i + 6] = freshBeacon[i]; // Copy to BSSID
    }
    
    // Make MAC locally administered and unicast
    freshBeacon[10] = (freshBeacon[10] & 0xFC) | 0x02;
    freshBeacon[16] = (freshBeacon[16] & 0xFC) | 0x02;
    
    // Update timestamp to current time
    uint64_t timestamp = micros();
    memcpy(&freshBeacon[24], &timestamp, 8);
    
    // Update DS parameter with current channel
    freshBeacon[108] = wifiChannels[currentWifiChannel];
    
    // Send the beacon with multiple bursts like Bruce (3 times with 1ms delay)
    for (int burst = 0; burst < 3; burst++) {
      esp_wifi_80211_tx(WIFI_IF_AP, freshBeacon, sizeof(freshBeacon), false);
      delayMicroseconds(1000); // 1ms delay like Bruce
    }
    
    // Update current SSID index for display and scrolling text
    currentBeaconSSIDIndex = ssidIndex;
    currentlySpammingBeacon = ssid;  // Update the scrolling display text
    
    // Small delay between SSIDs (like Bruce)
    delayMicroseconds(100);
  }
}

// Update scrolling beacon display
void updateScrollingBeaconDisplay() {
  if (millis() - lastBeaconScrollUpdate > BEACON_SCROLL_SPEED) {
    beaconScrollPosition++;
    lastBeaconScrollUpdate = millis();
  }
}

// Draw scrolling text at bottom of screen
void drawScrollingBeaconText(String text, int y, uint16_t color) {
  if (text.isEmpty()) return;
  
  // Clear the line first
  M5Cardputer.Display.fillRect(0, y, M5Cardputer.Display.width(), 16, BLACK);
  
  // If text fits on screen, just display it normally
  if (text.length() <= BEACON_SCROLL_AREA_WIDTH) {
    M5Cardputer.Display.setTextColor(color);
    M5Cardputer.Display.setCursor(5, y);
    M5Cardputer.Display.print(text);
    return;
  }
  
  // Create scrolling effect by cycling through the text
  String displayText = text + "    "; // Add spacing between loops
  int totalLength = displayText.length();
  
  // Calculate scroll position within the text
  int startPos = beaconScrollPosition % totalLength;
  
  // Build the display string
  String scrollingDisplay = "";
  for (int i = 0; i < BEACON_SCROLL_AREA_WIDTH && i < totalLength; i++) {
    int charIndex = (startPos + i) % totalLength;
    scrollingDisplay += displayText.charAt(charIndex);
  }
  
  M5Cardputer.Display.setTextColor(color);
  M5Cardputer.Display.setCursor(5, y);
  M5Cardputer.Display.print(scrollingDisplay);
}

// ===== SIMPLE MATRIX RAIN FOR MP3 SCREENSAVER =====

void initSimpleMatrix() {
  // Initialize matrix drops randomly
  for (int i = 0; i < MAX_MATRIX_DROPS; i++) {
    matrixDrops[i].x = random(0, M5Cardputer.Display.width() / 6); // 6-pixel wide characters
    matrixDrops[i].y = random(-20, 0); // Start above screen
    matrixDrops[i].speed = random(1, 4);
    matrixDrops[i].character = random(33, 127); // ASCII printable characters
    matrixDrops[i].active = (random(100) < 70); // 70% chance to be active
  }
}

void drawSimpleMatrix() {
  // Fade the screen slightly instead of clearing completely (creates trail effect)
  M5Cardputer.Display.fillRect(0, 20, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 40, TFT_BLACK);
  
  M5Cardputer.Display.setTextSize(1);
  
  // Update and draw each matrix drop
  for (int i = 0; i < MAX_MATRIX_DROPS; i++) {
    if (matrixDrops[i].active) {
      // Move drop down
      matrixDrops[i].y += matrixDrops[i].speed;
      
      // Draw the character in green
      M5Cardputer.Display.setTextColor(TFT_GREEN);
      M5Cardputer.Display.setCursor(matrixDrops[i].x * 6, matrixDrops[i].y);
      M5Cardputer.Display.write(matrixDrops[i].character);
      
      // Reset drop if it goes off screen
      if (matrixDrops[i].y > M5Cardputer.Display.height()) {
        matrixDrops[i].x = random(0, M5Cardputer.Display.width() / 6);
        matrixDrops[i].y = random(-20, -5);
        matrixDrops[i].speed = random(1, 4);
        matrixDrops[i].character = random(33, 127);
        matrixDrops[i].active = (random(100) < 70);
      }
      
      // Occasionally change character (matrix effect)
      if (random(100) < 5) {
        matrixDrops[i].character = random(33, 127);
      }
    }
  }
  
  // FIXED: Add display() call to show the matrix animation
  M5Cardputer.Display.display();
}
