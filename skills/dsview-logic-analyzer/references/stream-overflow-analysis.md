# DSLogic Stream Capture — Root Cause Analysis

**Date**: 2026-05-15. Pi 5 ARM64, DSLogic Plus via USB 2.0 (480 Mbps).
**Conclusion**: FPGA USB ring buffer is NOT the limiting factor. The 18M sample cap was a CLI bug.

## Key Driver Logic (dsl.c:2414-2418)

```c
// In stream mode (sdi->mode == LOGIC):
if (devc->limit_samples && !devc->is_loop &&
    devc->num_bytes >= devc->actual_bytes) {
    devc->status = DSL_STOP;  // stops when enough bytes received
}
```

Where `actual_bytes` is derived from `limit_samples`:
```c
actual_samples = (limit_samples + SAMPLES_ALIGN) & ~SAMPLES_ALIGN;
actual_bytes   = actual_samples / DSLOGIC_ATOMIC_SAMPLES * num_channels * DSLOGIC_ATOMIC_SIZE;
```

**DSView does nothing special** — it simply sets `limit_samples = sample_duration × sample_rate` via `SR_CONF_LIMIT_SAMPLES` (see `DSView/DSView/pv/toolbars/samplingbar.cpp:806-813`). For 1s @ 100MHz: limit_samples = 100,000,000.

## The Real Bug (fixed in v3.1)

dslogic-cli v3.0 had this in `cmd_stream()`:
```c
if (on)
    ds_set_actived_device_config(NULL, NULL, SR_CONF_LIMIT_SAMPLES,
        g_variant_new_uint64(10000000ULL));  // FORCED 10M!
```

This hardcoded 10M was intended as a "safe default" but actually prevented any capture longer than ~100ms.

**Fix**: Remove the forced limit. Let the user control `limit_samples` explicitly, same as DSView.

## What Was Misdiagnosed

The "FPGA USB ring buffer ~3.75 MB → max ~18M samples" theory was WRONG.
- In stream mode, data flows continuously over USB — FPGA DDR is not the bottleneck.
- The observed ~18M cap was an artifact of setting limit_samples too low.
- The "overflow" at ≥20M in earlier tests was likely caused by the CLI-side `g_limit` byte-limit triggering an early `ds_stop_collect()` which corrupted the driver state.

## Verified Correct Behavior

| limit_samples | Duration @100MHz | Expected |
|---------------|------------------|----------|
| 10M (old bug) | ~100ms | ❌ Bug — CLI forced this |
| 100M | ~1s | ✅ Same as DSView |
| Any | N × samplerate | ✅ Driver handles it |

## Related Source Locations

- `DSView/DSView/pv/toolbars/samplingbar.cpp:806` — DSView calculates sample_count from duration
- `dsl.c:2414-2418` — Driver stop condition for stream mode
- `dslogic.c:1436` — actual_samples calculation from limit_samples
- `dsl.c:2399-2405` — data forwarding with remain_length clamping
- `/opt/dslogic/src/dslogic-cli.c:234-236` — OLD: forced 10M (removed)
