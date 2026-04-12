/**
 * serial_bridge.c — COBS-framed serial parser for ESP32 gauge packets
 *
 * Protocol:
 *   ESP32 sends COBS-encoded frames over UART, terminated by 0x00.
 *   Each frame contains a packet type, gauge count, and gauge entries.
 *   The bridge decodes, parses, and calls the gauge update callback.
 *
 * COBS (Consistent Overhead Byte Stuffing):
 *   - 0x00 is the frame delimiter (not encoded)
 *   - All other bytes are encoded: runs of non-zero bytes are prefixed
 *     with their length, zero bytes become the distance to next 0x00
 *   - Guarantees unambiguous framing without escaping
 *
 * Build: gcc -std=c99 -Wall -Wextra -pedantic -c serial_bridge.c -I.
 */

#include "serial_bridge.h"
#include "serial_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

/* ═══ Internal State ═══ */

struct SerialBridge {
    int              fd;          /* serial file descriptor (-1 = simulated) */
    void            *room;        /* attached room (NULL = no room) */
    GaugeCallback    callback;    /* gauge update callback */
    SerialStats      stats;       /* statistics */
    uint8_t          rx_buf[SERIAL_BUF_SIZE]; /* receive buffer */
    size_t           rx_pos;      /* current position in rx buffer */
    int              simulated;   /* 1 = use simulated input for testing */
    uint8_t         *sim_data;    /* simulated COBS frame to decode */
    size_t           sim_len;     /* simulated data length */
    size_t           sim_pos;     /* simulated read position */
};

/* ═══ COBS Encode ═══ */

size_t cobs_encode(const uint8_t *input, size_t input_len,
                   uint8_t *output, size_t output_max) {
    if (!input || !output || input_len == 0 || output_max < 2) return 0;
    if (input_len > 254) return 0;

    size_t out_pos = 0;
    size_t code_pos = 0;  /* position of the current code byte */
    uint8_t code = 1;      /* current run length (starts at 1 for the code byte itself) */

    output[0] = 0xFF;     /* placeholder, will be overwritten */
    out_pos = 1;

    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == 0x00) {
            /* Zero byte — close current block */
            output[code_pos] = code;
            code_pos = out_pos;
            code = 1;
            out_pos++;
        } else {
            output[out_pos++] = input[i];
            code++;
            if (code == 0xFF) {
                /* Block full — close it */
                output[code_pos] = code;
                code_pos = out_pos;
                code = 1;
                out_pos++;
            }
        }
    }

    /* Close final block */
    output[code_pos] = code;

    /* Append frame delimiter */
    if (out_pos + 1 >= output_max) return 0;
    output[out_pos] = 0x00;

    return out_pos + 1;  /* encoded length including delimiter */
}

/* ═══ COBS Decode ═══ */

int cobs_decode(const uint8_t *input, size_t input_len,
                uint8_t *output, size_t output_max) {
    if (!input || !output || input_len < 2) return -1;

    /* Find frame boundary (0x00 at end) */
    size_t frame_len = input_len;
    if (input[frame_len - 1] != 0x00) {
        int found = 0;
        for (size_t i = 0; i < input_len; i++) {
            if (input[i] == 0x00) { frame_len = i + 1; found = 1; break; }
        }
        if (!found) return -1;
    }

    size_t in_pos = 0;
    size_t out_pos = 0;

    while (in_pos < frame_len - 1) {
        uint8_t code = input[in_pos++];
        if (code == 0x00) return -1;

        for (uint8_t j = 1; j < code && in_pos < frame_len - 1; j++) {
            if (out_pos >= output_max) return -1;
            output[out_pos++] = input[in_pos++];
        }

        /* Insert zero only if there are more code blocks to process */
        if (code < 0xFF && in_pos < frame_len - 1) {
            if (out_pos >= output_max) return -1;
            output[out_pos++] = 0x00;
        }
    }

    return (int)out_pos;
}

/* ═══ Packet Parse ═══ */

int serial_parse_packet(const uint8_t *payload, size_t len, SerialPacket *pkt) {
    if (!payload || !pkt || len < 2) return -1;

    memset(pkt, 0, sizeof(*pkt));
    pkt->type = (SerialPacketType)payload[0];
    pkt->gauge_count = payload[1];

    /* Validate */
    if (pkt->type == 0x00) return -1;
    if (pkt->gauge_count > SERIAL_MAX_GAUGES) return -1;

    size_t expected = 2 + (pkt->gauge_count * sizeof(SerialGaugeEntry));
    if (len < expected) return -1;

    /* Parse gauge entries */
    const uint8_t *ptr = payload + 2;
    for (uint8_t i = 0; i < pkt->gauge_count; i++) {
        memcpy(pkt->gauges[i].name, ptr, SERIAL_GAUGE_NAME_LEN);
        ptr += SERIAL_GAUGE_NAME_LEN;
        /* int16_t — little-endian */
        pkt->gauges[i].value = (int16_t)(ptr[0] | (ptr[1] << 8));
        ptr += 2;
    }

    return 0;
}

/* ═══ Packet Encode ═══ */

int serial_encode_packet(const SerialPacket *pkt, uint8_t *output, size_t max) {
    if (!pkt || !output) return -1;

    uint8_t payload[SERIAL_MAX_DECODED];
    memset(payload, 0, sizeof(payload));

    payload[0] = (uint8_t)pkt->type;
    payload[1] = pkt->gauge_count;

    size_t payload_len = 2;
    if (pkt->gauge_count > SERIAL_MAX_GAUGES) return -1;

    uint8_t *ptr = payload + 2;
    for (uint8_t i = 0; i < pkt->gauge_count; i++) {
        memcpy(ptr, pkt->gauges[i].name, SERIAL_GAUGE_NAME_LEN);
        ptr += SERIAL_GAUGE_NAME_LEN;
        /* int16_t little-endian */
        *ptr++ = (uint8_t)(pkt->gauges[i].value & 0xFF);
        *ptr++ = (uint8_t)((pkt->gauges[i].value >> 8) & 0xFF);
        payload_len += sizeof(SerialGaugeEntry);
    }

    return (int)cobs_encode(payload, payload_len, output, max);
}

/* ═══ Serial Port Open ═══ */

static int serial_open(const char *device, int baud_rate) {
    if (!device) return -1;

    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) return -1;

    struct termios tty;
    tcgetattr(fd, &tty);

    /* Baud rate */
    speed_t baud = B115200;
    switch (baud_rate) {
        case 9600:   baud = B9600;   break;
        case 19200:  baud = B19200;  break;
        case 38400:  baud = B38400;  break;
        case 57600:  baud = B57600;  break;
        case 115200: baud = B115200; break;
        case 230400: baud = B230400; break;
        case 460800: baud = B460800; break;
        default:     baud = B115200; break;
    }
    cfsetispeed(&tty, baud);
    cfsetospeed(&tty, baud);

    /* 8N1, no parity, raw mode */
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN]  = 0;  /* non-blocking read */
    tty.c_cc[VTIME] = 1;  /* 100ms inter-byte timeout */

    tcsetattr(fd, TCSANOW, &tty);

    return fd;
}

/* ═══ Bridge Create/Destroy ═══ */

SerialBridge *serial_bridge_create(const char *device, int baud_rate) {
    SerialBridge *bridge = calloc(1, sizeof(SerialBridge));
    if (!bridge) return NULL;

    bridge->fd = -1;
    bridge->room = NULL;
    bridge->callback = NULL;
    bridge->rx_pos = 0;
    bridge->simulated = 0;

    if (device && strcmp(device, "sim") != 0) {
        bridge->fd = serial_open(device, baud_rate);
        if (bridge->fd < 0) {
            /* Can't open serial — fail gracefully, allow simulated mode */
            fprintf(stderr, "serial_bridge: cannot open %s (%s)\n",
                    device, strerror(errno));
            bridge->simulated = 1;
        }
    } else {
        bridge->simulated = 1;
    }

    return bridge;
}

void serial_bridge_destroy(SerialBridge *bridge) {
    if (!bridge) return;
    if (bridge->fd >= 0) close(bridge->fd);
    /* Don't free sim_data — it may be stack-allocated in tests */
    free(bridge);
}

/* ═══ Attach to Room ═══ */

int serial_bridge_attach(SerialBridge *bridge, void *room, GaugeCallback callback) {
    if (!bridge) return -1;
    bridge->room = room;
    bridge->callback = callback;
    return 0;
}

/* ═══ Process One Frame ═══ */

static int process_frame(SerialBridge *bridge, const uint8_t *frame, size_t len) {

    /* COBS decode */
    uint8_t decoded[SERIAL_MAX_DECODED];
    int decoded_len = cobs_decode(frame, len, decoded, sizeof(decoded));
    if (decoded_len < 0) {
        bridge->stats.packets_failed++;
        bridge->stats.framing_errors++;
        return -1;
    }

    bridge->stats.packets_decoded++;

    /* Parse packet */
    SerialPacket pkt;
    if (serial_parse_packet(decoded, (size_t)decoded_len, &pkt) < 0) {
        bridge->stats.packets_failed++;
        return -1;
    }

    /* Handle packet type */
    switch (pkt.type) {
    case PKT_HEARTBEAT:
        bridge->stats.heartbeats++;
        break;

    case PKT_GAUGE_UPDATE:
        bridge->stats.gauge_updates += pkt.gauge_count;
        /* Update room gauges via callback */
        if (bridge->callback && bridge->room) {
            for (uint8_t i = 0; i < pkt.gauge_count; i++) {
                /* Normalize int16 to [0,1] range */
                double normalized = (double)pkt.gauges[i].value / 32768.0;
                if (normalized < 0.0) normalized = 0.0;
                if (normalized > 1.0) normalized = 1.0;

                /* Null-terminate gauge name (may not be null-terminated in struct) */
                char name[SERIAL_GAUGE_NAME_LEN + 1];
                memcpy(name, pkt.gauges[i].name, SERIAL_GAUGE_NAME_LEN);
                name[SERIAL_GAUGE_NAME_LEN] = '\0';

                bridge->callback(bridge->room, name, pkt.gauges[i].value, normalized);
            }
        }
        break;

    case PKT_ERROR:
        bridge->stats.errors++;
        break;

    default:
        bridge->stats.packets_failed++;
        break;
    }

    return 0;
}

/* ═══ Poll for Data ═══ */

int serial_bridge_poll(SerialBridge *bridge) {
    if (!bridge) return -1;

    uint8_t buf[256];
    ssize_t n;

    if (bridge->simulated) {
        /* Simulated mode — read from sim_data buffer */
        size_t avail = bridge->sim_len - bridge->sim_pos;
        size_t to_read = avail < 256 ? avail : 256;
        memcpy(buf, bridge->sim_data + bridge->sim_pos, to_read);
        n = (ssize_t)to_read;
        bridge->sim_pos += to_read;
    } else {
        /* Real serial — non-blocking read */
        n = read(bridge->fd, buf, sizeof(buf));
        if (n <= 0) return 0;
    }


    /* Accumulate into rx buffer, scanning for 0x00 frame delimiters */
    for (ssize_t i = 0; i < n; i++) {
        uint8_t byte = buf[i];

        if (byte == 0x00 && bridge->rx_pos > 0) {
            /* Frame complete — process it */
            bridge->rx_buf[bridge->rx_pos++] = 0x00;
            process_frame(bridge, bridge->rx_buf, bridge->rx_pos);
            bridge->rx_pos = 0;
        } else if (byte != 0x00) {
            /* Accumulate non-delimiter bytes */
            if (bridge->rx_pos < SERIAL_BUF_SIZE) {
                bridge->rx_buf[bridge->rx_pos++] = byte;
            } else {
                /* Buffer overflow — reset */
                bridge->rx_pos = 0;
                bridge->stats.framing_errors++;
            }
        }
        /* Lone 0x00 (rx_pos == 0) = keep-alive, skip */
    }

    return (int)n;
}

/* ═══ Send to ESP32 ═══ */

int serial_bridge_send(SerialBridge *bridge, const uint8_t *data, size_t len) {
    if (!bridge || !data || len == 0) return -1;
    if (bridge->fd < 0) return -1;

    /* COBS encode + send */
    uint8_t encoded[SERIAL_MAX_PKT_SIZE];
    size_t encoded_len = cobs_encode(data, len, encoded, sizeof(encoded));
    if (encoded_len == 0) return -1;

    ssize_t written = write(bridge->fd, encoded, encoded_len);
    return (written == (ssize_t)encoded_len) ? 0 : -1;
}

/* ═══ Statistics ═══ */

const SerialStats *serial_bridge_stats(const SerialBridge *bridge) {
    if (!bridge) return NULL;
    return &bridge->stats;
}

/* ═══ Unit Tests ═══ */

int serial_bridge_test(void) {
    int failures = 0;
    uint8_t buf[512];

    /* --- COBS encode/decode round-trip --- */
    {
        /* Test 1: simple payload */
        uint8_t input[] = {0x01, 0x02, 0x03};
        size_t enc_len = cobs_encode(input, 3, buf, sizeof(buf));
        if (enc_len != 5) { failures++; printf("FAIL cobs_encode len: got %zu, expected 5\n", enc_len); }
        uint8_t dec[16];
        int dec_len = cobs_decode(buf, enc_len, dec, sizeof(dec));
        if (dec_len != 3 || memcmp(dec, input, 3) != 0) {
            failures++;
            printf("FAIL cobs round-trip test1\n");
        }
    }

    /* Test 2: payload with zeros */
    {
        uint8_t input[] = {0x01, 0x00, 0x02, 0x00, 0x03};
        size_t enc_len = cobs_encode(input, 5, buf, sizeof(buf));
        uint8_t dec[16];
        int dec_len = cobs_decode(buf, enc_len, dec, sizeof(dec));
        if (dec_len != 5 || memcmp(dec, input, 5) != 0) {
            failures++;
            printf("FAIL cobs round-trip test2 (zeros)\n");
        }
    }

    /* Test 3: all zeros */
    {
        uint8_t input[] = {0x00, 0x00, 0x00};
        size_t enc_len = cobs_encode(input, 3, buf, sizeof(buf));
        uint8_t dec[16];
        int dec_len = cobs_decode(buf, enc_len, dec, sizeof(dec));
        if (dec_len != 3 || memcmp(dec, input, 3) != 0) {
            failures++;
            printf("FAIL cobs round-trip test3 (all zeros)\n");
        }
    }

    /* Test 4: 254 bytes (max COBS block) */
    {
        uint8_t input[254];
        memset(input, 0x42, sizeof(input));
        size_t enc_len = cobs_encode(input, 254, buf, sizeof(buf));
        uint8_t dec[256];
        int dec_len = cobs_decode(buf, enc_len, dec, sizeof(dec));
        if (dec_len != 254 || memcmp(dec, input, 254) != 0) {
            failures++;
            printf("FAIL cobs round-trip test4 (254 bytes)\n");
        }
    }

    /* Test 5: all 0xFF */
    {
        uint8_t input[] = {0xFF, 0xFF, 0xFF};
        size_t enc_len = cobs_encode(input, 3, buf, sizeof(buf));
        uint8_t dec[16];
        int dec_len = cobs_decode(buf, enc_len, dec, sizeof(dec));
        if (dec_len != 3 || memcmp(dec, input, 3) != 0) {
            failures++;
            printf("FAIL cobs round-trip test5 (all 0xFF)\n");
        }
    }

    /* --- Packet parse --- */
    {
        /* Build a gauge update packet: type=0x01, count=2, gauge1="temp\0\0\0\0"+0x7F00, gauge2="batt\0\0\0\0"+0x3C00 */
        uint8_t payload[22];
        memset(payload, 0, sizeof(payload));
        payload[0] = 0x01;  /* GAUGE_UPDATE */
        payload[1] = 0x02;  /* 2 gauges */
        memcpy(payload + 2,  "temp", 4);  /* gauge 1 name */
        payload[10] = 0x00; payload[11] = 0x7F;  /* value = 0x7F00 = 32512 */
        memcpy(payload + 12, "batt", 4);  /* gauge 2 name */
        payload[20] = 0x00; payload[21] = 0x3C;  /* value = 0x3C00 = 15360 */

        SerialPacket pkt;
        int rc = serial_parse_packet(payload, sizeof(payload), &pkt);
        if (rc != 0) { failures++; printf("FAIL packet parse\n"); }
        else {
            if (pkt.type != PKT_GAUGE_UPDATE) { failures++; printf("FAIL pkt type\n"); }
            if (pkt.gauge_count != 2) { failures++; printf("FAIL pkt gauge_count\n"); }
            if (pkt.gauges[0].value != 0x7F00) { failures++; printf("FAIL gauge1 value: %d\n", pkt.gauges[0].value); }
            if (pkt.gauges[1].value != 0x3C00) { failures++; printf("FAIL gauge2 value: %d\n", pkt.gauges[1].value); }
            if (strncmp(pkt.gauges[0].name, "temp", 4) != 0) { failures++; printf("FAIL gauge1 name\n"); }
        }
    }

    /* --- Packet encode/decode round-trip --- */
    {
        SerialPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = PKT_GAUGE_UPDATE;
        pkt.gauge_count = 1;
        memcpy(pkt.gauges[0].name, "speed", 5);
        pkt.gauges[0].value = 1234;

        uint8_t encoded[SERIAL_MAX_PKT_SIZE];
        int enc_len = serial_encode_packet(&pkt, encoded, sizeof(encoded));
        if (enc_len <= 0) { failures++; printf("FAIL packet encode\n"); }
        else {
            uint8_t decoded[SERIAL_MAX_DECODED];
            int dec_len = cobs_decode(encoded, (size_t)enc_len, decoded, sizeof(decoded));
            SerialPacket pkt2;
            if (serial_parse_packet(decoded, (size_t)dec_len, &pkt2) != 0) {
                failures++; printf("FAIL packet decode\n");
            } else if (pkt2.gauges[0].value != 1234) {
                failures++; printf("FAIL packet round-trip value\n");
            }
        }
    }

    /* --- Bridge simulated poll --- */
    {
        /* Create a simulated bridge, feed it a COBS-encoded gauge update */
        SerialBridge *bridge = serial_bridge_create("sim", 115200);
        if (!bridge) { failures++; printf("FAIL bridge create\n"); }
        else {
            bridge->simulated = 1;

            /* Build and encode a packet */
            SerialPacket pkt;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = PKT_HEARTBEAT;
            pkt.gauge_count = 0;

            uint8_t encoded[SERIAL_MAX_PKT_SIZE];
            int enc_len = serial_encode_packet(&pkt, encoded, sizeof(encoded));

            /* Feed to bridge */
            bridge->sim_data = encoded;
            bridge->sim_len = (size_t)enc_len;
            bridge->sim_pos = 0;

            serial_bridge_poll(bridge);

            const SerialStats *stats = serial_bridge_stats(bridge);
            if (stats->heartbeats != 1) {
                failures++;
                printf("FAIL bridge heartbeat: got %u, expected 1\n", stats->heartbeats);
            }
            if (stats->packets_decoded != 1) {
                failures++;
                printf("FAIL bridge decoded: got %u\n", stats->packets_decoded);
            }

            serial_bridge_destroy(bridge);
        }
    }

    /* --- Error cases --- */
    {
        /* Empty input */
        int rc = cobs_decode(NULL, 0, buf, sizeof(buf));
        if (rc != -1) { failures++; printf("FAIL decode empty\n"); }

        /* Zero-length input */
        rc = cobs_decode((uint8_t*)"", 0, buf, sizeof(buf));
        if (rc != -1) { failures++; printf("FAIL decode zero len\n"); }

        /* NULL packet parse */
        rc = serial_parse_packet(NULL, 0, NULL);
        if (rc != -1) { failures++; printf("FAIL parse null\n"); }
    }

    printf("serial_bridge_test: %d failures\n", failures);
    return failures;
}
