/*
 * Framebuffer.cpp
 *
 *  Created on: 26-03-2013
 *      Author: gozdzseb
 */

#include "Framebuffer.h"
#include "logger.h"

Framebuffer::Framebuffer(GLuint w,GLuint h, GLvoid* pixels,GLenum f,GLenum t):width(w),height(h),format(f),type(t) {
	initFbo(pixels);
}

Framebuffer::~Framebuffer() {
	destroyFbo();
}

GLuint Framebuffer::initRenderbuffer(GLuint width, GLuint height, GLenum format) {
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

void Framebuffer::initFbo(GLvoid* pixels) {
    // create renderable texture
	initTexture(&renderableTexture,width,height,format,type,pixels);

    // create framebuffer object
    glGenFramebuffers(1, &framebufferObject);
    CheckGlError("Framebuffer::initFbo: glGenFramebuffers");

    this->bind();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderableTexture, 0);
    CheckGlError("Framebuffer::initFbo: glFramebufferTexture2D");

    checkFBOStatus();

    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGlError("Framebuffer::initFbo: glBindTexture");

    this->unbind();

	this->width = width;
	this->height = height;
}

void Framebuffer::destroyFbo() {
    glDeleteFramebuffers(1, &framebufferObject);
    glDeleteTextures(1, &renderableTexture);
    framebufferObject = 0;
    renderableTexture = 0;
}

int Framebuffer::checkFBOStatus() {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	CheckGlError("engine_init_fbo: glCheckFramebufferStatus");

	if(status != GL_FRAMEBUFFER_COMPLETE) {
		switch(status) {
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				Log("Framebuffer::checkFBOStatus: FBO error: FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				Log("Framebuffer::checkFBOStatus: FBO error: FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
				Log("Framebuffer::checkFBOStatus: FBO error: FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
				break;

			case GL_FRAMEBUFFER_UNSUPPORTED:
				Log("Framebuffer::checkFBOStatus: FBO error: FRAMEBUFFER_UNSUPPORTED");
				break;

			default:
				Log("Framebuffer::checkFBOStatus: Unknown FBO error");
				break;
		}
	}
	else {
		Log("Framebuffer::checkFBOStatus: FBO has been successfully initialized");
	}
	return status;
}

void Framebuffer::bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER,framebufferObject);
	CheckGlError("Framebuffer::bind: glBindFramebuffer");
}

void Framebuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	CheckGlError("Framebuffer::unbind: glBindFramebuffer");
}

GLvoid* Framebuffer::grabDataPointer() {
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
	Log("Width %d Height %d typeSize %d pixelFormat %d",width,height,typeSize,pixelFormat);
	GLubyte* pixels = new GLubyte[width * height * pixelFormat * typeSize];

	bind();

	CheckGlError("scalePointer BindTexture");
	glReadPixels(0,0,width,height,format,type,pixels);
	CheckGlError("scalePointer glReadPixels");

	unbind();

	return pixels;
}

void Framebuffer::bindTexture() {
	glBindTexture(GL_TEXTURE_2D, renderableTexture);
	CheckGlError("Framebuffer::bindTexture glBindTexture");
}

void Framebuffer::unbindTexture() {
	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGlError("Framebuffer::unbindTexture glBindTexture");
}

void Framebuffer::setViewPort() {
    glGetIntegerv(GL_VIEWPORT, savedViewport);
    glViewport(0,0,width,height);
}

void Framebuffer::recoverSavedViewPort() {
    glGetIntegerv(GL_VIEWPORT, savedViewport);
    glViewport(savedViewport[0],savedViewport[1],savedViewport[2],savedViewport[3]);
}
