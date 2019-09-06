//
// Created by Sidorenko Nikita on 1/27/18.
//

#include "ShaderLoader.h"
#include <sstream>
#include <iostream>
#include <system/Logging.h>

const std::string VERTEX_HEADER = "[vertex]";
const std::string FRAGMENT_HEADER = "[fragment]";

// Splits vertex and fragment shaders from the source stream into two separate strings
void loader::loadShader(std::stringstream &sourceStream, std::string *outVertexSource, std::string *outFragmentSource)
{
  std::string line;

  std::ostringstream vertexStream;
  std::ostringstream fragmentStream;
  std::ostringstream *currentStream = nullptr;

  while (std::getline(sourceStream, line)) {
    if (line == VERTEX_HEADER) {
      currentStream = &vertexStream;
      continue;
    }
    if (line == FRAGMENT_HEADER) {
      currentStream = &fragmentStream;
      continue;
    }

    if (currentStream) {
      *currentStream << line << "\n";
    } else {
      ENGLog("ERROR: Shader header not found");
      return;
    }
  }

  *outVertexSource = vertexStream.str();
  *outFragmentSource = fragmentStream.str();
}