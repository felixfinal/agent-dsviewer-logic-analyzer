# Changelog

All notable changes to dslogic-cli.

## [3.2.0] — 2026-05-15

### Added
- Auto channel disable: `capture` now disables unused channels before querying HW_DEPTH,
  preventing USB overflow from `dsl_adjust_probes(16)` enabling all channels.
- `SR_CONF_VLD_CH_NUM` query to get mode's effective channel count.
- `DSL_FW_DIR` environment variable for configurable firmware path.
- Stderr info line: `{"info":{"samplerate":...,"requested_dur":...,"actual_dur":...,"samples":...,"max_dur":...}}`
- Verified 11 test profiles with DSLogic Plus.

### Fixed
- HW_DEPTH returned wrong value when all 16 physical channels were enabled
  (returned hw_depth/16 instead of hw_depth/mode_channels).
- USB overflow at high sample rates due to extra channels enabled.
- Capture auto-duration now correctly clamps to hardware maximum.

## [3.0.0] — 2026-05-14

### Added
- Complete rewrite: single-file C99 (~560 lines), JSON stdout.
- `scan`, `open`, `close`, `info`, `modes`, `channels` commands.
- `enable`, `vth`, `stream`, `channel_mode`, `samplerate` configuration.
- `trigger` with full stage support (edge/level, multi-stage, pre-trigger).
- `capture` with auto-max duration calculation.
- `run` mode for multi-step batch workflows via stdin.
- `--version`, `--help` meta commands.
- Direct link against `libsigrok4DSL.so` — same library as DSView GUI.
