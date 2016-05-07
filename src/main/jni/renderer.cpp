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
#include "glulookat.h"

#define LOG_TAG "EglSample"
#define _RES 40
static bool ortho = true;
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
static GLfloat vertices[][3] = {
    { -4.0, -4.0, -4.0 },
    {  4.0, -4.0, -4.0 },
    {  4.0,  4.0, -4.0 },
    { -4.0,  4.0, -4.0 },
    { -4.0, -4.0,  4.0 },
    {  4.0, -4.0,  4.0 },
    {  4.0,  4.0,  4.0 },
    { -4.0,  4.0,  4.0 }
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
    : _msg(MSG_NONE), _display(0), _surface(0), _context(0), _angle(0),dX(0),dY(0),_zoom(1),zoomchanged(false),isOrtho(ortho)
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

void Renderer::setPan(float X,float Y)
{
    dX = X;
    dY = Y;
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

void Renderer::setProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // glFrustumf(-ratio, ratio, -ratio, ratio, -ratio, ratio);
    //glFrustumf(-ratio, ratio, -1, 1, 1, 10);

    //glFrustumf(-RES, RES, -RES, RES, -RES, RES);
    //glFrustumf(-RES*2,RES*2,-RES*2,RES*2,-RES*2,RES*2);


    //glOrthof(-RES*2,RES*2,-RES*2,RES*2,-RES*2,RES*2);
    GLfloat zNear,zFar,left,right,top,bottom;
    GLfloat cx=0.0,cy=0.0,cz=0.0;
    zNear=-1.0f;
    zFar = zNear + RES;
    left = cx - RES;
    right = cx + RES;
    bottom = cy - RES;
    top = cy + RES;
    GLfloat aspect = (GLfloat) _width / _height;

    if ( aspect < 1.0 ) { // window taller than wide
        bottom /= aspect;
        top /= aspect;
    } else {
        left *= aspect;
        right *= aspect;
    }
    if(isOrtho)
        glOrthof(left*_zoom, right*_zoom, bottom*_zoom, top*_zoom, zNear, zFar);
    else
        glFrustumf(left*_zoom, right*_zoom, bottom*_zoom, top*_zoom, zNear, zFar);

    LOG_INFO("left:%f, right:%f, bottom:%f, top:%f, zNear:%f, zFar:%f",(float)left, (float)right, (float)bottom, (float)top, (float)zNear, (float)zFar);

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
    _width = width;
    _height = height;
    setProjection();
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

    //glLineWidth(5);

    for(int i=0;i<=RES*2;i++)
    {
        glTranslatef(0,1,0);
        glVertexPointer(3,GL_FLOAT,0,line_vertex);
        //glColorPointer(4, GL_FIXED, 0, colors);
        glDrawArrays(GL_LINES,0,2);
    }
    glTranslatef(0,-RES*2,0);/**/
    glRotatef(-90,0,0,1);
    glTranslatef(0,-1,0);
    //glColor4f(1,0,0,1);
    for(int i=0;i<=RES*2;i++)
    {
        glTranslatef(0,1,0);
        //glVertexPointer(3,GL_FLOAT,0,line_vertex);
        //glColorPointer(4, GL_FIXED, 0, colors);
        glDrawArrays(GL_LINES,0,2);
    }
    glTranslatef(0,-RES*2,0);
}
void Renderer::drawPoints()
{
    int k=0,l=0;
    for(int i = RES;i >= - RES;i--)
    {
        for(int j = -RES;j <= RES;j++)
        {
            pntVertex[k][l++] = j;
            pntVertex[k][l++] = i;
        }
        k++;l=0;
    }
    glPointSize(8);
    //glVertexPointer(3,GL_FLOAT,0,point_vertex2);
    //glVertexPointer(3,GL_FLOAT,0,point_corners);
    glVertexPointer(2,GL_FLOAT,0,pntVertex);
    glDrawArrays(GL_POINTS,0,(RES*RES*4));
}

void Renderer::drawCube()
{
    glEnableClientState(GL_COLOR_ARRAY);
    glTranslatef(0, 0, -3.0f);
    //glRotatef(_angle, 0, 1, 0);
    //glRotatef(_angle*0.25f, 1, 0, 0);
    //glFrontFace(GL_CW);
    //glColor4f(1,0,0,0);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FIXED, 0, colors);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);

    _angle += 1.2f;
}

void Renderer::doPanning()
{
    GLfloat actualResX,actualResY;
    GLfloat aspect = (GLfloat) (_width / _height) ;
    if(aspect < 1.0)
    {
        actualResX = RES*2/_width;
        actualResY = (RES*2) /aspect/_height;
    }
    else
    {
        actualResY = RES*2/_height;
        actualResX = ((RES*2) * aspect) /_width;
    }
    glTranslatef(dX*actualResX*_zoom,-dY*actualResY*_zoom,0);
}
void Renderer::setZoom(float z) {
 _zoom *= z;
    zoomchanged = true;
}
void Renderer::doZooming()
{
    //glScalef(_zoom,_zoom,_zoom);
    if(zoomchanged == true)
        setProjection();
    zoomchanged = false;
}
void Renderer::drawFrame()
{
    doZooming();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    doPanning();

    //LOG_INFO("dX,dY: %f,%f",dX,dY);
    //glRotatef(dX,1,0,0);
    //glRotatef(dY,0,1,0);
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX,mv);
    //view_set_lookat(mv,0.0,0.0,5.0,0.0,0.0,0.0,0.0,1.0,0.0);
    glLoadMatrixf(mv);
    //glRotatef(45,1,0,0);
    //glRotatef(45,0,1,0);
    //glTranslatef(0.0, 0.0, 10.0f);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4f(1,1,1,1);
    glEnableClientState(GL_VERTEX_ARRAY);
    //drawPoints();
    drawLines();
    drawCube();
}

void* Renderer::threadStartCallback(void *myself)
{
    Renderer *renderer = (Renderer*)myself;

    renderer->renderLoop();
    pthread_exit(0);
    
    return 0;
}

