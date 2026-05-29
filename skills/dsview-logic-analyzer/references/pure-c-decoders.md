# Pure C Protocol Decoders

Embedded in `dslogic-cli.c` — no Python, no libsigrokdecode, 0 extra deps.

## Architecture

```
dslogic-cli decode <proto> <file> --samplerate <hz> [opts]
                    │
    ┌───────────────┼───────────────┐
    ▼               ▼               ▼
decode_uart()  decode_spi()   decode_i2c()
```

Each decoder takes raw byte buffer + protocol-specific options, outputs JSON.

## UART Decoder (`decode_uart`)

Algorithm:
1. Scan for falling edge (idle-high → low = potential start bit)
2. Verify start bit at center (bit_width/2 after edge) — must be 0
3. Sample data bits at `start_center + full*(i+1)`, LSB first
4. Check parity: count 1's in val + parity_bit, compare to odd/even
5. Verify stop bit(s) — must be 1
6. Output JSON: type=data|error, value, optional frame_error/parity_error

Bit width: `samplerate / baudrate` (floating point, rounded to uint64).

## SPI Decoder (`decode_spi`)

Algorithm:
1. Sample clock line every sample tick, track last_clk state
2. Detect edge based on CPOL/CPHA combination:
   - CPOL=0,CPHA=0: sample on rising edge (0→1)
   - CPOL=0,CPHA=1: sample on falling edge (1→0)  
   - CPOL=1,CPHA=0: sample on falling edge (1→0)
   - CPOL=1,CPHA=1: sample on rising edge (0→1)
3. On edge, sample MISO/MOSI at consecutive samples (one per bit)
4. Assemble word: MSB-first or LSB-first
5. Skip wordsize samples ahead (avoids re-triggering on same word)

CS pin accepted as parameter but not used in decode logic (TODO).

## I2C Decoder (`decode_i2c`)

Algorithm:
1. Track last_scl, last_sda states
2. Detect START: SDA↓ while SCL=H
3. Detect STOP:  SDA↑ while SCL=H
4. Output JSON: type=start|stop

Data byte extraction and ACK detection are TBD.
Current implementation only detects bus conditions.

## Data Access

```c
static inline int sample_bit(const uint8_t *buf, uint64_t idx) {
    return (buf[idx / 8] >> (idx % 8)) & 1;
}
```

Physical channel N = bit position N in byte array.
`sample_bit(raw, sample_num * 1 + channel)` gives sample_num's bit for given channel.

## Capture File Format

Raw binary from `capture`:
- ≤8 channels: 1 byte/sample, D0=bit0, D7=bit7
- Decode assumes 1 byte/sample (channels ≤ 8)
- `samples = file_bytes / 1`
