#include "main.h"
#include "blocktest.h"
#include "apitez.h"
#include "tzstats.h"

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeSans9pt7b.h>

// Display 264x176
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

// default SPI
// SS    = 5;
// MOSI  = 23;
// MISO  = 19;
// SCK   = 18;

// mapping of Waveshare ESP32 Driver Board
// BUSY -> 25, RST -> 26, DC -> 27, CS-> 15, CLK -> 13, DIN -> 14

GxEPD2_BW<GxEPD2_270, MAX_HEIGHT(GxEPD2_270)> display(GxEPD2_270(/*CS=5*/ SS, /*DC=*/27, /*RST=*/26, /*BUSY=*/25)); // GDEW027W3

bool connected = false;

// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            Sprintln("");
            Sprintln(F("WiFi connected"));
            Sprintln(F("IP address: "));
            Sprintln(WiFi.localIP());
            connected = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Sprintln(F("WiFi lost connection"));
            connected = false;
            break;
        default:
            break;
    }
}

void connectWiFi() {
    Sprintln("Connecting to WiFi network: " + String(WIFI_SSID));

    WiFi.disconnect(true);
    delay(1000);
    WiFi.onEvent(WiFiEvent);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void setup() {
    delay(500);

    sleepEnable();

    Sbegin(115200);

    display.init(115200, true, 2, false);
    display.setRotation(1);
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();

    connectWiFi();

    Sprint(F("Waiting for WiFi to connect..."));
    while (!connected) {
        delay(500);
        Sprint(".");
    }
}

long nodeBlock;
Tzstats tzStats;

int isNodeOK() {
    Apitez apiTez;
    Blocktest blockTest;

    long network1 = tzStats.block;
    long network2 = apiTez.get();
    long currentBlock = (network1 > network2) ? network1 : network2;
    nodeBlock = blockTest.get();
    return (currentBlock - nodeBlock) < 2 ? 0 : 1;
}

struct BlockState {
    bool used;
    bool lost;
    bool stolen;
    bool missed;
    bool bondMiss;
    bool found;
};

void getBlockState(long start, long end, BlockState *state) {
    for (int i = 0; i < tzStats.numRights; i++) {
        if (tzStats.rights[i].block >= start && tzStats.rights[i].block <= end) {
            if ((tzStats.rights[i].baking && tzStats.rights[i].priority == 0) || !tzStats.rights[i].baking) {
                state->found = true;
                state->used = tzStats.rights[i].used;
                state->missed = tzStats.rights[i].missed;
                state->lost = tzStats.rights[i].lost;
                state->stolen = tzStats.rights[i].stolen;
                state->bondMiss = tzStats.rights[i].bondMiss;
                return;
            }
        }
    }
}

void printBlocks() {
    bool ok = tzStats.getRights();
    if (ok) {
        Sprintln(F("printing blocks"));
        for (uint16_t c = 0; c < 16; c++) {
            for (uint16_t r = 0; r < 16; r++) {
                long blockStart = tzStats.start + (long(r) + long(c) * 16) * 16;
                long blockEnd = blockStart + 15;
                BlockState blockState;
                blockState.found = false;
                getBlockState(blockStart, blockEnd, &blockState);

                display.drawRect(c * 10, r * 10, 10, 10, GxEPD_BLACK);
                if (blockState.found) {
                    Sprintln(blockStart);
                    Sprintln(blockEnd);
                    Sprintln(blockState.used);
                    Sprintln(blockState.missed);

                    // "Gray" rectangle
                    display.drawLine(c * 10 + 3, r * 10, c * 10 + 10, r * 10 + 7, GxEPD_BLACK);
                    display.drawLine(c * 10, r * 10, c * 10 + 10, r * 10 + 10, GxEPD_BLACK);
                    display.drawLine(c * 10, r * 10 + 3, c * 10 + 7, r * 10 + 10, GxEPD_BLACK);

                    if (blockState.used) {
                        // full rectangle
                        display.fillRect(c * 10, r * 10, 10, 10, GxEPD_BLACK);
                    }
                }
                if (tzStats.block >= blockStart && tzStats.block <= blockEnd) {
                    // "dot"
                    display.fillRect(c * 10 + 2, r * 10 + 2, 6, 6, GxEPD_BLACK);
                }
            }
        }
    }
}

void printStats() {
    uint16_t rowHeight = 16;
    uint16_t row = rowHeight - 2;
    uint16_t rowSep = 20;
    uint16_t startCol = 162;

    int res = isNodeOK();
    switch (res) {
        case 0:
        case 1:
            display.setCursor(startCol, row);
            display.print(F("Cycle"));
            row += rowHeight;
            display.setCursor(startCol, row);
            display.print(tzStats.cycle);

            row += rowSep;
            display.setCursor(startCol, row);
            display.print(F("Block:"));
            row += rowHeight;
            display.setCursor(startCol, row);
            display.print(tzStats.block);

            row += rowSep;
            display.setCursor(startCol, row);
            display.print(F("Node:"));
            row += rowHeight;
            display.setCursor(startCol, row);
            if (res == 0) {
                display.print(F("Up to date"));
            } else {
                display.print(F("Node is behind"));
            }

            row += rowSep;
            display.setCursor(startCol, row);
            display.print(F("Time:"));
            row += rowHeight;
            display.setCursor(startCol, row);
            display.print(tzStats.date);
            row += rowHeight;
            display.setCursor(startCol, row);
            display.print(tzStats.time);
            break;
        default:
            display.setCursor(startCol, row);
            display.print(F("Comm failure"));
            break;
    }
}

void loop() {
    if (connected) {
        display.firstPage();

        bool ok = tzStats.getHead();
        if (!ok) {
            delay(20000);
        } else {
            ok = tzStats.getCycleHead();
            if (!ok) {
                delay(20000);
            } else {
                printBlocks();

                printStats();

                display.setCursor(0, 174);
                display.print(F("Tezos nano dashboard - Felixls"));
                display.nextPage();
                display.powerOff();
                sleepStart();
            }
        }
    }

    delay(10000);
}
