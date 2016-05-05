//
// Copyright 2011 Tero Saarni
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <android/native_window.h> // requires ndk r5 or newer
#include <EGL/egl.h> // requires ndk r5 or newer
#include <GLES/gl.h>

#include "logger.h"
#include "renderer.h"

#define LOG_TAG "EglSample"
#define _RES 20
static const GLfloat RES=_RES;
GLfloat pntVertex[_RES*2*2][_RES*2*2];
GLfloat lineVertex[_RES*2][2];
static GLfloat line_vertex[][3]= {
        {-RES,-RES,0},
        {RES,-RES,0}
};
static GLfloat point_vertex2[][3]= {
        {-1.0,1.0,0},
        {0.0,1.0,0},
        {1.0,1.0,0},
        {-1.0,0.0,0},
        {0.0,0.0,0},
        {1.0,0.0,0},
        {-1.0,-1.0,0},
        {0.0,-1.0,0},
        {1.0,-1.0,0},
};
static GLfloat  point_corners[][3]= {
        {-RES,RES,0.0},
        {-RES,-RES,0.0},
        {RES,RES,0.0},
        {RES,-RES,0.0},
};
static GLint vertices[][3] = {
    { -0x50000, -0x50000, -0x50000 },
    {  0x50000, -0x50000, -0x50000 },
    {  0x50000,  0x50000, -0x50000 },
    { -0x50000,  0x50000, -0x50000 },
    { -0x50000, -0x50000,  0x50000 },
    {  0x50000, -0x50000,  0x50000 },
    {  0x50000,  0x50000,  0x50000 },
    { -0x50000,  0x50000,  0x50000 }
};

static GLint colors[][4] = {
    { 0x00000, 0x00000, 0x00000, 0x10000 },
    { 0x10000, 0x00000, 0x00000, 0x10000 },
    { 0x10000, 0x10000, 0x00000, 0x10000 },
    { 0x00000, 0x10000, 0x00000, 0x10000 },
    { 0x00000, 0x00000, 0x10000, 0x10000 },
    { 0x10000, 0x00000, 0x10000, 0x10000 },
    { 0x10000, 0x10000, 0x10000, 0x10000 },
    { 0x00000, 0x10000, 0x10000, 0x10000 }
};

GLubyte indices[] = {
    0, 4, 5,    0, 5, 1,
    1, 5, 6,    1, 6, 2,
    2, 6, 7,    2, 7, 3,
    3, 7, 4,    3, 4, 0,
    4, 7, 6,    4, 6, 5,
    3, 0, 1,    3, 1, 2
};


Renderer::Renderer()
    : _msg(MSG_NONE), _display(0), _surface(0), _context(0), _angle(0)
{
    LOG_INFO("Renderer instance created");
    pthread_mutex_init(&_mutex, 0);    
    return;
}

Renderer::~Renderer()
{
    LOG_INFO("Renderer instance destroyed");
    pthread_mutex_destroy(&_mutex);
    return;
}

void Renderer::start()
{
    LOG_INFO("Creating renderer thread");
    pthread_create(&_threadId, 0, threadStartCallback, this);
    return;
}

void Renderer::stop()
{
    LOG_INFO("Stopping renderer thread");

    // send message to render thread to stop rendering
    pthread_mutex_lock(&_mutex);
    _msg = MSG_RENDER_LOOP_EXIT;
    pthread_mutex_unlock(&_mutex);    

    pthread_join(_threadId, 0);
    LOG_INFO("Renderer thread stopped");

    return;
}

void Renderer::setWindow(ANativeWindow *window)
{
    // notify render thread that window has changed
    pthread_mutex_lock(&_mutex);
    _msg = MSG_WINDOW_SET;
    _window = window;
    pthread_mutex_unlock(&_mutex);

    return;
}



void Renderer::renderLoop()
{
    bool renderingEnabled = true;
    
    LOG_INFO("renderLoop()");

    while (renderingEnabled) {

        pthread_mutex_lock(&_mutex);

        // process incoming messages
        switch (_msg) {

            case MSG_WINDOW_SET:
                initialize();
                break;

            case MSG_RENDER_LOOP_EXIT:
                renderingEnabled = false;
                destroy();
                break;

            default:
                break;
        }
        _msg = MSG_NONE;
        
        if (_display) {
            drawFrame();
            if (!eglSwapBuffers(_display, _surface)) {
                LOG_ERROR("eglSwapBuffers() returned error %d", eglGetError());
            }
        }
        
        pthread_mutex_unlock(&_mutex);
    }
    
    LOG_INFO("Render loop exits");
    
    return;
}

bool Renderer::initialize()
{
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };
    EGLDisplay display;
    EGLConfig config;    
    EGLint numConfigs;
    EGLint format;
    EGLSurface surface;
    EGLContext context;
    EGLint width;
    EGLint height;
    GLfloat ratio;
    
    LOG_INFO("Initializing context");
    
    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOG_ERROR("eglGetDisplay() returned error %d", eglGetError());
        return false;
    }
    if (!eglInitialize(display, 0, 0)) {
        LOG_ERROR("eglInitialize() returned error %d", eglGetError());
        return false;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        LOG_ERROR("eglChooseConfig() returned error %d", eglGetError());
        destroy();
        return false;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
        destroy();
        return false;
    }

    ANativeWindow_setBuffersGeometry(_window, 0, 0, format);

    if (!(surface = eglCreateWindowSurface(display, config, _window, 0))) {
        LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
        destroy();
        return false;
    }
    
    if (!(context = eglCreateContext(display, config, 0, 0))) {
        LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
        destroy();
        return false;
    }
    
    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
        destroy();
        return false;
    }

    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
        destroy();
        return false;
    }

    _display = display;
    _surface = surface;
    _context = context;

    glDisable(GL_DITHER);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glClearColor(0, 0, 0, 0);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    
    glViewport(0, 0, width, height);

    ratio = (GLfloat) width / height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glFrustumf(-ratio, ratio, -ratio, ratio, -ratio*2, ratio*2);
    //glFrustumf(-RES, RES, -RES, RES, -RES, RES);


    glOrthof(-RES,RES,-RES,RES,-RES,RES);

    return true;
}

void Renderer::destroy() {
    LOG_INFO("Destroying context");

    eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(_display, _context);
    eglDestroySurface(_display, _surface);
    eglTerminate(_display);
    
    _display = EGL_NO_DISPLAY;
    _surface = EGL_NO_SURFACE;
    _context = EGL_NO_CONTEXT;

    return;
}
/*
 * {-RES,RES,0.0},
        {-RES,-RES,0.0},
        {RES,RES,0.0},
        {RES,-RES,0.0},
 * */
void Renderer::drawLines()
{
    int k=0,l=0;
    for(int i=RES;i >= -RES ;i--)
    {
        for(int j= -RES;j<=RES; j++)
        {
            //lineVertex[k][l++]=
        }
    }
}
void Renderer::drawPoints()
{
    int k=0,l=0;
    for(int i = RES;i >= - RES;i--)
    {
        for(int j = -RES;j <= RES;j++)
        {
            pntVertex[k][l++] = j++;
            pntVertex[k][l++] = i;
        }
        k++;l=0;
    }
    glColor4f(1,1,1,1);
    glPointSize(8);
    //glVertexPointer(3,GL_FLOAT,0,point_vertex2);
    //glVertexPointer(3,GL_FLOAT,0,point_corners);
    glVertexPointer(2,GL_FLOAT,0,pntVertex);
    glDrawArrays(GL_POINTS,0,RES*RES*4);
}
void Renderer::drawFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //glRotatef(45,1,0,0);
    //glRotatef(45,0,1,0);
    //glTranslatef(0.0, 0.0, 0.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    //drawPoints();

    glColor4f(1,1,1,1);
    //glTranslatef(0, 0, -3.0f);
    //glLineWidth(5);

    for(int i=0;i<RES*2;i++)
    {
        glTranslatef(0,1,0);
        glVertexPointer(3,GL_FLOAT,0,line_vertex);
        //glColorPointer(4, GL_FIXED, 0, colors);
        glDrawArrays(GL_LINES,0,2);
    }
    glTranslatef(0,-RES*2,0);/**/
    glRotatef(90,0,0,1);
    for(int i=0;i<RES*2;i++)
    {
        glTranslatef(0,1,0);
        glVertexPointer(3,GL_FLOAT,0,line_vertex);
        //glColorPointer(4, GL_FIXED, 0, colors);
        glDrawArrays(GL_LINES,0,2);
    }
    /**/
    //glTranslatef(-20,0,0);// bring back to 0,0,0
    // cube

    //glTranslatef(0, 0, -3.0f);
    //glRotatef(_angle, 0, 1, 0);
    //glRotatef(_angle*0.25f, 1, 0, 0);

/*
    
    //glFrontFace(GL_CW);
    //glColor4f(1,0,0,0);
    glVertexPointer(3, GL_FIXED, 0, vertices);
    glColorPointer(4, GL_FIXED, 0, colors);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);

    _angle += 1.2f;
     /**/
}

void* Renderer::threadStartCallback(void *myself)
{
    Renderer *renderer = (Renderer*)myself;

    renderer->renderLoop();
    pthread_exit(0);
    
    return 0;
}

