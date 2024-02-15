#include "error.h"

#include <GL/glew.h>
#include <fmt/core.h>

#include <iostream>

void LogError(std::string_view error_message, std::string_view file, int line) {
  std::cerr << fmt::format("File: {} Line: {} ", file, line) << error_message << '\n';
}

void CheckError(std::string_view file, int line) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        // Process/log the error.
        switch (err)
        {
            case GL_INVALID_ENUM:
                LogError("OpenGL: GL_INVALID_ENUM", file, line);
                break;
          /*  case GL_INVALID_VALUE:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_INVALID_VALUE", file, line));
                break;
            case GL_INVALID_OPERATION:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_INVALID_OPERATION", file, line));
                break;
            case GL_STACK_OVERFLOW:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_STACK_OVERFLOW", file, line));
                break;
            case GL_STACK_UNDERFLOW:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_STACK_UNDERFLOW", file, line));
                break;
            case GL_OUT_OF_MEMORY:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_OUT_OF_MEMORY", file, line));
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_INVALID_FRAMEBUFFER_OPERATION", file, line));
                break;
            case GL_CONTEXT_LOST:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_CONTEXT_LOST", file, line));
                break;
            case GL_TABLE_TOO_LARGE:
                LogError(fmt::format("File: {} Line: {} OpenGL: GL_TABLE_TOO_LARGE", file, line));
                break;*/
            default:
                break;
        }
    }
}