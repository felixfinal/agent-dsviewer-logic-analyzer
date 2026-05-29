# DSL Driver Internals — Stream Mode Architecture

Key findings from libsigrok4DSL/hardware/DSL/ source analysis (2026-05-15).

## Stream Mode Stop Condition

**dsl.c:2414-2418** — the ONLY stop condition for LOGIC stream mode:

```c
if (sdi->mode == LOGIC &&
    devc->limit_samples &&
    !devc->is_loop &&
    devc->num_bytes >= devc->actual_bytes) {
    devc->status = DSL_STOP;
}
```

- `actual_samples = (limit_samples + SAMPLES_ALIGN) & ~SAMPLES_ALIGN`  (dslogic.c:1436)
- `actual_bytes = actual_samples / DSLOGIC_ATOMIC_SAMPLES * dsl_en_ch_num(sdi) * DSLOGIC_ATOMIC_SIZE`  (dslogic.c:1437)
- If `is_loop=1` (set via `SR_CONF_LOOP_MODE`), this condition is skipped → indefinite streaming

## DSView's sw_depth Convention

**samplingbar.h:69-70**:
```cpp
static const uint64_t LogicMaxSWDepth64 = SR_GB(16);  // 16 GB for ≤16ch
static const uint64_t LogicMaxSWDepth32 = SR_GB(8);   //  8 GB for 32ch
```

Used to populate the duration dropdown:
```
max_duration = sw_depth / samplerate
```

For 100MHz: `16GB / 100MHz = 160 seconds`

## USB Buffer Sizing (dsl.c:2103-2158)

### Single buffer (get_buffer_size)
For stream mode:
```
buffer_size = get_single_buffer_time(devc) * to_bytes_per_ms(devc)
            = 20ms * byte_rate_ms
            → aligned to 512B (USB 2.0) or 1KB (USB 3.0)
```

### Data rate (to_bytes_per_ms)
For LOGIC mode:
```
byte_rate_ms = ceil(samplerate / 1000.0 * num_channels / 8)
```
100MHz × 3ch = `ceil(100000 * 3 / 8)` = 37,500 bytes/ms = 37.5 MB/s

### Total USB ring (get_number_of_transfers)
```
num_transfers = ceil(100ms * byte_rate_ms / buffer_size)
              = ceil(3,750,000 / ~750,080)  = 5 transfers
```

Total ring buffer: `5 × ~750KB = ~3.75 MB` — this is what was misdiagnosed as the "18M sample limit."

## Channel Mode Table Fragment (dsl.h:369-441)

Stream modes for DSLogic Plus (0x0020):
| ID | Name | Max ch | Max rate | Desc |
|----|------|--------|----------|------|
| 0 | DSL_STREAM20x16 | 16 | 20 MHz | 16 ch |
| 1 | DSL_STREAM25x12 | 12 | 25 MHz | 12 ch |
| 2 | DSL_STREAM50x6  |  6 | 50 MHz |  6 ch |
| 3 | DSL_STREAM100x3 |  3 | 100 MHz |  3 ch |

Buffer modes:
| 20 | DSL_BUFFER100x16 | 16 | 100 MHz |
| 21 | DSL_BUFFER200x8  |  8 | 200 MHz |
| 22 | DSL_BUFFER400x4  |  4 | 400 MHz |

## FPGA Capture Counter

Sent to FPGA via `dsl_fpga_arm()` (dsl.c:1092-1096):
```c
tmp_u64 = devc->actual_samples;
tmp_u64 >>= 4;  // hardware minimum unit = 64
setting.cnt_l = tmp_u64 & 0x0000ffff;
setting.cnt_h = tmp_u64 >> 16;
```

Max counter: `0xFFFFFFFF × 64 = 274,877,906,880 samples` — not a practical limit.

## DSView capture flow (samplingbar.cpp:788-818)

```
sample_duration = user_selected_duration_from_UI
sample_rate     = current samplerate

sample_count = ceil(sample_duration * sample_rate), aligned
set_config(SR_CONF_LIMIT_SAMPLES, sample_count)

// For non-DSO mode, also check RLE
if (rle_mode) set_config(SR_CONF_RLE, true)
```

No special tricks — just a large `limit_samples`.

## Hardware Limits (DSLogic Plus, pid 0x0020)

| Resource | Value |
|----------|-------|
| FPGA DDR | 256 MB (`SR_MB(256)`) |
| hw_depth (per effective ch) | `256MB / vld_num` (mode 3: 85M samples) |
| USB speed | LIBUSB_SPEED_HIGH (2.0, ~38 MB/s practical) |
| Max USB data rate | ~37.5 MB/s for stream modes (by design) |
| sw_depth (DSView UI cap) | 16 GB (for ≤16ch) — academic, not physical |

### Critical Pitfall: Channel Enable State

`dsl_adjust_probes(sdi, 16)` (dsl.c:178) sets `probe->enabled = TRUE` for ALL 16
physical channels. This causes:

1. **`dsl_en_ch_num()` returns 16** — counts all enabled channels
2. **`SR_CONF_HW_DEPTH` returns wrong value**: `256MB/16 = 16M` instead of `256MB/3 = 85M`
3. **USB overflow at high samplerates**: the driver computes throughput from enabled count:
   `to_bytes_per_ms = ceil(samplerate/1000 * 16 / 8)` → 200MB/s @ 100MHz, exceeding USB 2.0

**Fix**: Disable channels beyond the mode's `vld_num` before capture. The CLI now
auto-disables them in `cmd_capture()` using `SR_CONF_VLD_CH_NUM` to determine how
many to keep.

### Correct max_samples Calculation

```c
// 1. Query effective channels for current mode
SR_CONF_VLD_CH_NUM → 3 (for mode 3)

// 2. Disable channels ≥ 3
for (ch >= mode_ch) ds_enable_device_channel(ch, FALSE);

// 3. Now HW_DEPTH is correct
SR_CONF_HW_DEPTH → 256MB / 3 = 85M samples

// 4. Compute max duration
max_dur = 85M / samplerate  → 0.85s @ 100MHz
```
