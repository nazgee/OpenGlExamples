/*
 * framebuffer.c
 *
 *  Created on: 22-03-2013
 *      Author: gozdzseb
 */
#include "framebuffer.h"
#include "logger.h"
#include <stdlib.h>

TriangleVertex uniformQuadVertices[] =     { { 1.0f,  1.0f },
											{   -1.0f, -1.0f },
											{   1.0f, -1.0f  },
											{   -1.0f,  1.0f },
											{   -1.0f, -1.0f },
											{   1.0f,  1.0f  } };

TriangleVertex textureCoordinates[] =     { { 1.0f, 0.0f  },
											{   0.0f,  1.0f  },
											{    1.0f, 1.0f  },
											{    0.0f,  0.0f  },
											{    0.0f,  1.0f  },
											{    1.0f, 0.0f  } };
/**
 * Initializes Renderbuffer.
 */
GLuint init_renderbuffer(GLuint width, GLuint height, GLenum format) {
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
void init_fbo(struct framebuffer* fb,GLvoid* pixels, GLuint width, GLuint height) {
    // create renderable texture
	initTexture(&fb->renderableTexture,GL_RGB,GL_UNSIGNED_BYTE,width,height,pixels);

    // create framebuffer object
    glGenFramebuffers(1, &fb->framebufferObject);
    CheckGlError("engine_init_fbo: glGenFramebuffers");

    glBindFramebuffer(GL_FRAMEBUFFER, fb->framebufferObject);
    CheckGlError("engine_init_fbo: glBindFramebuffer");

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->renderableTexture, 0);
    CheckGlError("engine_init_fbo: glFramebufferTexture2D");

    checkFBOStatus();

    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGlError("engine_init_fbo: glBindTexture");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckGlError("engine_init_fbo: glBindFramebuffer");

	fb->width = width;
	fb->height = height;
}

void destroy_fbo(struct framebuffer* fb) {
    glDeleteFramebuffers(1, &fb->framebufferObject);
    glDeleteTextures(1, &fb->renderableTexture);
    fb->framebufferObject = 0;
    fb->renderableTexture = 0;
}

GLvoid* grabDataPointer(struct framebuffer* fb,GLenum format,GLenum type) {
	int i;
	GLuint pixelFormat,typeSize;
	switch(format){
		case GL_RGB:
			pixelFormat = 3;
			break;
		case GL_LUMINANCE:
			pixelFormat = 1;
			break;
		case GL_RGBA:
			pixelFormat = 4;
			break;
		default:
			pixelFormat = 3;
			break;
	}
	switch(type){
		case GL_UNSIGNED_BYTE:
			typeSize = 1;
			break;
		default:
			break;
	}
	Log("Width %d Height %d typeSize %d pixelFormat %d",fb->width,fb->height,typeSize,pixelFormat);
	GLubyte* pixels = (GLubyte*) malloc(fb->width * fb->height * pixelFormat * typeSize );

	glBindFramebuffer(GL_FRAMEBUFFER, fb->framebufferObject);

	memset(pixels,0,fb->width * fb->height * pixelFormat * typeSize );
	CheckGlError("scalePointer BindTexture");
	glReadPixels(0,0,fb->width,fb->height,format,type,pixels);
	CheckGlError("scalePointer glReadPixels");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return pixels;
}

void renderToFBO(struct framebuffer* fb, void (*drawFunction)(GLuint)) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb->framebufferObject);
    GLint viewport[4];                  // Where The Viewport Values Will Be Stored
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0,0,fb->width,fb->height);

    (*drawFunction)(fb->inputTextureHandler);
    // back to normal window-system-provided framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind

    // trigger mipmaps generation explicitly
    // NOTE: If GL_GENERATE_MIPMAP is set to GL_TRUE, then glCopyTexSubImage2D()
    // triggers mipmap generation automatically. However, the texture attached
    // onto a FBO should generate mipmaps manually via glGenerateMipmap().
    glBindTexture(GL_TEXTURE_2D, fb->renderableTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
}

void initTexture(GLuint* texture,GLenum format,GLenum type,GLuint width,GLuint height,GLvoid* pixels) {
    glGenTextures(1, texture);
    CheckGlError("initTexture: glGenTextures");

    glBindTexture(GL_TEXTURE_2D, *texture);
    Log("Texture ID %d",*texture);
    CheckGlError("initTexture: glBindTexture");

    if(pixels>0)
    	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, pixels);
    else
    	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, 0);
    CheckGlError("initTexture: glTexImage2D");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckGlError("initTexture: glTexParameteri");

    Log("****************************** initTexture: texture ID: %d", *texture);
}

int checkFBOStatus() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	    CheckGlError("engine_init_fbo: glCheckFramebufferStatus");

	    if(status != GL_FRAMEBUFFER_COMPLETE) {
	        switch(status) {
	            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
	                Log("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
	                break;

	            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
	                Log("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
	                break;

	            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
	                Log("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
	                break;

	            case GL_FRAMEBUFFER_UNSUPPORTED:
	                Log("****************************** engine_init_fbo: FBO error: FRAMEBUFFER_UNSUPPORTED");
	                break;

	            default:
	                Log("****************************** engine_init_fbo: Unknown FBO error");
	        }
	    }
	    else {
	        Log("****************************** engine_init_fbo: FBO has been successfully initialized");
	    }
	    return status;
}

void drawTextureOnUniformQuad(GLuint textureHandler){
    // clear buffer

    glClearColor(1, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), uniformQuadVertices );
	CheckGlError( "glVertexAttribPointer" );

	glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), textureCoordinates );
	CheckGlError( "glVertexAttribPointer" );

	// Set active texture
	glBindTexture( GL_TEXTURE_2D, textureHandler );

	glDrawArrays( GL_TRIANGLES, 0, 6 );
	CheckGlError("ImageTargets renderFrame");
}

GLvoid* scalePointer(struct framebuffer* fb,float ratio,GLvoid* inPointer,GLuint width,GLuint height,GLenum format,GLenum type) {
	GLuint textureID;
	int i;
	//	struct framebuffer fb;
	init_fbo(fb,0,ratio*width,ratio*height);
	initTexture(&textureID,format,type,width,height,inPointer);
	fb->inputTextureHandler = textureID;
	renderToFBO(fb,drawTextureOnUniformQuad);
//	destroy_fbo(&fb);
//	glDeleteTextures(1,&textureID);

	return grabDataPointer(fb,format,type);
}
