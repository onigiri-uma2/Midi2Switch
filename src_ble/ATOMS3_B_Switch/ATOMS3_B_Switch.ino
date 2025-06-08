// =============================
// ATOMS3-B: BLE to USB HID for Switch
// =============================
#include <M5AtomS3.h>
#include <switch_ESP32.h>
#include <NimBLEDevice.h>

// UUIDs (ATOMS3-Aと同じ)
#define SERVICE_UUID        "b1c0a5f3-7cc0-44cf-b21b-58b6b4f3a8e0"
#define CHARACTERISTIC_UUID "4739f3ad-80ad-4062-b2bb-43a4ebfafc62"

// --- 定数 ---
constexpr int MY_NOTE_MIN = 48;
constexpr int MY_NOTE_MAX = 72;

// --- HAT方向インデックス列挙 ---
enum HatLogicalDir { HAT_UP=0, HAT_DOWN, HAT_LEFT, HAT_RIGHT, HAT_HATCOUNT };

// --- グローバル ---
HardwareSerial uartSerial(1);
NSGamepad Gamepad;
const NimBLEAdvertisedDevice* advMIDI = nullptr;

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

// Peripheral探索
class MyScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
            advMIDI = advertisedDevice;
            NimBLEDevice::getScan()->stop();
        }
    }
};

void connectToPeripheral() {
    NimBLEClient* pClient = NimBLEDevice::createClient();

    class MyClientCallbacks : public NimBLEClientCallbacks {
        void onConnect(NimBLEClient* client) override {
            AtomS3.dis.drawpix(0x00FF00); // 緑
            AtomS3.update();
        }
        void onDisconnect(NimBLEClient* client, int reason) override {
            AtomS3.dis.drawpix(0xFF0000); // 赤
            AtomS3.update();
            NimBLEDevice::getScan()->start(0);
        }
    };
    pClient->setClientCallbacks(new MyClientCallbacks());

    if (!pClient->connect(advMIDI)) {
        AtomS3.dis.drawpix(0xFF0000);
        return;
    }

    NimBLERemoteService* pService = pClient->getService(SERVICE_UUID);
    if (pService) {
        NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(CHARACTERISTIC_UUID);
        if (pChar && pChar->canNotify()) {
            pChar->subscribe(true, [](NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
                if (len == 4 && data[0] == 0xF0 && data[3] == 0xF7) {
                    uint8_t note = data[1];
                    uint8_t on   = data[2];
                    onMidiNoteReceived(note, on);
                }
            });
            AtomS3.dis.drawpix(0x0000FF); // 青
            AtomS3.update();
        }
    }
}

// --- setup ---
void setup() {
    AtomS3.begin(true);
    AtomS3.dis.setBrightness(100);

    Gamepad.begin();
    USB.begin();

    NimBLEDevice::init("");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new MyScanCallbacks());
    pScan->setActiveScan(true);
    pScan->start(0);

    while (advMIDI == nullptr) delay(100);

    connectToPeripheral();
}

// --- 物理ボタンA：長押しでボタン配置切り替え、短押しでマッピング切り替え ---
unsigned long btnPressStart = 0;
bool btnWasPressed = false;

// --- loop ---
void loop() {
    AtomS3.update();

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
