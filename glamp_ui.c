/*
  Copyright 2012-2017 David Robillard <d@drobilla.net>

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
   @file glamp_ui.c Glamp.LV2 Plugin UI
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include <GLFW/glfw3.h>

#define GLAMP_UI_URI "http://drobilla.net/plugins/glamp#ui"

typedef struct {
	GLFWwindow*          view;
	LV2UI_Write_Function write;
	LV2UI_Controller     controller;
	int                  width;
	int                  height;
	float                brightness;
	bool                 exit;
} GlampUI;

static void
onReshape(GLFWwindow* view, int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, width, height);
	glOrtho(0, width, height, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	glLoadIdentity();
}

static void
onDisplay(GLFWwindow* view)
{
	GlampUI* self = (GlampUI*)glfwGetWindowUserPointer(view);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_QUADS);

	GLdouble x2 = 2.0;
	GLdouble y2 = 2.0;
	GLdouble x1 = self->width - 2.0;
	GLdouble y1 = self->height - 2.0;
	glColor3f(1.0f * self->brightness, 0.2f * self->brightness, 0.0f);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glColor3f(0.0f, 1.0f * self->brightness, 0.0f);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);

	glEnd();

	glfwSwapBuffers(view);
}

static void
onKeyboard(GLFWwindow* view, bool press, uint32_t key)
{
	if (press) {
		fprintf(stderr, "Keyboard press %c\n", key);
	} else {
		fprintf(stderr, "Keyboard release %c\n", key);
	}
}

static void
onMotion(GLFWwindow* view, int x, int y)
{
	fprintf(stderr, "Motion: %d,%d\n", x, y);
}

static void
onMouse(GLFWwindow* view, int button, bool press, int x, int y)
{
	fprintf(stderr, "Mouse %d %s at %d,%d\n",
	        button, press ? "down" : "up", x, y);
}

static void
onScroll(GLFWwindow* view, float dx, float dy)
{
	fprintf(stderr, "Scroll %f %f\n", dx, dy);
}

static void
onError(int error, const char* description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor*   descriptor,
            const char*               plugin_uri,
            const char*               bundle_path,
            LV2UI_Write_Function      write_function,
            LV2UI_Controller          controller,
            LV2UI_Widget*             widget,
            const LV2_Feature* const* features)
{
	GlampUI* self = (GlampUI*)calloc(1, sizeof(GlampUI));
	if (!self) {
		return NULL;
	}

	self->write      = write_function;
	self->controller = controller;
	self->width      = 256;
	self->height     = 32;
	self->exit       = false;

	// Get parent window and resize API from features
	void*         parent = 0;
	LV2UI_Resize* resize = NULL;
	for (int i = 0; features && features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_UI__parent)) {
			parent = features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_UI__resize)) {
			resize = (LV2UI_Resize*)features[i]->data;
		}
	}

	if (!parent) {
		fprintf(stderr, "error: glamp_ui: No parent window provided\n");
		free(self);
		return NULL;
	}

	glfwSetErrorCallback(onError);

	if (!glfwInit()) {
		fprintf(stderr, "error: glamp_ui: Failed to initialize GLFW\n");
		free(self);
		return NULL;
	}

	glfwWindowHintPointer(GLFW_NATIVE_PARENT_HANDLE, parent);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    self->view = glfwCreateWindow(640, 480, "Glamp", NULL, NULL);
    if (!self->view) {
		fprintf(stderr, "error: glamp_ui: Failed to create window\n");
	    free(self);
	    return NULL;
    }

	/* self->view = puglCreate(parent, "Glamp", self->width, self->height, true); */
    glfwSetWindowUserPointer(self->view, self);
	glfwSetWindowRefreshCallback(self->view, onDisplay);
    glfwSetWindowSizeCallback(self->view, onReshape);
	/* puglSetKeyboardFunc(self->view, onKeyboard); */
	/* puglSetMotionFunc(self->view, onMotion); */
	/* puglSetMouseFunc(self->view, onMouse); */
	/* puglSetScrollFunc(self->view, onScroll); */

    glfwMakeContextCurrent(self->view);
    glfwSwapInterval(1);

	if (resize) {
		resize->ui_resize(resize->handle, self->width, self->height);
	}

    glfwMakeContextCurrent(self->view);
    onReshape(self->view, 640, 480);

    *widget = glfwGetWindowNativeHandle(self->view);
	return self;
}

static int
idle(LV2UI_Handle handle)
{
	GlampUI* self = (GlampUI*)handle;

	// Silly pulsing animation to check that idle handler is working
	self->brightness = fmod(self->brightness + 0.01, 1.0);
	glfwMakeContextCurrent(self->view);
	onDisplay(self->view);
	glfwPollEvents();
	/* puglPostRedisplay(self->view); */
	/* puglProcessEvents(self->view); */

	return 0;
}

static void
cleanup(LV2UI_Handle handle)
{
	GlampUI* self = (GlampUI*)handle;
	self->exit = true;
	glfwDestroyWindow(self->view);
	free(self);
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	//GlampUI* self = (GlampUI*)handle;
}

static const LV2UI_Idle_Interface idle_iface = { idle };

static const void*
extension_data(const char* uri)
{
	if (!strcmp(uri, LV2_UI__idleInterface)) {
		return &idle_iface;
	}
	return NULL;
}

static const LV2UI_Descriptor descriptor = {
	GLAMP_UI_URI,
	instantiate,
	cleanup,
	port_event,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
