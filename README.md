# lux-rx
6-bit optical text receiver (and matching encoder) in C.

[![Test](https://github.com/eiriksm/lux-rx/actions/workflows/test.yml/badge.svg)](https://github.com/eiriksm/lux-rx/actions/workflows/test.yml)

## Description

Decodes text sent as blinking light, sampled by a light sensor at 64 Hz. Each bit is NRZ with 8 samples per bit, and each frame is `[START] [data symbols...] [CRC6] [END]` with a 62 character alphabet. See `src/lux_rx.h` for the details of the protocol.

## Usage

```c
#include "lux_rx.h"

lux_rx_t rx;
lux_rx_init(&rx);

// Each tick at 64 Hz:
if (lux_rx_feed(&rx, adc_val) == LUX_RX_DONE) {
  use(rx.payload); // null-terminated string
  lux_rx_reset(&rx);
}
```

The encoder produces the samples a transmitter should send:

```c
lux_rx_encoder_t enc;
uint8_t tick;
lux_rx_encode(&enc, "hello world");
while (lux_rx_encode_next(&enc, &tick)) {
  transmit(tick);
}
```

## Licence

MIT
