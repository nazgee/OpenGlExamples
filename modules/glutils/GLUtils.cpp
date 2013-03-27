/*
 * GLUtils.cpp
 *
 *  Created on: 26-03-2013
 *      Author: gozdzseb
 */

#include "GLUtils.h"
#include "logger.h"
#include "file.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// CompileShader - Compiles the passed in string for the given shaderType
GLuint CompileShader( GLenum shaderType, const char* pSource, GLint * fileSize )
{
    // Create a shader handle
    GLuint shaderHandle = glCreateShader( shaderType );

    if( shaderHandle )
    {
        // Set the shader source
        glShaderSource( shaderHandle, 1, &pSource, fileSize );

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
                char* pShaderInfoLog = new char[infoLogLength];
                if( pShaderInfoLog )
                {
                    glGetShaderInfoLog( shaderHandle, infoLogLength, NULL, pShaderInfoLog );
                    LogError( "Error compiling shader: \n%s", pShaderInfoLog );
                    delete pShaderInfoLog;
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
GLuint createProgram( const char* pVertexPath, const char* pFragmentPath )
{
    char* pVertexData = NULL;
    GLint vertexFileSize = 0;
    char* pFragmenData = NULL;
    GLint fragmentFileSize = 0;
    ReadFile( pVertexPath, &pVertexData, (unsigned int*)&vertexFileSize );
	Log("Shader size %d ",vertexFileSize);
	Log("Shader val \n%s",pVertexData);
    // Compile the vertex shader
    GLuint vertexShaderHandle = CompileShader( GL_VERTEX_SHADER, pVertexData , &vertexFileSize );
    delete pVertexData;
    ReadFile(pFragmentPath,&pFragmenData,(unsigned int*)&fragmentFileSize);
    Log("Shader size %d ",fragmentFileSize);
    Log("Shader val \n%s",pFragmenData);
    GLuint pixelShaderHandle  = CompileShader( GL_FRAGMENT_SHADER, pFragmenData , &fragmentFileSize );

    delete pFragmenData;

    if( !vertexShaderHandle || !pixelShaderHandle )
    {
        return 0;
    }
    Log("Vertex file compiled");
    Log("Fragment file compiled");

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
                char* pInfoLog = new char[infoLogLength];
                if( pInfoLog )
                {
                    glGetProgramInfoLog( programHandle, infoLogLength, NULL, pInfoLog );
                    LogError( "Error linking the program: \n%s", pInfoLog );
                    delete pInfoLog;
                }
            }

            // Free the handle
            glDeleteProgram( programHandle );
            programHandle = 0;
        }
    }

    return programHandle;
}

GLvoid* scalePointer(float ratio,GLvoid* inPointer,GLuint width,GLuint height,GLenum format,GLenum type) {
	GLuint textureID;
	int i;
	Framebuffer* fb = new Framebuffer(ratio*width,ratio*height,0,format,type);
	//	struct framebuffer fb;
//	init_fbo(fb,0,ratio*width,ratio*height);
	initTexture(&textureID,format,type,width,height,inPointer);
//	fb->inputTextureHandler = textureID;
//	renderToFBO(fb,drawTextureOnUniformQuad);
//	destroy_fbo(&fb);
//	glDeleteTextures(1,&textureID);

//	return fb->grabDataPointer(fb,format,type);
}

void initTexture(GLuint* texture,GLuint width,GLuint height,GLenum format,GLenum type,GLvoid* pixels) {
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

