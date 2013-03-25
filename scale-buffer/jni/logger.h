/*
 * logger.h
 *
 *  Created on: 22-03-2013
 *      Author: gozdzseb
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <android/log.h>

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


#endif /* LOGGER_H_ */
