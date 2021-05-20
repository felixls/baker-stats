# Tezos nano dashboard

This device shows you how a particular baker is doing during one cycle (4096 blocks) represented as a 16x16 grid, each cube is 16 blocks where a baker can have rights to endorse or bake.

It also connect the your baker and check if is on sync with Tezos blockchain.

### Hardware

- Microcontroller ESP32-WROOM-32D
- 2.7inch BW e-Paper display
- Generic battery bank (e.g: 5V 5000mAh). Notice the battery bank should not cut power on low current.

### Energy usage

It uses 40mA @ 3.3v (0.132W) while active (once every minute), while sleeping uses 4mA (0.0132W).
The display doesn't need power to retain the image.

### Configuration

create a `src/local.h` file containing the following information:

```
#ifndef LOCAL_H
#define LOCAL_H

#define BAKER_HOST "your-backer-host-or-ip"
#define BAKER_PORT your-baker-rpc-port
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"
#define BAKER_ADDRESS "your-tz-baker-address"

#endif
```

### Compilation

### Requirements

Get PlatformIO from https://platformio.org/

`$ pio run`

### Flash

`$ pio run -t upload`

### Serial monitor

`$ pio device monitor`
