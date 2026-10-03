#ifndef MIDI_PITCHBEND_CC_TESTER_H
#define MIDI_PITCHBEND_CC_TESTER_H

#include <stdint.h>
#include <stdbool.h>
#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/midi/midi.h>

#define PLUGIN_URI "urn:bariyan:midi-pitchbend-cc-tester"
#define MAX_BUFFER_SIZE 2048

typedef enum {
  PORT_OUT = 0,
  PORT_ENABLE,
  PORT_INTERVAL_MS,
  PORT_CHANNEL,
  PORT_NOTE_NUMBER,
  PORT_VELOCITY,
  PORT_PB_VALUE,
  PORT_CC_BREATH,      /* CC #2 */
  PORT_CC_EXPRESSION,  /* CC #11 */
  PORT_CC_MODULATION,  /* CC #1 */
  PORT_CC_EXTRA_NUM,   /* Extra CC Number */
  PORT_CC_EXTRA_VAL    /* Extra CC Value */
} PortIndex;

#endif /* MIDI_PITCHBEND_CC_TESTER_H */