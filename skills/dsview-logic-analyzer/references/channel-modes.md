# DSLogic Channel Modes

From `dsl.h` — `channel_modes[]` array. Struct fields (in order): `id`, `mode`, `type`, `stream`, `num` (total ch), `vld_num` (max ch at full rate), `unit_bits`, `min_samplerate`, `max_samplerate`, `hw_min_samplerate`, `hw_max_samplerate`, `pre_div`, `descr`.

## DSLogic Plus (pid=0x0020) Profiles

DSLogic Plus has TWO profiles in `dsl.h`:

**Profile 1 (line 508):** `0x0020`, `LIBUSB_SPEED_HIGH` (USB 2.0), `DSLogicPlus.bin` FPGA bitstream.
Channels mask = `DSL_STREAM20x16 | DSL_STREAM25x12 | DSL_STREAM50x6 | DSL_STREAM100x3 | DSL_BUFFER100x16 | DSL_BUFFER200x8 | DSL_BUFFER400x4`.
Hardware depth = 256 MB. Default channel mode = `DSL_BUFFER100x16`.
Max samplerates: 200 MHz (stream), 400 MHz (buffer).

**Profile 2 (line 722):** `0x0020`, `LIBUSB_SPEED_SUPER` (USB 3.0), `DSLogicPlus-pgl12.bin` FPGA bitstream.
Channels mask includes ALL modes (32-channel stream/buffer, up to 1 GHz).
Hardware depth = 1 GB.

Our DSLogic Plus (firmware 2.2) uses Profile 1 (USB 2.0, limited to 16-channel modes).

## Stream Modes (stream=TRUE)

### Basic (pre_div=1, unit_bits=1) — USB 2.0

| ID | Channels | Max Rate | Description |
|----|----------|----------|-------------|
| DSL_STREAM20x16 | 16/16 | 20 MHz | Use 16 Channels (Max 20MHz) |
| DSL_STREAM25x12 | 16/12 | 25 MHz | Use 12 Channels (Max 25MHz) |
| DSL_STREAM50x6 | 16/6 | 50 MHz | Use 6 Channels (Max 50MHz) |
| DSL_STREAM100x3 | 16/3 | 100 MHz | Use 3 Channels (Max 100MHz) |

### 3DN2 (pre_div=5, unit_bits=5) — DSLogic Plus FPGA

| ID | Total Ch | Max Ch | Max Rate | Description |
|----|----------|--------|----------|-------------|
| DSL_STREAM20x16_3DN2 | 16 | 16 | 20 MHz | Use 16 Channels (Max 20MHz) |
| DSL_STREAM25x12_3DN2 | 16 | 12 | 25 MHz | Use 12 Channels (Max 25MHz) |
| DSL_STREAM50x6_3DN2 | 16 | 6 | 50 MHz | Use 6 Channels (Max 50MHz) |
| DSL_STREAM100x3_3DN2 | 16 | 3 | 100 MHz | Use 3 Channels (Max 100MHz) |
| DSL_STREAM10x32_32_3DN2 | 32 | 32 | 10 MHz | Use 32 Channels (Max 10MHz) |
| DSL_STREAM20x16_32_3DN2 | 32 | 16 | 20 MHz | Use 16 Channels (Max 20MHz) |
| DSL_STREAM25x12_32_3DN2 | 32 | 12 | 25 MHz | Use 12 Channels (Max 25MHz) |
| DSL_STREAM50x6_32_3DN2 | 32 | 6 | 50 MHz | Use 6 Channels (Max 50MHz) |
| DSL_STREAM100x3_32_3DN2 | 32 | 3 | 100 MHz | Use 3 Channels (Max 100MHz) |

### High-speed (pre_div=5, USB 3.0 profile only)

| ID | Total Ch | Max Ch | Max Rate | Description |
|----|----------|--------|----------|-------------|
| DSL_STREAM50x32 | 32 | 32 | 50 MHz | Use 32 Channels (Max 50MHz) |
| DSL_STREAM100x30 | 32 | 30 | 100 MHz | Use 30 Channels (Max 100MHz) |
| DSL_STREAM250x12 | 32 | 12 | 250 MHz | Use 12 Channels (Max 250MHz) |
| DSL_STREAM125x16_16 | 16 | 16 | 125 MHz | Use 16 Channels (Max 125MHz) |
| DSL_STREAM250x12_16 | 16 | 12 | 250 MHz | Use 12 Channels (Max 250MHz) |
| DSL_STREAM500x6 | 16 | 6 | 500 MHz | Use 6 Channels (Max 500MHz) |
| DSL_STREAM1000x3 | 8 | 3 | 1 GHz | Use 3 Channels (Max 1GHz) |

## Buffer Modes (stream=FALSE, unit_bits=1)

| ID | Channels | Max Rate | Description |
|----|----------|----------|-------------|
| DSL_BUFFER100x16 | 16/16 | 100 MHz | Use Channels 0~15 (Max 100MHz) |
| DSL_BUFFER200x8 | 8/8 | 200 MHz | Use Channels 0~7 (Max 200MHz) |
| DSL_BUFFER400x4 | 4/4 | 400 MHz | Use Channels 0~3 (Max 400MHz) |
| DSL_BUFFER250x32 | 32/32 | 250 MHz | Use Channels 0~31 (Max 250MHz) |

## USB Bandwidth Math

### Driver buffer allocation (`dsl.c`)

```
to_bytes_per_ms  = ceil(samplerate/1000 × channels / 8)    // bits→bytes per ms
single_buf_size  = get_single_buffer_time × to_bytes_per_ms
                  // USB 3.0: 10ms; USB 2.0: 20ms
                  // aligned to 1K (USB 3.0) or 512B (USB 2.0)
num_transfers    = ceil(get_total_buffer_time × to_bytes_per_ms / single_buf_size)
                  // USB 3.0: 40ms; USB 2.0: 100ms
                  // capped at NUM_SIMUL_TRANSFERS (64)
total_alloc      = num_transfers × single_buf_size
```

### 100MHz stream, 3 channels (USB 2.0)

| Parameter | Value |
|-----------|-------|
| `to_bytes_per_ms` | 37,500 bytes/ms (37.5 MB/s) |
| `single_buf_size` | 750,080 bytes (733 KB, 20ms × 37500, aligned to 512) |
| `num_transfers` | 5 (ceil(100 × 37500 / 750080) = 5) |
| **Total DMA allocation** | **3,750,400 bytes (3.58 MB)** |
| USB 2.0 bandwidth | 480 Mbps = 60 MB/s → 62.5% utilization ✅ |
| Raspberry Pi result | ❌ LIBUSB_ERROR_NO_MEM (DWC2/VL805 DMA descriptor limit) |
| PC (xHCI/EHCI) result | ✅ works with DSView |

### Why Pi fails

Raspberry Pi's USB host controller (DWC2 on older models, VL805 on Pi 4) has limited DMA scatter-gather descriptor space. Submitting 5 bulk transfers of 733 KB each simultaneously requires 3.58 MB of contiguous DMA buffers — exceeds Pi controller capability. PC xHCI/EHCI controllers have much larger descriptor pools.

Reduce `get_single_buffer_time` (20→10ms) and `get_total_buffer_time` (100→50ms) in `dsl.c` to cut total to ~1.8 MB (untested Pi workaround).

## Profile Field Reference

`DSL_profile` struct fields (in order, from `dsl.h`):
`vid`, `pid`, `usb_speed`, `vendor`, `product`, `firmware`, `fw`, `fpga_bit33`, `fpga_bit50`, `dev_caps`

`dev_caps` struct:
`mode_caps`, `feature_caps`, `channels` (bitmask of supported `channel_modes[]` indices), `channel_groups`, `hw_depth`, `dso_depth`, `default_channelmode`, `default_samplelimit`, `firmware_versions[]`, `vga_id`, `default_vga`, `default_samplerate`, `default_samplelimit2`, `vdivs[]`, `ref_min`, `ref_max`, `default_pwmtrans`, `default_pwmmargin`, `default_comb_comp`, `max_comb_comp`, `min_stream_samplerate`, `max_stream_samplerate`, `min_buffer_samplerate`, `max_buffer_samplerate`

Key flags:
- `CAPS_MODE_LOGIC` (0x01), `CAPS_MODE_DSO` (0x02), `CAPS_MODE_ANALOG` (0x04)
- `CAPS_FEATURE_VTH` (0x01) — variable threshold
- `CAPS_FEATURE_BUF` (0x02) — buffer mode supported
- `CAPS_FEATURE_USB30` (0x08) — USB 3.0
- `CAPS_FEATURE_SEEP` (0x80) — serial EEPROM config
- `CAPS_FEATURE_FLASH` (0x200) — SPI flash config
