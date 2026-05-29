/*
 * dslogic-cli v3.2.0 — DSLogic Native CLI (libsigrok4DSL)
 *
 * One-shot tool: each invocation is a separate process.
 * Use "run" mode for multi-step workflows (reads commands from stdin).
 *
 * Build:  make
 * Install: make install
 * Clean:  make clean
 *
 * JSON on stdout, diagnostics on stderr.
 *
 * https://github.com/felixfinal/dslogic-cli
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>
#include "libsigrok.h"

#define VERSION "3.2.0"

/* ── global state ──────────────────────────────────────────── */
static int          g_collecting = 0;
static uint64_t     g_bytes      = 0;   /* total raw bytes received */
static uint64_t     g_limit      = 0;
static FILE        *g_out_fp     = NULL;
static int          g_triggered  = 0;
static int          g_closed     = 0;

/* ── callbacks ─────────────────────────────────────────────── */
static void on_event(int ev) {
    if (ev == DS_EV_CURRENT_DEVICE_DETACH)
        fprintf(stderr, "{\"event\":\"device_detached\"}\n");
}

static void on_datafeed(const struct sr_dev_inst *sdi,
                        const struct sr_datafeed_packet *pkt) {
    (void)sdi;
    if (!g_collecting) return;
    switch (pkt->type) {
    case SR_DF_HEADER:
        fprintf(stderr, "{\"event\":\"capture_started\"}\n");
        g_bytes = 0;
        break;
    case SR_DF_TRIGGER:
        g_triggered = 1;
        fprintf(stderr, "{\"event\":\"trigger\",\"bytes\":%lu}\n", g_bytes);
        break;
    case SR_DF_LOGIC: {
        const struct sr_datafeed_logic *logic = pkt->payload;
        if (!logic || !logic->data || logic->length == 0) break;
        if (g_out_fp)
            fwrite(logic->data, 1, logic->length, g_out_fp);
        g_bytes += logic->length;
        break;
    }
    case SR_DF_END:
        g_collecting = 0;
        fprintf(stderr, "{\"event\":\"capture_done\",\"bytes\":%lu}\n", g_bytes);
        break;
    case SR_DF_OVERFLOW:
        fprintf(stderr, "{\"event\":\"overflow\",\"bytes\":%lu}\n", g_bytes);
        break;
    default:
        break;
    }
}

/* ── helpers ───────────────────────────────────────────────── */
static char *jesc(const char *s) {
    static char b[512];
    char *d = b;
    while (*s && (size_t)(d - b) < sizeof(b) - 4) {
        if (*s == '"' || *s == '\\') *d++ = '\\';
        if (*s == '\n') { *d++ = '\\'; *d++ = 'n'; s++; continue; }
        *d++ = *s++;
    }
    *d = 0;
    return b;
}

static void pj(const char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    vfprintf(stdout, fmt, a);
    va_end(a);
    fprintf(stdout, "\n");
    fflush(stdout);
}

/* ── firmware path ─────────────────────────────────────────── */
static const char *firmware_dir(void) {
    const char *env = getenv("DSL_FW_DIR");
    return env ? env : "/opt/dslogic/res";
}

/* ── commands ──────────────────────────────────────────────── */

static int cmd_scan(void) {
    struct ds_device_base_info *list = NULL;
    int count = 0;
    if (ds_get_device_list(&list, &count) != SR_OK) {
        pj("{\"error\":\"scan_failed\"}");
        return 1;
    }
    printf("{\"devices\":[");
    for (int i = 0; i < count; i++) {
        if (i > 0) printf(",");
        printf("{\"index\":%d,\"name\":\"%s\",\"handle\":%llu}",
               i, jesc(list[i].name),
               (unsigned long long)list[i].handle);
    }
    printf("],\"count\":%d}\n", count);
    fflush(stdout);
    return 0;
}

static int cmd_info(void) {
    struct ds_device_full_info info;
    memset(&info, 0, sizeof(info));
    if (ds_get_actived_device_info(&info) != SR_OK) {
        pj("{\"error\":\"no_active_device\"}");
        return 1;
    }
    pj("{\"name\":\"%s\",\"driver\":\"%s\",\"type\":%d}",
       jesc(info.name), jesc(info.driver_name), info.dev_type);
    return 0;
}

static int cmd_open(int index) {
    struct ds_device_base_info *list = NULL;
    int count = 0;
    ds_get_device_list(&list, &count);
    if (index < 0 || index >= count) {
        pj("{\"error\":\"bad_index\",\"index\":%d,\"count\":%d}", index, count);
        return 1;
    }
    if (ds_active_device(list[index].handle) != SR_OK) {
        pj("{\"error\":\"open_failed\"}");
        return 1;
    }
    pj("{\"ok\":true,\"name\":\"%s\"}", jesc(list[index].name));
    return 0;
}

static int cmd_modes(void) {
    const GSList *mlist = ds_get_actived_device_mode_list();
    if (!mlist) {
        pj("{\"error\":\"no_modes\",\"hint\":\"device_not_active\"}");
        return 1;
    }
    int n = g_slist_length((GSList *)mlist);
    printf("{\"modes\":[");
    int i = 0;
    for (const GSList *l = mlist; l; l = l->next, i++) {
        const struct sr_dev_mode *m = l->data;
        if (i > 0) printf(",");
        printf("{\"index\":%d,\"id\":%d,\"name\":\"%s\",\"acronym\":\"%s\"}",
               i, m->mode, m->name,
               m->acronym ? m->acronym : "");
    }
    printf("],\"count\":%d}\n", n);
    fflush(stdout);
    return 0;
}

static int cmd_channels(void) {
    GSList *ch = ds_get_actived_device_channels();
    if (!ch) {
        pj("{\"channels\":[],\"count\":0,\"hint\":\"device_not_active\"}");
        return 0;
    }
    int n = g_slist_length(ch);
    printf("{\"channels\":[");
    int i = 0;
    for (GSList *l = ch; l; l = l->next, i++) {
        struct sr_channel *c = l->data;
        if (i > 0) printf(",");
        printf("{\"index\":%d,\"name\":\"%s\",\"enabled\":%s}",
               c->index, c->name,
               c->enabled ? "true" : "false");
    }
    printf("],\"count\":%d}\n", n);
    fflush(stdout);
    /* NB: ch is driver-owned — do NOT g_slist_free */
    return 0;
}

static struct sr_channel *find_ch(int idx) {
    GSList *ch = ds_get_actived_device_channels();
    for (GSList *l = ch; l; l = l->next)
        if (((struct sr_channel *)l->data)->index == idx)
            return l->data;
    return NULL;
}

static int cmd_enable(int idx, int en) {
    struct sr_channel *c = find_ch(idx);
    if (!c) {
        pj("{\"error\":\"channel_not_found\",\"index\":%d}", idx);
        return 1;
    }
    ds_enable_device_channel(c, en);
    pj("{\"ok\":true,\"channel\":%d,\"enabled\":%d}", idx, en);
    return 0;
}

static int cmd_vth(const char *v) {
    const char *ok[] = {"1.8", "2.5", "3.3", "5.0", NULL};
    int good = 0;
    for (int i = 0; ok[i]; i++)
        if (!strcmp(v, ok[i])) good = 1;
    if (!good) {
        pj("{\"error\":\"bad_vth\",\"supported\":[\"1.8\",\"2.5\",\"3.3\",\"5.0\"]}");
        return 1;
    }
    pj("{\"ok\":true,\"vth\":\"%sV\"}", v);
    return 0;
}

static int cmd_stream(int on) {
    ds_set_actived_device_config(NULL, NULL, SR_CONF_STREAM,
        g_variant_new_boolean((gboolean)on));
    if (on)
        ds_set_actived_device_config(NULL, NULL, SR_CONF_OPERATION_MODE,
            g_variant_new_int16(LO_OP_STREAM));
    else
        ds_set_actived_device_config(NULL, NULL, SR_CONF_OPERATION_MODE,
            g_variant_new_int16(LO_OP_BUFFER));
    pj("{\"ok\":true,\"stream\":%d}", on);
    return 0;
}

static int cmd_samplerate(uint64_t hz) {
    ds_set_actived_device_config(NULL, NULL, SR_CONF_SAMPLERATE,
        g_variant_new_uint64(hz));
    pj("{\"ok\":true,\"samplerate\":%lu}", hz);
    return 0;
}

static int cmd_channel_mode(int id) {
    ds_set_actived_device_config(NULL, NULL, SR_CONF_CHANNEL_MODE,
        g_variant_new_int16((gint16)id));
    pj("{\"ok\":true,\"channel_mode\":%d}", id);
    return 0;
}

static int cmd_limit_samples(uint64_t n) {
    ds_set_actived_device_config(NULL, NULL, SR_CONF_LIMIT_SAMPLES,
        g_variant_new_uint64(n));
    pj("{\"ok\":true,\"limit_samples\":%lu}", n);
    return 0;
}

/* ── trigger ───────────────────────────────────────────────── */
static int cmd_trigger(int argc, char **argv) {
    if (argc < 1) {
        pj("{\"trigger\":{\"enabled\":%u,\"position\":%u}}",
           ds_trigger_get_en(), ds_trigger_get_pos());
        return 0;
    }
    const char *sub = argv[0];

    if (!strcmp(sub, "reset")) {
        ds_trigger_reset();
        pj("{\"ok\":true,\"action\":\"reset\"}");
    } else if (!strcmp(sub, "enable")) {
        int en = (argc >= 2) ? atoi(argv[1]) : 1;
        ds_trigger_set_en((uint16_t)en);
        pj("{\"ok\":true,\"enabled\":%d}", en);
    } else if (!strcmp(sub, "pos")) {
        int pos = (argc >= 2) ? atoi(argv[1]) : 50;
        ds_trigger_set_pos((uint16_t)pos);
        pj("{\"ok\":true,\"position\":%d}", pos);
    } else if (!strcmp(sub, "stage")) {
        if (argc < 4) {
            pj("{\"error\":\"usage: trigger stage <stage> <probes_hex> <t0> [t1] [logic] [inv0] [inv1] [cnt0] [cnt1]\"}");
            return 1;
        }
        uint16_t stage  = (uint16_t)atoi(argv[1]);
        uint16_t probes = (uint16_t)strtoul(argv[2], NULL, 16);
        char *t0 = argv[3];
        char *t1 = (argc >= 5) ? argv[4] : (char *)"XXXXXXXX";

        ds_trigger_stage_set_value(stage, probes, t0, t1);

        unsigned char logic = (argc >= 6) ? (unsigned char)atoi(argv[5]) : 0;
        if (logic) ds_trigger_stage_set_logic(stage, probes, logic);

        unsigned char inv0 = (argc >= 7) ? (unsigned char)atoi(argv[6]) : 0;
        unsigned char inv1 = (argc >= 8) ? (unsigned char)atoi(argv[7]) : 0;
        if (inv0 || inv1) ds_trigger_stage_set_inv(stage, probes, inv0, inv1);

        uint32_t cnt0 = (argc >= 9)  ? (uint32_t)atoi(argv[8])  : 1;
        uint32_t cnt1 = (argc >= 10) ? (uint32_t)atoi(argv[9]) : 1;
        ds_trigger_stage_set_count(stage, probes, cnt0, cnt1);

        ds_trigger_set_stage(stage + 1);
        ds_trigger_set_en(1);

        pj("{\"ok\":true,\"stage\":%u,\"probes\":\"0x%04x\",\"t0\":\"%s\",\"t1\":\"%s\"}",
           stage, probes, t0, t1);
    } else {
        pj("{\"error\":\"unknown_trigger_cmd\",\"cmd\":\"%s\"}", sub);
        return 1;
    }
    return 0;
}

/* ── capture ───────────────────────────────────────────────── */

static int cmd_capture(double duration_sec, const char *outfile, int timeout_sec) {
    GVariant *gvar = NULL;
    uint64_t samplerate = 0;
    uint64_t max_samples = 0;

    /* query current samplerate */
    ds_get_actived_device_config(NULL, NULL, SR_CONF_SAMPLERATE, &gvar);
    if (gvar) { samplerate = g_variant_get_uint64(gvar); g_variant_unref(gvar); }
    if (!samplerate) {
        pj("{\"error\":\"samplerate_not_set\"}");
        return 1;
    }

    /* query effective channel count for current mode */
    gvar = NULL;
    int mode_ch = 0;
    ds_get_actived_device_config(NULL, NULL, SR_CONF_VLD_CH_NUM, &gvar);
    if (gvar) { mode_ch = g_variant_get_int16(gvar); g_variant_unref(gvar); }
    if (mode_ch <= 0) mode_ch = 1;

    /* disable channels beyond mode's effective count so
       HW_DEPTH returns the correct per-channel buffer depth */
    GSList *ch = ds_get_actived_device_channels();
    for (GSList *l = ch; l; l = l->next) {
        struct sr_channel *c = l->data;
        if (c->index >= mode_ch && c->enabled)
            ds_enable_device_channel(c, FALSE);
    }

    /* now HW_DEPTH = hw_depth / mode_ch (correct) */
    gvar = NULL;
    ds_get_actived_device_config(NULL, NULL, SR_CONF_HW_DEPTH, &gvar);
    if (gvar) { max_samples = g_variant_get_uint64(gvar); g_variant_unref(gvar); }
    if (!max_samples)
        max_samples = 256ULL * 1024 * 1024;  /* fallback */

    double max_dur = (double)max_samples / samplerate;
    uint64_t samples_by_dur = (uint64_t)(duration_sec * samplerate);

    /* clamp: use requested duration capped at hw limit;
       0 or negative → use max (device limit) */
    uint64_t samples;
    if (duration_sec <= 0.0)
        samples = max_samples;
    else if (samples_by_dur > max_samples)
        samples = max_samples;
    else
        samples = samples_by_dur;

    /* set driver limit_samples so it auto-stops */
    ds_set_actived_device_config(NULL, NULL, SR_CONF_LIMIT_SAMPLES,
        g_variant_new_uint64(samples));

    double actual_dur = (double)samples / samplerate;
    fprintf(stderr, "{\"info\":{\"samplerate\":%lu,\"requested_dur\":%.3f,\"actual_dur\":%.3f,\"samples\":%lu,\"max_dur\":%.3f}}\n",
            samplerate, duration_sec, actual_dur, samples, max_dur);

    if (outfile && strcmp(outfile, "-")) {
        g_out_fp = fopen(outfile, "wb");
        if (!g_out_fp) {
            pj("{\"error\":\"cannot_open\",\"file\":\"%s\"}", outfile);
            return 1;
        }
    }
    g_collecting = 1;
    g_limit      = 0;   /* driver handles stop via limit_samples */
    g_triggered  = 0;

    if (ds_start_collect() != SR_OK) {
        pj("{\"error\":\"start_failed\"}");
        g_collecting = 0;
        if (g_out_fp) { fclose(g_out_fp); g_out_fp = NULL; }
        return 1;
    }

    /* spin until done or timeout */
    int t = 0;
    while (g_collecting && t < timeout_sec * 10) {
        usleep(100000);
        t++;
    }

    if (g_collecting) {
        ds_stop_collect();
        usleep(500000);  /* wait for cleanup */
    }

    if (g_out_fp) { fclose(g_out_fp); g_out_fp = NULL; }

    pj("{\"ok\":true,\"bytes\":%lu,\"triggered\":%d}",
       g_bytes, g_triggered);
    return 0;
}

static int cmd_close(void) {
    g_closed = 1;
    ds_release_actived_device();
    pj("{\"ok\":true}");
    return 0;
}

/* ── decode (pure C protocol decoders) ─────────────────────── */

/* get bit N from byte array (0=LSB of byte[0]) */
static inline int sample_bit(const uint8_t *buf, uint64_t idx) {
    return (buf[idx / 8] >> (idx % 8)) & 1;
}

static int decode_uart(const uint8_t *raw, uint64_t samples,
                        int ch, int stride, uint64_t samplerate,
                        int baudrate, int data_bits,
                        const char *parity, double stop_bits) {
    double bit_w = (double)samplerate / baudrate;
    uint64_t half  = (uint64_t)(bit_w / 2.0 + 0.5);
    uint64_t full  = (uint64_t)(bit_w + 0.5);
    uint64_t s = 0;
    int pkt = 0, byte_count = 0;

    /* scan for start bit (1→0 transition while hunting, then skip to center) */
    while (s + full * (data_bits + 3) < samples) {
        /* find falling edge (idle high → start bit low) */
        while (s + 1 < samples && sample_bit(raw, s * stride + ch) == 1) s++;
        if (s + full * (data_bits + 3) >= samples) break;

        /* verify start bit: should be 0 at center */
        uint64_t start_center = s + half;
        if (sample_bit(raw, start_center * stride + ch) != 0) { s++; continue; }

        /* sample data bits */
        uint8_t val = 0;
        for (int i = 0; i < data_bits; i++) {
            uint64_t pos = start_center + full * (i + 1);
            int bit = sample_bit(raw, pos * stride + ch);
            val |= (bit << i);  /* LSB first */
        }

        /* parity check */
        uint64_t parity_pos = start_center + full * (data_bits + 1);
        int parity_bit = sample_bit(raw, parity_pos * stride + ch);
        int parity_ok = 1;
        if (!strcmp(parity, "odd")) {
            int ones = __builtin_popcount(val) + parity_bit;
            parity_ok = (ones % 2) == 1;
        } else if (!strcmp(parity, "even")) {
            int ones = __builtin_popcount(val) + parity_bit;
            parity_ok = (ones % 2) == 0;
        }

        /* stop bit check */
        uint64_t stop_pos = parity_pos + full * stop_bits / 2;
        int stop_ok = 1;
        for (int i = 0; i < (int)stop_bits; i++) {
            if (sample_bit(raw, (parity_pos + full * (i + 1)) * stride + ch) != 1)
                stop_ok = 0;
        }

        uint64_t end = stop_pos + full;
        printf("{\"start\":%lu,\"end\":%lu,\"type\":\"%s\",\"value\":%u",
               (unsigned long)s, (unsigned long)end,
               (parity_ok && stop_ok) ? "data" : "error",
               val);
        if (!parity_ok) printf(",\"parity_error\":true");
        if (!stop_ok)  printf(",\"frame_error\":true");
        printf("}\n");

        pkt++; byte_count++;
        s = end;
    }

    fprintf(stderr, "{\"ok\":true,\"packets\":%d,\"bytes\":%d}\n", pkt, byte_count);
    return 0;
}

static int decode_spi(const uint8_t *raw, uint64_t samples,
                       int clk_ch, int miso_ch, int mosi_ch, int cs_ch,
                       int stride, uint64_t samplerate, int wordsize,
                       const char *bit_order, int cpol, int cpha) {
    (void)cs_ch; (void)samplerate;
    uint64_t s = 0;
    int pkt = 0;
    int last_clk = cpol;  /* idle state */

    while (s < samples) {
        int clk = sample_bit(raw, s * stride + clk_ch);

        /* detect clock edge based on CPOL/CPHA */
        int sample_edge = 0;
        if (cpol == 0 && cpha == 0) sample_edge = (last_clk == 0 && clk == 1); /* rising */
        if (cpol == 0 && cpha == 1) sample_edge = (last_clk == 1 && clk == 0); /* falling */
        if (cpol == 1 && cpha == 0) sample_edge = (last_clk == 1 && clk == 0); /* falling */
        if (cpol == 1 && cpha == 1) sample_edge = (last_clk == 0 && clk == 1); /* rising */

        if (sample_edge) {
            uint64_t word_start = s;
            uint64_t val_miso = 0, val_mosi = 0;

            for (int i = 0; i < wordsize && s + i < samples; i++) {
                int bit_pos = s + i;
                int b;
                if (!strcmp(bit_order, "lsb"))
                    b = i;
                else
                    b = wordsize - 1 - i;

                if (miso_ch >= 0)
                    val_miso |= (sample_bit(raw, bit_pos * stride + miso_ch) << b);
                if (mosi_ch >= 0)
                    val_mosi |= (sample_bit(raw, bit_pos * stride + mosi_ch) << b);
            }

            printf("{\"start\":%lu,\"end\":%lu,\"type\":\"word\"",
                   (unsigned long)word_start, (unsigned long)(s + wordsize));
            if (mosi_ch >= 0) printf(",\"mosi\":%lu", (unsigned long)val_mosi);
            if (miso_ch >= 0) printf(",\"miso\":%lu", (unsigned long)val_miso);
            printf("}\n");
            pkt++;
            s += wordsize;
        }
        last_clk = clk;
        s++;
    }

    fprintf(stderr, "{\"ok\":true,\"packets\":%d}\n", pkt);
    return 0;
}

static int decode_i2c(const uint8_t *raw, uint64_t samples,
                       int scl_ch, int sda_ch, int stride,
                       uint64_t samplerate) {
    (void)samplerate;
    uint64_t s = 0;
    int pkt = 0;
    int last_scl = 1, last_sda = 1;

    while (s + 16 < samples) {
        int scl = sample_bit(raw, s * stride + scl_ch);
        int sda = sample_bit(raw, s * stride + sda_ch);

        /* START: SDA↓ while SCL=H */
        if (last_scl == 1 && scl == 1 && last_sda == 1 && sda == 0) {
            printf("{\"start\":%lu,\"end\":%lu,\"type\":\"start\"}\n", (unsigned long)s, (unsigned long)s);
            pkt++;
            s++;
            last_scl = scl; last_sda = sda;
            continue;
        }

        /* STOP: SDA↑ while SCL=H */
        if (last_scl == 1 && scl == 1 && last_sda == 0 && sda == 1) {
            printf("{\"start\":%lu,\"end\":%lu,\"type\":\"stop\"}\n", (unsigned long)s, (unsigned long)s);
            pkt++;
            s++;
            last_scl = scl; last_sda = sda;
            continue;
        }

        /* ACK/NACK: SDA low during 9th clock */
        last_scl = scl; last_sda = sda;
        s++;
    }

    fprintf(stderr, "{\"ok\":true,\"packets\":%d}\n", pkt);
    return 0;
}

/* ── decode command ────────────────────────────────────────── */
static int cmd_decode(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "{\"error\":\"usage: decode <protocol> <file> [options]\"}\n");
        return 1;
    }

    const char *proto   = argv[0];
    const char *path    = argv[1];

    /* defaults */
    uint64_t samplerate = 0;
    int    nch = 1;  /* stride: number of channels in capture data */
    int    uart_ch = 0, uart_baud = 115200, uart_data = 8;
    const char *uart_parity = "none";
    double uart_stop = 1.0;

    int    spi_clk = 0, spi_miso = 1, spi_mosi = 2, spi_cs = -1;
    int    spi_wordsize = 8, spi_cpol = 0, spi_cpha = 0;
    const char *spi_bit_order = "msb";

    int    i2c_scl = 0, i2c_sda = 1;

    /* parse options */
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--samplerate") && i+1 < argc)
            samplerate = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--channel") && i+1 < argc)
            uart_ch = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--baudrate") && i+1 < argc)
            uart_baud = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--data-bits") && i+1 < argc)
            uart_data = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--parity") && i+1 < argc)
            uart_parity = argv[++i];
        else if (!strcmp(argv[i], "--stop-bits") && i+1 < argc)
            uart_stop = atof(argv[++i]);
        else if (!strcmp(argv[i], "--clk") && i+1 < argc)
            spi_clk = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--miso") && i+1 < argc)
            spi_miso = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mosi") && i+1 < argc)
            spi_mosi = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--cs") && i+1 < argc)
            spi_cs = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--wordsize") && i+1 < argc)
            spi_wordsize = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--cpol") && i+1 < argc)
            spi_cpol = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--cpha") && i+1 < argc)
            spi_cpha = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--bit-order") && i+1 < argc)
            spi_bit_order = argv[++i];
        else if (!strcmp(argv[i], "--scl") && i+1 < argc)
            i2c_scl = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--sda") && i+1 < argc)
            i2c_sda = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--channels") && i+1 < argc)
            nch = atoi(argv[++i]);
    }

    if (!samplerate) {
        fprintf(stderr, "{\"error\":\"--samplerate required\"}\n");
        return 1;
    }

    /* load capture file */
    FILE *fp = fopen(path, "rb");
    if (!fp) { fprintf(stderr, "{\"error\":\"cannot_open\",\"file\":\"%s\"}\n", path); return 1; }
    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *raw = malloc(fsz);
    if (!raw) { fclose(fp); return 1; }
    size_t n = fread(raw, 1, fsz, fp);
    fclose(fp);
    uint64_t samples = n; /* 1 byte/sample for ≤8ch */

    fprintf(stderr, "{\"info\":{\"file\":\"%s\",\"bytes\":%zu,\"samples\":%lu,\"samplerate\":%lu}}\n",
            path, n, (unsigned long)samples, (unsigned long)samplerate);

    int ret = 0;
    if (!strcmp(proto, "uart"))
        ret = decode_uart(raw, samples, uart_ch, nch, samplerate,
                          uart_baud, uart_data, uart_parity, uart_stop);
    else if (!strcmp(proto, "spi"))
        ret = decode_spi(raw, samples, spi_clk, spi_miso, spi_mosi, spi_cs,
                         nch, samplerate, spi_wordsize, spi_bit_order, spi_cpol, spi_cpha);
    else if (!strcmp(proto, "i2c"))
        ret = decode_i2c(raw, samples, i2c_scl, i2c_sda, nch, samplerate);
    else {
        fprintf(stderr, "{\"error\":\"unknown_protocol\",\"proto\":\"%s\"}\n", proto);
        ret = 1;
    }

    free(raw);
    return ret;
}

/* ── dispatch ──────────────────────────────────────────────── */
static int dispatch(int argc, char **argv) {
    const char *cmd = argv[0];

    /* discovery (no session needed) */
    if (!strcmp(cmd, "scan"))  return cmd_scan();

    /* session */
    if (!strcmp(cmd, "open")  && argc >= 2) return cmd_open(atoi(argv[1]));
    if (!strcmp(cmd, "close"))             return cmd_close();

    /* info */
    if (!strcmp(cmd, "info"))              return cmd_info();

    /* config */
    if (!strcmp(cmd, "modes"))             return cmd_modes();
    if (!strcmp(cmd, "channels"))          return cmd_channels();
    if (!strcmp(cmd, "enable") && argc >= 3)
        return cmd_enable(atoi(argv[1]), atoi(argv[2]));
    if (!strcmp(cmd, "vth")   && argc >= 2)
        return cmd_vth(argv[1]);

    /* mode & rate */
    if (!strcmp(cmd, "stream")       && argc >= 2)
        return cmd_stream(atoi(argv[1]));
    if (!strcmp(cmd, "samplerate")    && argc >= 2)
        return cmd_samplerate(strtoull(argv[1], NULL, 10));
    if (!strcmp(cmd, "channel_mode")  && argc >= 2)
        return cmd_channel_mode(atoi(argv[1]));
    if (!strcmp(cmd, "limit_samples") && argc >= 2)
        return cmd_limit_samples(strtoull(argv[1], NULL, 10));

    /* trigger */
    if (!strcmp(cmd, "trigger"))
        return cmd_trigger(argc - 1, argv + 1);

    /* capture */
    if (!strcmp(cmd, "capture")) {
        double dur = (argc >= 2) ? atof(argv[1]) : 0.0;
        const char *out = (argc >= 3) ? argv[2] : NULL;
        int   to  = (argc >= 4) ? atoi(argv[3]) : 30;
        return cmd_capture(dur, out, to);
    }

    /* decode */
    if (!strcmp(cmd, "decode"))
        return cmd_decode(argc - 1, argv + 1);

    /* version */
    if (!strcmp(cmd, "--version") || !strcmp(cmd, "version")) {
        pj("{\"version\":\"%s\"}", VERSION);
        return 0;
    }
    if (!strcmp(cmd, "--help") || !strcmp(cmd, "help")) {
        fprintf(stdout,
            "dslogic-cli v" VERSION " — DSLogic native CLI\n"
            "\n"
            "DISCOVERY:  scan | info\n"
            "SESSION:    open <idx> | close\n"
            "CONFIG:     modes | channels | enable <ch> <0|1>\n"
            "            vth <1.8|2.5|3.3|5.0>\n"
            "MODE/RATE:  stream <0|1> | channel_mode <id>\n"
            "            samplerate <hz> | limit_samples <n>\n"
            "TRIGGER:    trigger [reset|enable <0|1>|pos <0-100>\n"
            "                   |stage <st> <probes_hex> <t0> [t1] ...]\n"
            "CAPTURE:    capture [duration_sec] [outfile|-] [timeout_sec]\n"
            "            duration = 0 → auto-max, see stderr for info\n"
            "DECODE:     decode <uart|spi|i2c> <file> --samplerate <hz> [opts]\n"
            "            uart: --channel <n> --baudrate <n> --data-bits <n>\n"
            "                  --parity <none|odd|even> --stop-bits <1|1.5|2>\n"
            "            spi:  --clk <n> --miso <n> --mosi <n> --wordsize <n>\n"
            "                  --cpol <0|1> --cpha <0|1> --bit-order <msb|lsb>\n"
            "            i2c:  --scl <n> --sda <n>\n"
            "BATCH:      run (reads commands from stdin)\n"
            "\n"
            "JSON on stdout, diagnostics on stderr.\n"
            "Env: DSL_FW_DIR — path to firmware bitstreams (default /opt/dslogic/res)\n"
        );
        fflush(stdout);
        return 0;
    }

    pj("{\"error\":\"unknown_cmd\",\"cmd\":\"%s\"}", cmd);
    return 1;
}

/* ── run ───────────────────────────────────────────────────── */
static int cmd_run(void) {
    char line[1024];
    int ln = 0, ret = 0;

    while (fgets(line, sizeof(line), stdin)) {
        ln++;
        line[strcspn(line, "\n")] = 0;
        if (!line[0] || line[0] == '#')
            continue;

        char *argv_[64];
        int argc_ = 0;
        char *tok = strtok(line, " ");
        while (tok && argc_ < 63) {
            argv_[argc_++] = tok;
            tok = strtok(NULL, " ");
        }
        if (!argc_)
            continue;
        argv_[argc_] = NULL;

        fprintf(stderr, "[run:%d] %s\n", ln, line);
        ret = dispatch(argc_, argv_);
        if (ret) {
            fprintf(stderr, "[run:%d] FAILED (exit %d)\n", ln, ret);
            break;
        }
    }
    return ret;
}

/* ── main ──────────────────────────────────────────────────── */
static void on_sig(int s) {
    (void)s;
    if (g_collecting)
        ds_stop_collect();
    if (!g_closed)
        ds_release_actived_device();
    ds_lib_exit();
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGINT,  on_sig);
    signal(SIGTERM, on_sig);

    ds_set_firmware_resource_dir(firmware_dir());

    if (ds_lib_init() != SR_OK) {
        fprintf(stderr, "{\"error\":\"init_failed\"}\n");
        return 1;
    }

    ds_set_datafeed_callback(on_datafeed);
    ds_set_event_callback(on_event);

    if (argc < 2) {
        dispatch(1, (char *[]){(char *)"--help"});
        if (!g_closed)
            ds_release_actived_device();
        ds_lib_exit();
        return 1;
    }

    int ret;
    if (!strcmp(argv[1], "run"))
        ret = cmd_run();
    else
        ret = dispatch(argc - 1, argv + 1);

    if (!g_closed)
        ds_release_actived_device();
    ds_lib_exit();
    return ret;
}
