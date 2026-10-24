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
//   QTPY_I2C_ADDRESS — the QT Py's I2C peripheral address
//   QTPY_CMD_STOP    — stop Test Pattern (fire-and-forget, no ACK read)
//   QTPY_CMD_START   — start Test Pattern (host reads back QTPY_ACK_BYTE)
//   QTPY_ACK_BYTE    — the fixed response byte to QTPY_CMD_START

#pragma once

#define QTPY_I2C_ADDRESS 0x55
#define QTPY_CMD_STOP    0x00
#define QTPY_CMD_START   0x01
#define QTPY_ACK_BYTE    0xAC
