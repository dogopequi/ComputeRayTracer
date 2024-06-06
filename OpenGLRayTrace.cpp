
#include <stdio.h>
#include <cstdlib>
#include "glm/common.hpp"
#include "Renderer.h"
unsigned int SCR_WIDTH = 1980;
unsigned int SCR_HEIGHT = 1080;
unsigned int IMAGE_WIDTH = 1600;
unsigned int IMAGE_HEIGHT = 900;
int main()
{

    Renderer renderer(false, SCR_WIDTH, SCR_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT, "Dogo");
    renderer.Init("Resources/Shaders/shader.frag", "Resources/Shaders/shader.vert", "Resources/Shaders/shader.compute", "Resources/Shaders/accumulate.compute");
}
