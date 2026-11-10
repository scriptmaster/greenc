#include "ui_window_win32.h"
#include <GL/gl.h>

struct ui_gpu {
    ui_window* window; /* Borrowed; must outlive the context. */
    HGLRC context;
};

ui_gpu* ui_gpu_create(ui_window* window) {
    ui_gpu* gpu = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*gpu));
    if (!gpu) {
        return NULL;
    }

    gpu->window = window;

    PIXELFORMATDESCRIPTOR requested = {0}, actual = {0};
    requested.nSize = sizeof(requested);
    requested.nVersion = 1;
    requested.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    requested.iPixelType = PFD_TYPE_RGBA;
    requested.cColorBits = 24;
    requested.iLayerType = PFD_MAIN_PLANE;

    int format = ChoosePixelFormat(window->dc, &requested);
    if (!format || !DescribePixelFormat(window->dc, format, sizeof(actual), &actual)) {
        goto failure;
    }

    /* Reject the generic software renderer for this GPU example. */
    if ((actual.dwFlags & PFD_GENERIC_FORMAT) ||
        (actual.dwFlags & requested.dwFlags) != requested.dwFlags) {
        goto failure;
    }

    if (!SetPixelFormat(window->dc, format, &actual)) {
        goto failure;
    }

    gpu->context = wglCreateContext(window->dc);
    if (!gpu->context || !wglMakeCurrent(window->dc, gpu->context)) {
        goto failure;
    }

    return gpu;

failure:
    ui_gpu_destroy(gpu);

    return NULL;
}

void ui_gpu_destroy(ui_gpu* gpu) {
    if (!gpu) {
        return;
    }

    if (gpu->context) {
        if (wglGetCurrentContext() == gpu->context) {
            wglMakeCurrent(NULL, NULL);
        }

        wglDeleteContext(gpu->context);
    }

    HeapFree(GetProcessHeap(), 0, gpu);
}

bool ui_gpu_draw_triangle(ui_gpu* gpu, const ui_vertex2 vertices[3],
                          f32 angle_degrees, ui_color background) {
    RECT client;
    if (!GetClientRect(gpu->window->handle, &client)) {
        return false;
    }

    if (!client.right || !client.bottom) {
        return true;
    }

    if (!wglMakeCurrent(gpu->window->dc, gpu->context)) {
        return false;
    }

    glViewport(0, 0, client.right, client.bottom);
    glClearColor(background.r, background.g, background.b, background.a);
    glClear(GL_COLOR_BUFFER_BIT);

    f32 aspect = (f32)client.right / (f32)client.bottom;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    f32 horizontal = aspect >= 1.0f ? aspect : 1.0f;
    f32 vertical = aspect >= 1.0f ? 1.0f : 1.0f / aspect;
    glOrtho(-horizontal, horizontal, -vertical, vertical, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(angle_degrees, 0, 0, 1);

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < 3; ++i) {
        glColor4f(vertices[i].color.r, vertices[i].color.g,
                  vertices[i].color.b, vertices[i].color.a);
        glVertex2f(vertices[i].x, vertices[i].y);
    }

    glEnd();

    if (glGetError() != GL_NO_ERROR) {
        return false;
    }

    return SwapBuffers(gpu->window->dc) != 0;
}
