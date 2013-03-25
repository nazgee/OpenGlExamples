/*
 * framebuffer.h
 *
 *  Created on: 22-03-2013
 *      Author: gozdzseb
 */

#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <EGL/egl.h>
#include <GLES2/gl2.h>

GLuint gProgramHandle;
GLuint gaPositionHandle;
GLuint gaTexCoordHandle;
GLuint gaTexSamplerHandle;
GLuint gTextureHandlePNG;

typedef struct
{
    float x;
    float y;

} TriangleVertex;


struct framebuffer {
    GLuint renderableTexture,framebufferObject;
    int height,width;
    GLuint inputTextureHandler;
};

GLuint init_renderbuffer(GLuint width, GLuint height, GLenum format);
void init_fbo(struct framebuffer* fb,GLvoid* pixels, GLuint width, GLuint height);
void destroy_fbo(struct framebuffer* fb);
void renderToFBO(struct framebuffer* fb,void (*drawFunction)(GLuint));
int checkFBOStatus();
void initTexture(GLuint* texture,GLenum format,GLenum type,GLuint width,GLuint height,GLvoid* pixels);
void drawTextureOnUniformQuad(GLuint textureHandler);
GLvoid* scalePointer(struct framebuffer* fb,float ratio,GLvoid* inPointer,GLuint width,GLuint height,GLenum format,GLenum type);
GLvoid* grabDataPointer(struct framebuffer* fb,GLenum format,GLenum type);

#endif /* FRAMEBUFFER_H_ */
