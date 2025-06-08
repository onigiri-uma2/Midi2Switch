// =============================
// ATOMS3-B: UART to USB HID for Switch
// =============================
#include <M5AtomS3.h>
#include <switch_ESP32.h>

#define UART_TX 2
#define UART_RX 1

// --- 定数 ---
constexpr int MY_NOTE_MIN = 48;
constexpr int MY_NOTE_MAX = 72;

// --- HAT方向インデックス列挙 ---
enum HatLogicalDir { HAT_UP=0, HAT_DOWN, HAT_LEFT, HAT_RIGHT, HAT_HATCOUNT };

// --- グローバル ---
HardwareSerial uartSerial(1);
NSGamepad Gamepad;

// --- 入力タイプ ---
enum InputType : uint8_t {
    BUTTON, HAT, STICK
};

struct InputMapping {
    InputType type;
    uint16_t button = 0;
    uint8_t hat = NSGAMEPAD_DPAD_CENTERED;
    struct { uint8_t lx = 0, ly = 0, rx = 0, ry = 0; } stick;
};

// --- キーマッピング配列１ ---
const InputMapping noteToInput1[25] = {
    {BUTTON, NSButton_LeftThrottle},      // 48: ZL
    {BUTTON, 0},                          // 49:未使用
    {BUTTON, NSButton_RightThrottle},     // 50: ZR
    {BUTTON, 0},                          // 51:未使用
    {HAT,    0, HAT_DOWN},                // 52: ↓
    {BUTTON, NSButton_B},                 // 53: B
    {BUTTON, 0},                          // 54: 未使用
    {HAT,    0, HAT_LEFT},                // 55: ←
    {BUTTON, 0},                          // 56: 未使用
    {BUTTON, NSButton_Y},                 // 57: Y
    {BUTTON, 0},                          // 58: 未使用
    {HAT,    0, HAT_UP},                  // 59: ↑
    {BUTTON, NSButton_X},                 // 60: X
    {BUTTON, 0},                          // 61: 未使用
    {HAT,    0, HAT_RIGHT},               // 62: →
    {BUTTON, 0},                          // 63: 未使用
    {BUTTON, NSButton_A},                 // 64: A
    {BUTTON, NSButton_LeftTrigger},       // 65: L
    {BUTTON, 0},                          // 66: 未使用
    {BUTTON, NSButton_RightTrigger},      // 67: R
    {BUTTON, 0},                          // 68: 未使用
    {STICK,  0, 0, { 128, 0, 0, 0 }},     // 69: 左←
    {BUTTON, 0},                          // 70: 未使用
    {STICK,  0, 0, { 0, 0, 128, 0 }},     // 71: 右←
    {STICK,  0, 0, { 127, 0, 0, 0 }},     // 72: 左→
};

// --- キーマッピング配列２ ---
const InputMapping noteToInput2[25] = {
    {HAT,    0, HAT_DOWN},                // 48: ↓
    {BUTTON, 0},                          // 49: 未使用
    {HAT,    0, HAT_LEFT},                // 50: ←
    {BUTTON, 0},                          // 51: 未使用
    {HAT,    0, HAT_UP},                  // 52: ↑
    {STICK,  0, 0, {0,127,0,0}},          // 53: 左↓
    {BUTTON, 0},                          // 54: 未使用
    {STICK,  0, 0, {128,0,0,0}},          // 55: 左←
    {BUTTON, 0},                          // 56: 未使用
    {BUTTON, NSButton_LeftTrigger},       // 57: L
    {BUTTON, 0},                          // 58: 未使用
    {BUTTON, NSButton_LeftThrottle},      // 59: ZL
    {STICK,  0, 0, {0,0,0,127}},          // 60: 右↓
    {BUTTON, 0},                          // 61: 未使用
    {STICK,  0, 0, {0,0,127,0}},          // 62: 右→
    {BUTTON, 0},                          // 63: 未使用
    {STICK,  0, 0, {0,0,0,128}},          // 64: 右↑
    {BUTTON, NSButton_B},                 // 65: B
    {BUTTON, 0},                          // 66: 未使用
    {BUTTON, NSButton_A},                 // 67: A
    {BUTTON, 0},                          // 68: 未使用
    {BUTTON, NSButton_X},                 // 69: X
    {BUTTON, 0},                          // 70: 未使用
    {BUTTON, NSButton_RightTrigger},      // 71: R
    {BUTTON, NSButton_RightThrottle},     // 72: ZR
};

// --- HAT方向フラグ（配列で管理） ---
bool hat_flags[HAT_HATCOUNT] = {false, false, false, false}; // up, down, left, right

// --- マッピング切替 ---
bool mapping2Mode = false;
void toggleMappingMode() { mapping2Mode = !mapping2Mode; }

// --- スティック状態 ---
int8_t lastLeftX = 0, lastLeftY = 0, lastRightX = 0, lastRightY = 0;

// --- ノートON/OFFからSwitch入力変換 ---
void onMidiNoteReceived(uint8_t note, bool on) {
    if (note < MY_NOTE_MIN || note > MY_NOTE_MAX) return;
    const InputMapping* mapping = mapping2Mode ? noteToInput2 : noteToInput1;
    const InputMapping& input = mapping[note - MY_NOTE_MIN];

    switch (input.type) {
        case BUTTON:
            if (on) Gamepad.press(input.button);
            else    Gamepad.release(input.button);
            break;
        case HAT:
            if (input.hat < HAT_HATCOUNT) {
                hat_flags[input.hat] = on;
                Gamepad.dPad(
                    hat_flags[HAT_UP],
                    hat_flags[HAT_DOWN],
                    hat_flags[HAT_LEFT],
                    hat_flags[HAT_RIGHT]
                );
            }
            break;
        case STICK:
            if (on) {
                if (input.stick.lx != 0) {
                    lastLeftX = input.stick.lx;
                    lastLeftY = 0;
                }
                if (input.stick.ly != 0) {
                    lastLeftX = 0;
                    lastLeftY = input.stick.ly;
                }
                if (input.stick.rx != 0) {
                    lastRightX = input.stick.rx;
                    lastRightY = 0;
                }
                if (input.stick.ry != 0) {
                    lastRightX = 0;
                    lastRightY = input.stick.ry;
                }
            } else {
                if (input.stick.lx != 0) {
                    lastLeftX = 0;
                }
                if (input.stick.ly != 0) {
                    lastLeftY = 0;
                }
                if (input.stick.rx != 0) {
                    lastRightX = 0;
                }
                if (input.stick.ry != 0) {
                    lastRightY = 0;
                }
            }
            Gamepad.allAxes(lastRightY, lastRightX, lastLeftY, lastLeftX);
            break;
        default:
            break;
    }
    Gamepad.loop();
}

uint8_t readByteWithTimeout() {
    unsigned long start = millis();
    while (!uartSerial.available()) {
        if (millis() - start > 100) return 0;
    }
    return uartSerial.read();
}

// --- setup ---
void setup() {
    AtomS3.begin(true);
    AtomS3.dis.setBrightness(100);
    AtomS3.dis.drawpix(0x000088);
    AtomS3.update();

    uartSerial.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

    Gamepad.begin();
    USB.begin();
}

// --- 物理ボタンA：長押しでボタン配置切り替え、短押しでマッピング切り替え ---
unsigned long btnPressStart = 0;
bool btnWasPressed = false;

// --- loop ---
void loop() {
    AtomS3.update();

    if (uartSerial.available()) {
        if (uartSerial.read() == 0xF0) {
            uint8_t note = readByteWithTimeout();
            uint8_t on   = readByteWithTimeout();
            if (readByteWithTimeout() == 0xF7) {
                onMidiNoteReceived(note, on);
            }
        }
    }

    // ボタンA押下（初回だけ判定）
    if (AtomS3.BtnA.isPressed() && !btnWasPressed) {
        btnPressStart = millis();
        btnWasPressed = true;
        Gamepad.press(NSButton_RightStick); // RSボタン押下
        Gamepad.loop();
    }
    // ボタンA離したとき
    if (AtomS3.BtnA.wasReleased()) {
        unsigned long pressDuration = millis() - btnPressStart;
        if (pressDuration < 1000) { // 1秒未満→マッピング切替
            toggleMappingMode();
        }
        Gamepad.release(NSButton_RightStick); // RSボタン離す
        Gamepad.loop();
        btnWasPressed = false;
    }
}
