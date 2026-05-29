# DSView .dsl Session File Format

.dsl files saved by DSView GUI are zip archives containing per-channel raw capture data.

## Archive Contents

| File | Description |
|------|-------------|
| `header` | Binary header (~250 bytes) |
| `session` | JSON metadata (sample rate, count, mode, channels) |
| `decoders` | Decoder configuration (4 bytes) |
| `L-N/M` | Per-channel raw data (N=group, M=channel index) |

For a single-group 4-channel capture: `L-0/0`, `L-0/1`, `L-0/2`, `L-0/3`.
For multi-group: `L-0/0` through `L-0/3` (group 0) and `L-1/0` through `L-1/3` (group 1).

## Session JSON

```json
{
    "Sample count": "10000384",
    "Sample rate": "1000000",
    "Operation Mode": 1,
    "Channel Mode": 2,
    "channel": [
        {"enabled": true, "index": 0, "name": "0"},
        {"enabled": true, "index": 1, "name": "1"},
        ...
    ]
}
```

- `Operation Mode`: 0=BUFFER, 1=STREAM
- `Sample count`: total samples per channel (as string)
- `Sample rate`: Hz (as string)

## Per-Channel Raw Data

Each `L-N/M` file contains packed bits — **8 samples per byte, LSB = earliest sample**.

```python
def read_channel(path):
    with open(path, 'rb') as f:
        data = f.read()
    bits = []
    for byte in data:
        for bit in range(8):
            bits.append((byte >> bit) & 1)
    return bits

d0 = read_channel('L-0/0')  # bit 0 = D0 at each sample
d1 = read_channel('L-0/1')  # bit 0 = D1 at each sample
# ...
```

To build a multi-bit value at sample i:
```python
nibble = (d3[i] << 3) | (d2[i] << 2) | (d1[i] << 1) | d0[i]
```

## Finding Transitions

```python
prev = (d3[0]<<3)|(d2[0]<<2)|(d1[0]<<1)|d0[0]
for i in range(1, n):
    val = (d3[i]<<3)|(d2[i]<<2)|(d1[i]<<1)|d0[i]
    if val != prev:
        t_ms = i / sample_rate * 1000
        print(f"@ {t_ms:.0f}ms: {prev:04b}({prev}) → {val:04b}({val})")
        prev = val
```

## CLI Raw Binary vs .dsl

| Format | Samples/byte | Multi-channel |
|--------|-------------|---------------|
| CLI `.bin` | 1 sample/byte (≤8ch), 2 samples/byte (9-16ch) | All channels in one file, bit-interleaved |
| .dsl `L-N/M` | 8 samples/byte | One file per channel, bit-packed |

The CLI format is simpler for quick analysis; .dsl files separate channels cleanly.
