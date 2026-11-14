#include "ui_window_win32.h"
#include <GL/gl.h>

struct ui_gpu {
    ui_window* window; /* Borrowed; must outlive the context. */
    HGLRC context;
    GLuint texture;
    size_t texture_width, texture_height;
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

    /* This context is hardware-only; the common renderer owns CPU fallback. */
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
            if (gpu->texture) {
                glDeleteTextures(1, &gpu->texture);
            }

            wglMakeCurrent(NULL, NULL);
        }

        wglDeleteContext(gpu->context);
    }

    HeapFree(GetProcessHeap(), 0, gpu);
}

bool ui_gpu_present_framebuffer(ui_gpu* gpu, const ui_framebuffer* framebuffer) {
    if (!wglMakeCurrent(gpu->window->dc, gpu->context)) {
        return false;
    }

    GLint maximum;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
    size_t width = 1, height = 1;
    while (width < framebuffer->width) {
        width *= 2;
    }

    while (height < framebuffer->height) {
        height *= 2;
    }

    if (width > (size_t)maximum || height > (size_t)maximum) {
        return false;
    }

    if (!gpu->texture) {
        glGenTextures(1, &gpu->texture);
    }

    glBindTexture(GL_TEXTURE_2D, gpu->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    /* BGRA matches the shared top-down BGRX canvas. Unsupported uploads fail
     * cleanly into the CPU presenter; no driver-specific application code. */
    if (gpu->texture_width != width || gpu->texture_height != height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height,
                     0, 0x80E1 /* GL_BGRA */, GL_UNSIGNED_BYTE, NULL);
        gpu->texture_width = width;
        gpu->texture_height = height;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)framebuffer->width,
                   (GLsizei)framebuffer->height, 0x80E1, GL_UNSIGNED_BYTE,
                   framebuffer->pixels);
    glViewport(0, 0, (GLsizei)framebuffer->width, (GLsizei)framebuffer->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
    f32 right = (f32)framebuffer->width / (f32)width;
    f32 bottom = (f32)framebuffer->height / (f32)height;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(right, 0);
    glVertex2f(1, 0);
    glTexCoord2f(right, bottom);
    glVertex2f(1, 1);
    glTexCoord2f(0, bottom);
    glVertex2f(0, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    if (glGetError() != GL_NO_ERROR) {
        return false;
    }

    return SwapBuffers(gpu->window->dc) != 0;
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
