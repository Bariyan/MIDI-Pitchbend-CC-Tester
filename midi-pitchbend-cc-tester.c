#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "midi-pitchbend-cc-tester.h"

typedef struct {
  LV2_URID midi_event;
} URIs;

typedef struct {
  LV2_Atom_Sequence* out;
  const float* enable;
  const float* interval_ms;
  const float* channel;
  const float* note_number;
  const float* velocity;
  const float* pb_value;
  const float* cc_breath;
  const float* cc_expression;
  const float* cc_modulation;
  const float* cc_extra_num;
  const float* cc_extra_val;

  double sample_rate;
  uint32_t frames_since_emit;
  uint8_t last_note;
  bool note_active;

  LV2_URID_Map* map;
  URIs uris;
  LV2_Atom_Forge forge;
  LV2_Atom_Forge_Frame seq_frame;
} Plugin;

static int clampi(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }

static uint32_t interval_frames(Plugin* self){
  float ms = (self->interval_ms) ? *self->interval_ms : 20.0f;
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
    case PORT_OUT:           s->out = (LV2_Atom_Sequence*)d; break;
    case PORT_ENABLE:        s->enable = (const float*)d; break;
    case PORT_INTERVAL_MS:   s->interval_ms = (const float*)d; break;
    case PORT_CHANNEL:       s->channel = (const float*)d; break;
    case PORT_NOTE_NUMBER:   s->note_number = (const float*)d; break;
    case PORT_VELOCITY:      s->velocity = (const float*)d; break;
    case PORT_PB_VALUE:      s->pb_value = (const float*)d; break;
    case PORT_CC_BREATH:     s->cc_breath = (const float*)d; break;
    case PORT_CC_EXPRESSION: s->cc_expression = (const float*)d; break;
    case PORT_CC_MODULATION: s->cc_modulation = (const float*)d; break;
    case PORT_CC_EXTRA_NUM:  s->cc_extra_num = (const float*)d; break;
    case PORT_CC_EXTRA_VAL:  s->cc_extra_val = (const float*)d; break;
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

  lv2_atom_forge_set_buffer(&s->forge, (uint8_t*)s->out, MAX_BUFFER_SIZE);
  lv2_atom_forge_sequence_head(&s->forge, &s->seq_frame, 0);

  bool active = (s->enable && *s->enable >= 0.5f);
  uint8_t ch = (s->channel) ? (uint8_t)clampi((int)*s->channel, 0, 15) : 0;
  uint8_t note = (s->note_number) ? (uint8_t)clampi((int)*s->note_number, 0, 127) : 60;
  uint8_t vel = (s->velocity) ? (uint8_t)clampi((int)*s->velocity, 1, 127) : 64;

  /* Note On / Off Management */
  if (active && !s->note_active) {
    uint8_t note_on[3] = { (uint8_t)(0x90 | ch), note, vel };
    emit(s, 0, note_on);
    s->last_note = note;
    s->note_active = true;
  } else if (!active && s->note_active) {
    uint8_t note_off[3] = { (uint8_t)(0x80 | ch), s->last_note, 0 };
    emit(s, 0, note_off);
    s->note_active = false;
  }

  /* Handle Note Change while Active (Slur) */
  if (active && s->note_active && note != s->last_note) {
    uint8_t note_off[3] = { (uint8_t)(0x80 | ch), s->last_note, 0 };
    uint8_t note_on[3] = { (uint8_t)(0x90 | ch), note, vel };
    emit(s, 0, note_off);
    emit(s, 0, note_on);
    s->last_note = note;
  }

  /* Periodic Continuous Controllers (CC / Pitch Bend) Output */
  if (active) {
    s->frames_since_emit += n;
    if (s->frames_since_emit >= interval_frames(s)) {
      s->frames_since_emit = 0;

      /* 1. Pitch Bend */
      int pb = (s->pb_value) ? (int)lrintf(*s->pb_value) : 0;
      int b  = clampi(pb + 8192, 0, 16383);
      uint8_t pbm[3] = { (uint8_t)(0xE0 | ch), (uint8_t)(b & 0x7F), (uint8_t)((b >> 7) & 0x7F) };
      emit(s, 0, pbm);

      /* 2. CC #2 (Breath Control) */
      uint8_t breath = (s->cc_breath) ? (uint8_t)clampi((int)*s->cc_breath, 0, 127) : 0;
      uint8_t cc2[3] = { (uint8_t)(0xB0 | ch), 2, breath };
      emit(s, 0, cc2);

      /* 3. CC #11 (Expression) */
      uint8_t expr = (s->cc_expression) ? (uint8_t)clampi((int)*s->cc_expression, 0, 127) : 0;
      uint8_t cc11[3] = { (uint8_t)(0xB0 | ch), 11, expr };
      emit(s, 0, cc11);

      /* 4. CC #1 (Modulation) */
      uint8_t mod = (s->cc_modulation) ? (uint8_t)clampi((int)*s->cc_modulation, 0, 127) : 0;
      uint8_t cc1[3] = { (uint8_t)(0xB0 | ch), 1, mod };
      emit(s, 0, cc1);

      /* 5. Extra CC (Assignable) */
      uint8_t ex_num = (s->cc_extra_num) ? (uint8_t)clampi((int)*s->cc_extra_num, 0, 127) : 7;
      uint8_t ex_val = (s->cc_extra_val) ? (uint8_t)clampi((int)*s->cc_extra_val, 0, 127) : 0;
      uint8_t cc_ex[3] = { (uint8_t)(0xB0 | ch), ex_num, ex_val };
      emit(s, 0, cc_ex);
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