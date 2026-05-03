#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

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
  PORT_PB_VALUE,
  PORT_CC_MSB_NUMBER,
  PORT_CC_MSB_VALUE,
  PORT_CC_LSB_VALUE
} PortIndex;

typedef struct {
  LV2_URID midi_event;
} URIs;

typedef struct {
  LV2_Atom_Sequence* out;
  const float* enable;
  const float* interval_ms;
  const float* channel;
  const float* pb_value;
  const float* cc_msb_number;
  const float* cc_msb_value;
  const float* cc_lsb_value;

  double sample_rate;
  uint32_t frames_since_emit;

  LV2_URID_Map* map;
  URIs uris;
  LV2_Atom_Forge forge;
  LV2_Atom_Forge_Frame seq_frame;
} Plugin;

static int clampi(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }

static uint32_t interval_frames(Plugin* self){
  float ms = (self->interval_ms) ? *self->interval_ms : 100.0f;
  // sample_rateが0または負の場合の保護
  double rate = (self->sample_rate > 0) ? self->sample_rate : 44100.0;
  uint32_t f = (uint32_t)llround(rate * (double)ms / 1000.0);
  return f < 1 ? 1 : f;
}

static LV2_Handle instantiate(const LV2_Descriptor* d, double rate,
                              const char* p, const LV2_Feature* const* f){
  (void)d; (void)p;
  Plugin* self = (Plugin*)calloc(1, sizeof(Plugin));
  if (!self) return NULL;

  self->sample_rate = rate;

  for (; f && *f; ++f) {
    if (!strcmp((*f)->URI, LV2_URID__map)) {
      self->map = (LV2_URID_Map*)(*f)->data;
    }
  }

  if (!self->map) {
    free(self);
    return NULL;
  }

  self->uris.midi_event = self->map->map(self->map->handle, LV2_MIDI__MidiEvent);
  lv2_atom_forge_init(&self->forge, self->map);

  return (LV2_Handle)self;
}

static void connect_port(LV2_Handle i, uint32_t p, void* d){
  Plugin* s = (Plugin*)i;
  switch ((PortIndex)p) {
    case PORT_OUT: s->out = (LV2_Atom_Sequence*)d; break;
    case PORT_ENABLE: s->enable = (const float*)d; break;
    case PORT_INTERVAL_MS: s->interval_ms = (const float*)d; break;
    case PORT_CHANNEL: s->channel = (const float*)d; break;
    case PORT_PB_VALUE: s->pb_value = (const float*)d; break;
    case PORT_CC_MSB_NUMBER: s->cc_msb_number = (const float*)d; break;
    case PORT_CC_MSB_VALUE: s->cc_msb_value = (const float*)d; break;
    case PORT_CC_LSB_VALUE: s->cc_lsb_value = (const float*)d; break;
    default: break;
  }
}

static void emit(Plugin* s, int64_t t, const uint8_t* m){
  lv2_atom_forge_frame_time(&s->forge, t);
  lv2_atom_forge_atom(&s->forge, 3, s->uris.midi_event);
  lv2_atom_forge_write(&s->forge, m, 3);
}

static void run(LV2_Handle i, uint32_t n){
  Plugin* s = (Plugin*)i;
  if (!s->out) return;

  // Forgeを毎サイクル確実に初期化
  lv2_atom_forge_set_buffer(&s->forge, (uint8_t*)s->out, MAX_BUFFER_SIZE);
  lv2_atom_forge_sequence_head(&s->forge, &s->seq_frame, 0);

  if (s->enable && *s->enable >= 0.5f) {
    s->frames_since_emit += n;
    if (s->frames_since_emit >= interval_frames(s)) {
      s->frames_since_emit = 0;

      uint8_t ch = (s->channel) ? (uint8_t)clampi((int)*s->channel, 0, 15) : 0;

      // 1. Pitch Bend
      int pb = (s->pb_value) ? (int)lrintf(*s->pb_value) : 0;
      int b  = clampi(pb + 8192, 0, 16383);
      uint8_t pbm[3] = { (uint8_t)(0xE0 | ch), (uint8_t)(b & 0x7F), (uint8_t)((b >> 7) & 0x7F) };
      emit(s, 0, pbm);

      // 2. CC MSB
      uint8_t msb_num = (s->cc_msb_number) ? (uint8_t)clampi((int)*s->cc_msb_number, 0, 127) : 1;
      uint8_t msb_val = (s->cc_msb_value)  ? (uint8_t)clampi((int)*s->cc_msb_value,  0, 127) : 0;
      uint8_t msb[3] = { (uint8_t)(0xB0 | ch), msb_num, msb_val };
      emit(s, 0, msb);

      // 3. CC LSB: 0-31かつValue > 0の時のみ
      if (msb_num <= 31) {
        uint8_t lsb_val = (s->cc_lsb_value) ? (uint8_t)clampi((int)*s->cc_lsb_value, 0, 127) : 0;
        if (lsb_val > 0) {
          uint8_t lsb_num = msb_num + 32;
          uint8_t lsb[3] = { (uint8_t)(0xB0 | ch), lsb_num, lsb_val };
          emit(s, 0, lsb);
        }
      }
    }
  }
  
  lv2_atom_forge_pop(&s->forge, &s->seq_frame);
}

static void cleanup(LV2_Handle i){
  if (i) free(i);
}

static const LV2_Descriptor descriptor = {
  PLUGIN_URI, instantiate, connect_port, NULL, run, NULL, cleanup, NULL
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t i){
  return (i == 0) ? &descriptor : NULL;
}