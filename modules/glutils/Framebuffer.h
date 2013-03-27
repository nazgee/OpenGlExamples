/*
 * Framebuffer.h
 *
 *  Created on: 26-03-2013
 *      Author: gozdzseb
 */

#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <GLUtils.h>

class Framebuffer {
public:
	Framebuffer(GLuint width,GLuint height,GLvoid* pixels = 0,GLenum format = GL_RGB,GLenum type = GL_UNSIGNED_BYTE);
	virtual ~Framebuffer();

	void initFbo(GLvoid* pixels = 0);
	void destroyFbo();
	int checkFBOStatus();
	void bind();
	void unbind();
	void bindTexture();
	void unbindTexture();
	GLvoid* grabDataPointer();
	void setViewPort();
	void recoverSavedViewPort();
private:
	GLuint initRenderbuffer(GLuint width, GLuint height, GLenum format);

    GLuint renderableTexture,framebufferObject;
    int height,width;
    GLuint inputTextureHandler;
    GLenum format,type;
    GLint savedViewport[4];
};

#endif /* FRAMEBUFFER_H_ */
