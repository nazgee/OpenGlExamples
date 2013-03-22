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
#include <android_native_app_glue.h>

#include <assert.h>
#include "texture.h"
#include "file.h"
#include "framebuffer.h"
#include "logger.h"

const int   TEXTURE_WIDTH   = 256;  // NOTE: texture size cannot be larger than
const int   TEXTURE_HEIGHT  = 256;  // the rendering window size in non-FBO mode

// -------------------------------------------

float scaleFactor = 1.0;
typedef struct
{
    float x;
    float y;
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
//	Log("ScaleFactor: %f",scaleFactor);
	for (i=0;i<6;i++)
	{
		gTriangleVerticesPNG[i].x = gTriangleVertices[i].x * scaleFactor - (1-scaleFactor) ;
		gTriangleVerticesPNG[i].y = gTriangleVertices[i].y * scaleFactor - (1-scaleFactor) ;
//		Log("X: %f Y: %f",gTriangleVerticesPNG[i].x,gTriangleVerticesPNG[i].y);
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

    struct framebuffer fb;
};

void draw()
{
    // clear buffer
    glClearColor(0, 1, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // draw a rotating teapot at the origin
    glUseProgram(gProgramHandle);

    // Enable vertex position
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

	glDisableVertexAttribArray(gaTexCoordHandle);
	glDisableVertexAttribArray(gaTexCoordHandle);
}

void grabFrambufferDataPointer(GLuint fboPointer,GLenum imageFormat,GLuint fbWidth,GLuint fbHeight,void* dataPointer) {
	if(dataPointer) {
		glBindFramebuffer(GL_FRAMEBUFFER, fboPointer);
		glReadPixels(0,0,fbWidth,fbHeight,GL_RGB,GL_UNSIGNED_BYTE,dataPointer);
	    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind
	}
	else
		LogError("Probably unallocated data Pointer has been passed");
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
        Log("Unable to eglMakeCurrent");
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
    Log("Program handle %d",gProgramHandle);
    if( !gProgramHandle )
    {
        LogError( "Could not create program." );
        assert(0);
    }
//
    // Read attribute locations from the program
    gaPositionHandle = glGetAttribLocation( gProgramHandle, "aPosition" );
    Log("gaPositionHandle %d",gaPositionHandle);
    CheckGlError( "glGetAttribLocation" );
//
    gaTexCoordHandle = glGetAttribLocation( gProgramHandle, "aTexCoord" );
    Log("gaTexCoordHandle %d",gaTexCoordHandle);
    CheckGlError( "glGetAttribLocation" );

    gaTexSamplerHandle = glGetUniformLocation( gProgramHandle, "sTexture" );
    Log("gaTexSamplerHandle %d",gaTexSamplerHandle);
    CheckGlError( "glGetUnitformLocation" );

    gTextureHandlePNG = LoadTexturePNG("cat.png");

    init_fbo(&engine->fb, w, h);
    glViewport(0,0,engine->width,engine->height);
    engine->animating = 1;


    renderToFBO(&engine->fb,draw);

    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
    	Log("No Display");
        return;
    }

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

    glBindTexture( GL_TEXTURE_2D, engine->fb.renderableTexture );

    glDrawArrays( GL_TRIANGLES, 0, 6 );

    glFlush();
    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
    	destroy_fbo(&engine->fb);
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
//			Log("Received key event: AKEYCODE_VOLUME_DOWN\n");
			scale(scaleFactor-0.05);
		}
		if(key_val == AKEYCODE_VOLUME_UP){
//			Log("Received key event: AKEYCODE_VOLUME_UP\n");
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
