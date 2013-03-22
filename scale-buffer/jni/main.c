/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <assert.h>
#include "texture.h"
#include "file.h"
#include "Teapot.h"
#include "matrices.h"

const int   TEXTURE_WIDTH   = 256;  // NOTE: texture size cannot be larger than
const int   TEXTURE_HEIGHT  = 256;  // the rendering window size in non-FBO mode

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define  Log(...)  __android_log_print( ANDROID_LOG_INFO, "TextureLoader", __VA_ARGS__ )
#define  LogError(...)  __android_log_print( ANDROID_LOG_ERROR, "TextureLoader", __VA_ARGS__ )

static void CheckGlError( const char* pFunctionName )
{
    GLint error = glGetError();
    if( error != GL_NO_ERROR )
    {
        LogError( "%s returned glError 0x%x\n", pFunctionName, error );
    }
}

// -------------------------------------------

float scaleFactor = 1.0;
typedef struct
{
    float x;
    float y;
//    float u;
//    float v;

} TriangleVertex;

TriangleVertex gTriangleVertices[] =     { { 1.0f,  1.0f },
                                            {   -1.0f, -1.0f },
                                            {   1.0f, -1.0f  },
                                            {   -1.0f,  1.0f },
                                            {   -1.0f, -1.0f },
                                            {   1.0f,  1.0f  } };

TriangleVertex gTriangleVerticesPNG[] =     { { 1.0f,  1.0f },
                                            {   -1.0f, -1.0f },
                                            {   1.0f, -1.0f  },
                                            {   -1.0f,  1.0f },
                                            {   -1.0f, -1.0f },
                                            {   1.0f,  1.0f  } };

TriangleVertex gTextureCoordsPNG[] =     { { 1.0f, 0.0f  },
                                            {   0.0f,  1.0f  },
                                            {    1.0f, 1.0f  },
                                            {    0.0f,  0.0f  },
                                            {    0.0f,  1.0f  },
                                            {    1.0f, 0.0f  } };


void scale(float factor)
{
	int i=0;
	scaleFactor=factor;
	if(scaleFactor>=1)
		scaleFactor = 1.0;
	if(scaleFactor<0.05)
		scaleFactor = 0.05;
	Log("ScaleFactor: %f",scaleFactor);
	for (i=0;i<6;i++)
	{
		gTriangleVerticesPNG[i].x = gTriangleVertices[i].x * scaleFactor - (1-scaleFactor) ;
		gTriangleVerticesPNG[i].y = gTriangleVertices[i].y * scaleFactor - (1-scaleFactor) ;
		Log("X: %f Y: %f",gTriangleVerticesPNG[i].x,gTriangleVerticesPNG[i].y);
	}
}

static const char gVertexShader[] =
    "attribute vec4 aPosition;  \n"
    "attribute vec2 aTexCoord;  \n"
    "varying vec2 vTexCoord;    \n"
    "void main()                \n"
    "{                          \n"
    "  vTexCoord = aTexCoord;   \n"
    "  gl_Position =  aPosition; \n"
    "}                          \n";

///////////////////////////////////////////////////////////////////////////////////////////////////
// Simple pixel (aka Fragment) shader
static const char gPixelShader[] =
    "precision mediump float;                          \n"
    "varying vec2 vTexCoord;                           \n"
    "uniform sampler2D sTexture;                       \n"
    "void main()                                       \n"
    "{                                                 \n"
    "  gl_FragColor = texture2D(sTexture, vTexCoord);  \n"
    "}                                                 \n";

///////////////////////////////////////////////////////////////////////////////////////////////////
// CompileShader - Compiles the passed in string for the given shaderType
GLuint CompileShader( GLenum shaderType, const char* pSource )
{
    // Create a shader handle
    GLuint shaderHandle = glCreateShader( shaderType );

    if( shaderHandle )
    {
        // Set the shader source
        glShaderSource( shaderHandle, 1, &pSource, NULL );

        // Compile the shader
        glCompileShader( shaderHandle );

        // Check the compile status
        GLint compileStatus = 0;
        glGetShaderiv( shaderHandle, GL_COMPILE_STATUS, &compileStatus );

        if( !compileStatus )
        {
            // There was an error, print it out
            GLint infoLogLength = 0;
            glGetShaderiv( shaderHandle, GL_INFO_LOG_LENGTH, &infoLogLength );

            if( infoLogLength > 1 )
            {
                char* pShaderInfoLog = (char*)malloc( infoLogLength );
                if( pShaderInfoLog )
                {
                    glGetShaderInfoLog( shaderHandle, infoLogLength, NULL, pShaderInfoLog );
                    LogError( "Error compiling shader: \n%s", pShaderInfoLog );
                    free( pShaderInfoLog );
                }

                // Free the handle
                glDeleteShader( shaderHandle );
                shaderHandle = 0;
            }
        }
    }

    return shaderHandle;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// createProgram - Creates a new program with the given vertex and pixel shader
GLuint createProgram( const char* pVertexSource, const char* pPixelSource )
{
    // Compile the vertex shader
    GLuint vertexShaderHandle = CompileShader( GL_VERTEX_SHADER, pVertexSource );
    GLuint pixelShaderHandle  = CompileShader( GL_FRAGMENT_SHADER, pPixelSource );

    if( !vertexShaderHandle || !pixelShaderHandle )
    {
        return 0;
    }

    // Create a new handle for this program
    GLuint programHandle = glCreateProgram();

    // Link and finish the program
    if( programHandle )
    {
        // Set the vertex shader for this program
        glAttachShader( programHandle, vertexShaderHandle );
        CheckGlError( "glAttachShader" );

        // Set the pixel shader for this program
        glAttachShader( programHandle, pixelShaderHandle );
        CheckGlError( "glAttachShader" );

        // Link the program
        glLinkProgram( programHandle );

        // Check the link status
        GLint linkStatus = 0;
        glGetProgramiv( programHandle, GL_LINK_STATUS, &linkStatus );

        if( !linkStatus )
        {
            // There was an error, print it out
            GLint infoLogLength = 0;
            glGetProgramiv( programHandle, GL_INFO_LOG_LENGTH, &infoLogLength );

            if( infoLogLength )
            {
                char* pInfoLog = (char*)malloc( infoLogLength );
                if( pInfoLog )
                {
                    glGetProgramInfoLog( programHandle, infoLogLength, NULL, pInfoLog );
                    LogError( "Error linking the program: \n%s", pInfoLog );
                    free( pInfoLog );
                }
            }

            // Free the handle
            glDeleteProgram( programHandle );
            programHandle = 0;
        }
    }

    return programHandle;
}


GLuint gProgramHandle;
GLuint gaPositionHandle;
GLuint gaTexCoordHandle;
GLuint gaTexSamplerHandle;
GLuint gTextureHandlePNG;

/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    float rotationDirection;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;

    GLuint renderableTexture,framebufferObject;
};

void drawTextureFBO(struct engine* engine) {
int i=0;
    glBindFramebuffer(GL_FRAMEBUFFER, engine->framebufferObject);

    // clear buffer
    glClearColor(1, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,2*engine->width,2*engine->height);
    // draw a rotating teapot at the origin
    glUseProgram(gProgramHandle);

    glEnableVertexAttribArray( gaPositionHandle );
	CheckGlError( "glEnableVertexAttribArray" );

	// Enable tex coords
	glEnableVertexAttribArray( gaTexCoordHandle );
	CheckGlError( "glEnableVertexAttribArray" );

	// Set texture sampler
	glActiveTexture( GL_TEXTURE0 );

	// Enable texture sampler
	glUniform1i( gaTexSamplerHandle, 0 );

    glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTriangleVertices );
    CheckGlError( "glVertexAttribPointer" );

    glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTextureCoordsPNG );
    CheckGlError( "glVertexAttribPointer" );

    // Set active texture
    glBindTexture( GL_TEXTURE_2D, gTextureHandlePNG );

    glDrawArrays( GL_TRIANGLES, 0, 6 );
	CheckGlError("ImageTargets renderFrame");

//    GLubyte* pixels = (GLubyte*) malloc(3*2*engine->width * 3*2*engine->height * sizeof(GLubyte) );
//    glReadPixels(0,0,2*engine->width,2*engine->height,GL_RGB,GL_UNSIGNED_BYTE,pixels);
//    for(i=0;i<3*2*engine->width * 3*2*engine->height * sizeof(GLubyte);i++)
//    {
//    	LOGI("Pixel[%d] = %d",i,pixels[i]);
//    }

    // back to normal window-system-provided framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind

    // trigger mipmaps generation explicitly
    // NOTE: If GL_GENERATE_MIPMAP is set to GL_TRUE, then glCopyTexSubImage2D()
    // triggers mipmap generation automatically. However, the texture attached
    // onto a FBO should generate mipmaps manually via glGenerateMipmap().
    glBindTexture(GL_TEXTURE_2D, engine->renderableTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glViewport(0,0,engine->width,engine->height);
}

/**
 * Initializes Renderbuffer.
 */
static GLuint init_renderbuffer(GLuint width, GLuint height, GLenum format) {
    GLuint renderbuffer;

    glGenRenderbuffers(1, &renderbuffer);
    CheckGlError("init_renderbuffer: glGenRenderbuffers");

    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    CheckGlError("init_renderbuffer: glBindRenderbuffer");

    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    CheckGlError("init_renderbuffer: glRenderbufferStorage");

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    CheckGlError("init_renderbuffer: glBindRenderbuffer");

    return renderbuffer;
}

/**
 * Initializes FBO.
 */
static void engine_init_fbo(struct engine* engine, GLuint width, GLuint height) {
    // create renderable texture
    glGenTextures(1, &engine->renderableTexture);
    CheckGlError("engine_init_fbo: glGenTextures");

    glBindTexture(GL_TEXTURE_2D, engine->renderableTexture);
    CheckGlError("engine_init_fbo: glBindTexture");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    CheckGlError("engine_init_fbo: glTexImage2D");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckGlError("engine_init_fbo: glTexParameteri");

    LOGI("****************************** engine_init_fbo: texture ID: %d",
         engine->renderableTexture);

    // create framebuffer object
    glGenFramebuffers(1, &engine->framebufferObject);
    CheckGlError("engine_init_fbo: glGenFramebuffers");

    glBindFramebuffer(GL_FRAMEBUFFER, engine->framebufferObject);
    CheckGlError("engine_init_fbo: glBindFramebuffer");

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, engine->renderableTexture, 0);
    CheckGlError("engine_init_fbo: glFramebufferTexture2D");

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    CheckGlError("engine_init_fbo: glCheckFramebufferStatus");

    if(status != GL_FRAMEBUFFER_COMPLETE) {
        switch(status) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                LOGI("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                LOGI("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                LOGI("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
                break;

            case GL_FRAMEBUFFER_UNSUPPORTED:
                LOGI("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_UNSUPPORTED");
                break;

            default:
                LOGI("****************************** engine_init_fbo: Unknown FBO error");
        }
    }
    else {
        LOGI("****************************** engine_init_fbo: FBO has been successfully initialized");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGlError("engine_init_fbo: glBindTexture");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckGlError("engine_init_fbo: glBindFramebuffer");
}


/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    const EGLint context_attrib_list [] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, context_attrib_list);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    // Initialize GL state.
//    glHint(GL_PEr, GL_FASTEST);
    glEnable(GL_CULL_FACE);
//    glShadeModel(GL_SH);
    glDisable(GL_DEPTH_TEST);

    // Init the shaders
    gProgramHandle = createProgram( gVertexShader, gPixelShader );
    LOGI("Program handle %d",gProgramHandle);
    if( !gProgramHandle )
    {
        LogError( "Could not create program." );
        assert(0);
    }
//
    // Read attribute locations from the program
    gaPositionHandle = glGetAttribLocation( gProgramHandle, "aPosition" );
    LOGI("gaPositionHandle %d",gaPositionHandle);
    CheckGlError( "glGetAttribLocation" );
//
    gaTexCoordHandle = glGetAttribLocation( gProgramHandle, "aTexCoord" );
    LOGI("gaTexCoordHandle %d",gaTexCoordHandle);
    CheckGlError( "glGetAttribLocation" );

    gaTexSamplerHandle = glGetUniformLocation( gProgramHandle, "sTexture" );
    LOGI("gaTexSamplerHandle %d",gaTexSamplerHandle);
    CheckGlError( "glGetUnitformLocation" );

    gTextureHandlePNG = LoadTexturePNG("cat.png");

    engine_init_fbo(engine, 2*w, 2*h);
    glViewport(0,0,engine->width,engine->height);
    engine->animating = 1;
    engine->state.rotationDirection = 1;
    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
    	LOGI("No Display");
        return;
    }

    drawTextureFBO(engine);

    glClearColor( 0.8f, 0.7f, 0.6f, 1.0f);
    CheckGlError( "glClearColor" );

    // Clear the color and depth buffer
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    CheckGlError( "glClear" );

    // Select vertex/pixel shader
    glUseProgram( gProgramHandle );
    CheckGlError( "glUseProgram" );


    CheckGlError( "glUniformMatrix4fv" );
    // Enable vertex
    glEnableVertexAttribArray( gaPositionHandle );
    CheckGlError( "glEnableVertexAttribArray" );

    // Enable tex coords
    glEnableVertexAttribArray( gaTexCoordHandle );
    CheckGlError( "glEnableVertexAttribArray" );

    // Set texture sampler
    glActiveTexture( GL_TEXTURE0 );

    // Enable texture sampler
    glUniform1i( gaTexSamplerHandle, 0 );

    // PNG //////////////////////////////////////////////////////////////////////////////////////////////////////

    // Set vertex position
    glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTriangleVerticesPNG );
    CheckGlError( "glVertexAttribPointer" );

//     Set vertex texture coordinates
    glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTextureCoordsPNG );
    CheckGlError( "glVertexAttribPointer" );

    glBindTexture( GL_TEXTURE_2D, engine->renderableTexture );

    glDrawArrays( GL_TRIANGLES, 0, 6 );

    glFlush();
    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        glDeleteFramebuffers(1, &engine->framebufferObject);
        glDeleteTextures(1, &engine->renderableTexture);
        engine->framebufferObject = 0;
        engine->renderableTexture = 0;
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
    {
		int key_val = AKeyEvent_getKeyCode(event);
	
		if(key_val == AKEYCODE_VOLUME_DOWN){
			LOGI("Received key event: AKEYCODE_VOLUME_DOWN\n");
			scale(scaleFactor-0.05);
		}
		if(key_val == AKEYCODE_VOLUME_UP){
			LOGI("Received key event: AKEYCODE_VOLUME_UP\n");
			scale(scaleFactor+0.05);
		}
		return 0;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:

            break;
        case APP_CMD_LOST_FOCUS:

            // Also stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;
    SetAssetManager(engine.app->activity->assetManager);
    // Prepare to monitor accelerometer

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.

    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }
//        engine_draw_frame(&engine);
        if (engine.animating) {
            engine_draw_frame(&engine);
        }
    }
}
//END_INCLUDE(all)
