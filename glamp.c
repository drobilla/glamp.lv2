/*
  Copyright 2012 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file glamp.c Example LV2 amp plugin with OpenGL UI.
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define GLAMP_URI "http://drobilla.net/plugins/glamp"

typedef enum {
	GLAMP_GAIN   = 0,
	GLAMP_INPUT  = 1,
	GLAMP_OUTPUT = 2
} PortIndex;

typedef struct {
	float* gain;
	float* input;
	float* output;
} Glamp;

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Glamp* self = (Glamp*)malloc(sizeof(Glamp));

	return (LV2_Handle)self;
}

/** Connect a port to a buffer (audio thread, must be RT safe). */
static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Glamp* self = (Glamp*)instance;

	switch ((PortIndex)port) {
	case GLAMP_GAIN:
		self->gain = (float*)data;
		break;
	case GLAMP_INPUT:
		self->input = (float*)data;
		break;
	case GLAMP_OUTPUT:
		self->output = (float*)data;
		break;
	}
}

/** Initialise and prepare the plugin instance for running. */
static void
activate(LV2_Handle instance)
{
	/* Nothing to do here in this trivial mostly stateless plugin. */
}

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

/** Process a block of audio (audio thread, must be RT safe). */
static void
run(LV2_Handle instance, uint32_t n_samples)
{
	Glamp* self = (Glamp*)instance;

	const float        gain   = *(self->gain);
	const float* const input  = self->input;
	float* const       output = self->output;

	const float coef = DB_CO(gain);

	for (uint32_t pos = 0; pos < n_samples; pos++) {
		output[pos] = input[pos] * coef;
	}
}

/** Finish running (counterpart to activate()). */
static void
deactivate(LV2_Handle instance)
{
	/* Nothing to do here in this trivial mostly stateless plugin. */
}

/** Destroy a plugin instance (counterpart to instantiate()). */
static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

/** Return extension data provided by the plugin. */
static const void*
extension_data(const char* uri)
{
	return NULL;  /* This plugin has no extension data. */
}

/** The LV2_Descriptor for this plugin. */
static const LV2_Descriptor descriptor = {
	GLAMP_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

/** Entry point, the host will call this function to access descriptors. */
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
