/*
 * GLUtils.h
 *
 *  Created on: 26-03-2013
 *      Author: gozdzseb
 */

#ifndef GLUTILS_H_
#define GLUTILS_H_

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "Framebuffer.h"


	GLuint createProgram( const char* pVertexPath, const char* pFragmentPath );
	GLuint CompileShader( GLenum shaderType, const char* pSource , GLint* fileSize );
	GLvoid* scalePointer(float ratio,GLvoid* inPointer,GLuint width,GLuint height,GLenum format,GLenum type);
	void initTexture(GLuint* texture,GLuint width,GLuint height,GLenum format = GL_RGB,GLenum type = GL_UNSIGNED_BYTE,GLvoid* pixels = 0);

#endif /* GLUTILS_H_ */
