// i2c_protocol.h
// The I2C command-byte constants shared by the MacroPad (I2C host) and the
// QT Py LED driver (I2C peripheral on Wire1 — see the QT Py sketch). This
// covers only what the Test Pattern feature needs: start it, stop it — not
// a general preset-rendering protocol, which is future work pending the
// still-undecided monthly scheme content. See
// docs/superpowers/specs/2026-10-23-outdoor-led-driver-architecture-design.md
// ("MacroPad <-> QT Py command link").
//
// Exposes:
//   QTPY_I2C_ADDRESS     — the QT Py's I2C peripheral address
//   QTPY_CMD_STOP        — stop Test Pattern (fire-and-forget, no ACK read)
//   QTPY_CMD_START       — start Test Pattern (host reads back QTPY_ACK_BYTE)
//   QTPY_CMD_PING        — heartbeat only, no effect on Test Pattern
//                          run state. MUST carry this one real payload
//                          byte, not a bare zero-byte transaction: the
//                          installed arduino-pico core's Wire.cpp only
//                          invokes the peripheral's onReceive callback
//                          `if (_buffLen)` — a zero-byte write (address,
//                          no data) never reaches the QT Py's handler at
//                          all, confirmed against that library's own
//                          source. A presence probe that doesn't write
//                          this byte will still correctly ACK on the
//                          MacroPad's connectivity check (that's a
//                          hardware-level address ACK, unrelated to
//                          firmware) but will NOT register as contact
//                          for QTPY_LINK_TIMEOUT_MS below.
//   QTPY_ACK_BYTE        — the fixed response byte to QTPY_CMD_START
//   QTPY_STATUS_POLL_MS  — how often the MacroPad probes for QT Py presence
//                          (ui_controller.h's corner icon) — defined here,
//                          not just on the MacroPad side, because
//                          QTPY_LINK_TIMEOUT_MS below depends on it: the QT
//                          Py treats any QTPY_CMD_PING/START/STOP as a
//                          heartbeat, so its own timeout must stay
//                          comfortably longer than this interval or normal
//                          polling jitter would read as a dropped link
//   QTPY_LINK_TIMEOUT_MS — the QT Py's own threshold for "haven't heard from
//                          the MacroPad in too long" (qtpy_led_driver's
//                          link_status_led.h) — reuses whatever I2C traffic
//                          already happens (the presence probe's
//                          QTPY_CMD_PING, or a real Start/Stop) rather than
//                          a separate protocol, keeping this minimal

#pragma once

#define QTPY_I2C_ADDRESS     0x55
#define QTPY_CMD_STOP        0x00
#define QTPY_CMD_START       0x01
#define QTPY_CMD_PING        0x02
#define QTPY_ACK_BYTE        0xAC
#define QTPY_STATUS_POLL_MS  1000
#define QTPY_LINK_TIMEOUT_MS (3 * QTPY_STATUS_POLL_MS)
