/*
 * Scene.cpp
 *
 *  Created on: 26-03-2013
 *      Author: gozdzseb
 */

#include "Scene.h"
#include "logger.h"
#include <assert.h>
#include <stdlib.h>
TriangleVertex Scene::triangleVerticesPNG[] = {
		{ 1.0f,  1.0f }, { -1.0f, -1.0f }, { 1.0f, -1.0f },
		{ -1.0f, 1.0f }, { -1.0f, -1.0f }, { 1.0f,  1.0f }
};

TriangleVertex Scene::textureCoordsPNG[] = {
		{ 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f },
		{ 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }
};

Scene::Scene(int w,int h):width(w),height(h),scale(1.0),fb(0),textureHandle(0),checkboard_width(256),checkboard_height(256) {
	   // Initialize GL state.
	//    glHint(GL_PEr, GL_FASTEST);
	    glEnable(GL_CULL_FACE);
	//    glShadeModel(GL_SH);
	    glDisable(GL_DEPTH_TEST);

	    // Init the shaders
	//    gProgramHandle = createProgram( gVertexShader, gPixelShader );
	    programHandle = createProgram( "shaders/vertexShader", "shaders/fragmentShader" );
	    Log("Program handle %d",programHandle);
	    if( !programHandle )
	    {
	        LogError( "Could not create program." );
	        assert(0);
	    }
	//
	    // Read attribute locations from the program
	    aPositionHandle = glGetAttribLocation( programHandle, "aPosition" );
	    Log("gaPositionHandle %d",aPositionHandle);
	    CheckGlError( "glGetAttribLocation" );
	//
	    aTexCoordHandle = glGetAttribLocation( programHandle, "aTexCoord" );
	    Log("gaTexCoordHandle %d",aTexCoordHandle);
	    CheckGlError( "glGetAttribLocation" );

	    aTexSamplerHandle = glGetUniformLocation( programHandle, "sTexture" );
	    Log("gaTexSamplerHandle %d",aTexSamplerHandle);
	    CheckGlError( "glGetUnitformLocation" );

	    GLubyte* pixels = generateCheckBoardTextureData(checkboard_width,checkboard_height,3);


	    GLubyte* resizedPointer = (GLubyte*)scaleTexture(1.0,pixels,checkboard_width,checkboard_height,GL_RGB,GL_UNSIGNED_BYTE);
	    /*
	     * Do whatever you want with resized texture data pointer
	     */
	    delete resizedPointer;
	    delete pixels;
	    glViewport(0,0,width,height);

}

Scene::~Scene() {
	if(fb)
		delete fb;
	fb = NULL;
}

void Scene::draw(GLuint textureHandler) {
    glClearColor( 0.8f, 0.7f, 0.6f, 1.0f);
    CheckGlError( "glClearColor" );

    // Clear the color and depth buffer
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    CheckGlError( "glClear" );

    // Select vertex/pixel shader
    glUseProgram( programHandle );
    CheckGlError( "glUseProgram" );


    CheckGlError( "glUniformMatrix4fv" );
    // Enable vertex
    glEnableVertexAttribArray( aPositionHandle );
    CheckGlError( "glEnableVertexAttribArray" );

    // Enable tex coords
    glEnableVertexAttribArray( aTexCoordHandle );
    CheckGlError( "glEnableVertexAttribArray" );

    // Set texture sampler
    glActiveTexture( GL_TEXTURE0 );

    // Enable texture sampler
    glUniform1i( aTexSamplerHandle, 0 );

    // PNG //////////////////////////////////////////////////////////////////////////////////////////////////////

    // Set vertex position
    glVertexAttribPointer( aPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), triangleVerticesPNG );
    CheckGlError( "glVertexAttribPointer" );

//     Set vertex texture coordinates
    glVertexAttribPointer( aTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), textureCoordsPNG );
    CheckGlError( "glVertexAttribPointer" );

//    glBindTexture( GL_TEXTURE_2D, fb.renderableTexture );
    if(textureHandler) {
    	glBindTexture( GL_TEXTURE_2D, textureHandler );
    }
    else {
    	fb->bindTexture();
    }

    glDrawArrays( GL_TRIANGLES, 0, 6 );

	fb->unbindTexture();

    glFlush();
}

GLubyte* Scene::generateCheckBoardTextureData(GLuint width,GLuint height, GLuint format){
    GLubyte* pixels = new GLubyte[3*width*height*sizeof(uint8_t)];
    uint8_t color = 255;
    int i;
    for(i=0;i<height*format*width;i+=8*format)
    {
    	color = (color == 255) ? 0 : 255;
    	if(i %(width*format*8) == 0)
    	{
    		color = (color == 255) ? 0 : 255;
    	}
    	memset(pixels+i,color,3*8);

    }
    return pixels;
}

void Scene::renderTextureToFbo() {
	fb->bind();
	Log("Scene::renderTextureToFbo width %f height %f",checkboard_width*scale,checkboard_height*scale);
    fb->setViewPort();

    this->draw(textureHandle);
    // back to normal window-system-provided framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind
    fb->unbind();

    fb->recoverSavedViewPort();
}

void Scene::scaleUp() {
	scale += 0.05;
	if(scale > 10.0)
		scale = 2.0;
	if(fb)
		delete fb;
	fb = new Framebuffer(scale*checkboard_width,scale*checkboard_height,0,GL_RGB,GL_UNSIGNED_BYTE);
	renderTextureToFbo();
}

void Scene::scaleDown() {
	scale -= 0.05;
	if(scale < 0.0)
		scale = 0.0;
	if(fb)
		delete fb;
	fb = new Framebuffer(scale*checkboard_width,scale*checkboard_height,0,GL_RGB,GL_UNSIGNED_BYTE);
	renderTextureToFbo();
}

void Scene::loadTextureFromPointer(GLvoid* data,GLuint width,GLuint height,GLenum format,GLenum type) {
	if(textureHandle > 0)
		glDeleteTextures(1,&textureHandle);
	initTexture(&textureHandle,width,height,format,type,data);
}

GLvoid* Scene::scaleTexture(float ratio,GLvoid* data,GLuint w,GLuint h,GLenum f,GLenum t) {
	loadTextureFromPointer(data,w,h,f,t);
	checkboard_height = h;
	checkboard_width = w;
	scale = ratio;
	Log("Texture Loaded from pointer Tex width %f height %f",width*ratio,height*ratio);
	if(fb)
		delete fb;
	fb = new Framebuffer(ratio*w,ratio*h,0,f,t);
	renderTextureToFbo();
	GLvoid* resizedTextureData = fb->grabDataPointer();
	return resizedTextureData;
}
