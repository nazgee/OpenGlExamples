/*
 * Scene.h
 *
 *  Created on: 26-03-2013
 *      Author: gozdzseb
 */

#ifndef SCENE_H_
#define SCENE_H_

#include "GLUtils.h"

typedef struct
{
    float x;
    float y;

} TriangleVertex;

class Scene {
public:
	Scene(int width,int height);
	virtual ~Scene();
	void draw(GLuint textureHandler = 0);
	void renderTextureToFbo();
	void scaleDown();
	void scaleUp();
	void loadTextureFromPointer(GLvoid* data,GLuint width, GLuint height,GLenum format,GLenum type);
	GLvoid* scaleTexture(float ratio,GLvoid* data,GLuint width,GLuint height,GLenum format,GLenum type);
private:
	static TriangleVertex triangleVerticesPNG[];
	static TriangleVertex textureCoordsPNG[];

	GLuint programHandle;
	GLuint aPositionHandle;
	GLuint aTexCoordHandle;
	GLuint aTexSamplerHandle;
	GLuint textureHandle;

	int width,height;
	Framebuffer* fb;

	float scale;
	GLubyte* generateCheckBoardTextureData(GLuint width,GLuint height, GLuint format);
	int checkboard_width,checkboard_height;
};

#endif /* SCENE_H_ */
