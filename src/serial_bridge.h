/**
 * serial_bridge.h — COBS-framed serial parser for ESP32 gauge packets
 * 
 * Reads gauge data from ESP32 over UART/serial and updates room gauges
 * in the holodeck MUD. COBS framing ensures reliable packet boundaries
 * without requiring special delimiter bytes inside payloads.
 * 
 * Packet format (after COBS decode):
 *   [1 byte: packet_type] [1 byte: gauge_count] [N * gauge_entry]
 *   
 * gauge_entry (10 bytes each):
 *   [8 bytes: name (null-padded)] [2 bytes: int16 value]
 *   
 * Packet types:
 *   0x01 = GAUGE_UPDATE  — update gauge values
 *   0x02 = GAUGE_ACK     — acknowledge previous update
 *   0x03 = HEARTBEAT     — ESP32 still alive, no data
 *   0xFF = ERROR         — ESP32 reports fault
 *
 * Usage:
 *   SerialBridge *bridge = serial_bridge_create("/dev/ttyUSB0", 115200);
 *   serial_bridge_attach(bridge, room, on_gauge_update);
 *   serial_bridge_poll(bridge);  // call from event loop
 *   serial_bridge_destroy(bridge);
 */

#ifndef SERIAL_BRIDGE_H
#define SERIAL_BRIDGE_H

#include "holodeck.h"
#include <stddef.h>
#include <stdint.h>

/* ═══ Constants ═══ */
#define SERIAL_MAX_PKT_SIZE   256    /* max COBS-encoded frame */
#define SERIAL_MAX_DECODED    254    /* max decoded payload */
#define SERIAL_MAX_GAUGES     24     /* max gauges per packet */
#define SERIAL_GAUGE_NAME_LEN 8      /* bytes per gauge name */
#define SERIAL_BUF_SIZE       512    /* serial read buffer */

/* ═══ Packet Types ═══ */
typedef enum {
    PKT_GAUGE_UPDATE = 0x01,
    PKT_GAUGE_ACK    = 0x02,
    PKT_HEARTBEAT    = 0x03,
    PKT_ERROR        = 0xFF
} SerialPacketType;

/* ═══ Gauge Entry (10 bytes) ═══ */
typedef struct __attribute__((packed)) {
    char   name[SERIAL_GAUGE_NAME_LEN];  /* null-padded gauge name */
    int16_t value;                       /* gauge reading */
} SerialGaugeEntry;

/* ═══ Decoded Packet ═══ */
typedef struct {
    SerialPacketType type;
    uint8_t          gauge_count;
    SerialGaugeEntry gauges[SERIAL_MAX_GAUGES];
} SerialPacket;

/* ═══ Gauge Update Callback ═══ */
typedef void (*GaugeCallback)(void *context, const char *gauge_name, 
                               int16_t value, double normalized);

/* ═══ Statistics ═══ */
typedef struct {
    uint32_t packets_received;
    uint32_t packets_decoded;
    uint32_t packets_failed;
    uint32_t bytes_received;
    uint32_t gauge_updates;
    uint32_t heartbeats;
    uint32_t errors;
    uint32_t framing_errors;
} SerialStats;

/* ═══ Serial Bridge ═══ */
typedef struct SerialBridge SerialBridge;

/* Create/destroy */
SerialBridge *serial_bridge_create(const char *device, int baud_rate);
void          serial_bridge_destroy(SerialBridge *bridge);

/* Attach to a room (for gauge updates) */
int serial_bridge_attach(SerialBridge *bridge, void *room, GaugeCallback callback);

/* Poll for incoming data — call from event loop */
int serial_bridge_poll(SerialBridge *bridge);

/* Send command to ESP32 */
int serial_bridge_send(SerialBridge *bridge, const uint8_t *data, size_t len);

/* Get statistics */
const SerialStats *serial_bridge_stats(const SerialBridge *bridge);

/* ═══ COBS Encode/Decode (standalone, testable) ═══ */

/**
 * COBS encode a payload into a framed buffer.
 * Returns encoded length (including trailing 0x00 delimiter).
 * output must be at least input_len + 2 bytes.
 */
size_t cobs_encode(const uint8_t *input, size_t input_len, 
                   uint8_t *output, size_t output_max);

/**
 * COBS decode a framed buffer into payload.
 * Returns decoded length, or -1 on error.
 * output must be at least input_len bytes.
 */
int cobs_decode(const uint8_t *input, size_t input_len,
                uint8_t *output, size_t output_max);

/**
 * Parse a decoded payload into a SerialPacket struct.
 * Returns 0 on success, -1 on error.
 */
int serial_parse_packet(const uint8_t *payload, size_t len, SerialPacket *pkt);

/**
 * Encode a SerialPacket into COBS-framed bytes.
 * Returns encoded length including delimiter, or -1 on error.
 */
int serial_encode_packet(const SerialPacket *pkt, uint8_t *output, size_t max);

/* ═══ Testing ═══ */

/**
 * Run all serial bridge unit tests.
 * Returns number of failures (0 = all pass).
 */
int serial_bridge_test(void);

#endif /* SERIAL_BRIDGE_H */
