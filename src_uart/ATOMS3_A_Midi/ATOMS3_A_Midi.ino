// =============================
// ATOMS3-A: MIDI to UART Sender
// =============================

#include <M5AtomS3.h>
#include "USB_Conexion.h"

#define UART_TX 1
#define UART_RX 2
HardwareSerial uartSerial(1);

// --- グローバル定数定義 ---
constexpr int MY_NOTE_MIN = 48;
constexpr int MY_NOTE_MAX = 72;
constexpr int NOTE_RANGE  = MY_NOTE_MAX - MY_NOTE_MIN;

// --- HSV→RGB変換（0-255出力）---
void hsv2rgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    float r1 = 0, g1 = 0, b1 = 0;
    if      (h <  60) { r1 = c; g1 = x; }
    else if (h < 120) { r1 = x; g1 = c; }
    else if (h < 180) { g1 = c; b1 = x; }
    else if (h < 240) { g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; b1 = c; }
    else              { r1 = c; b1 = x; }
    r = static_cast<uint8_t>((r1 + m) * 255);
    g = static_cast<uint8_t>((g1 + m) * 255);
    b = static_cast<uint8_t>((b1 + m) * 255);
}

// --- MIDI受信時の色・LED処理 ---
void showNoteColor(int note, int velocity) {
    int n = constrain(note - MY_NOTE_MIN, 0, NOTE_RANGE);
    float h = (240.0f * n) / NOTE_RANGE;
    uint8_t r, g, b;
    hsv2rgb(h, 1.0f, 0.8f, r, g, b); // 彩度100%、明度80%
    uint32_t color = (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(r) << 8) | b; // GRB
    uint8_t val = map(velocity, 1, 127, 20, 100); // Velocityで輝度
    AtomS3.dis.setBrightness(val);
    AtomS3.dis.drawpix(color);
    AtomS3.update();
}

// --- MIDI OFF時のLED消灯 ---
void clearNoteColor() {
    AtomS3.dis.drawpix(0x000000);
    AtomS3.update();
}

// --- USB_Conexionを継承し、MIDIイベント処理を行うクラス ---
class MyUSBConexion : public USB_Conexion {
public:
    void onMidiDataReceived(const uint8_t* data, size_t length) override {
        // USB-MIDIの仕様で4バイト/3バイトパケット両方に対応
        const uint8_t* midiData = (length == 3) ? data : (length >= 4 ? data + 1 : nullptr);
        if (!midiData) return;

        uint8_t midiStatus = midiData[0] & 0xF0;
        uint8_t note = midiData[1];
        uint8_t velocity = midiData[2];

        if (midiStatus == 0x90 && velocity > 0) {
            showNoteColor(note, velocity);
            onMidiNoteReceived(note, true);
        } else if (midiStatus == 0x80 || (midiStatus == 0x90 && velocity == 0)) {
            clearNoteColor();
            onMidiNoteReceived(note, false);
        }
    }
};
MyUSBConexion myUSB;

// --- UART送信 ---
void onMidiNoteReceived(uint8_t note, bool on) {
    if(note < MY_NOTE_MIN || note > MY_NOTE_MAX) return;
    uartSerial.write(0xF0);
    uartSerial.write(note);
    uartSerial.write(on);
    uartSerial.write(0xF7);
}

// --- セットアップ ---
void setup() {
    AtomS3.begin(true);
    AtomS3.dis.setBrightness(100);
    AtomS3.dis.drawpix(0x000000);

    uartSerial.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

    // USBホスト初期化
    myUSB.begin();
}

void loop() {
    AtomS3.update();
    myUSB.task();
}
