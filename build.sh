
# gcc gl_x11_test.c -g -o glX11Test -lX11 -lGL -lGLU
# gcc x11_test.c -o vkX11Test -lX11  -lGL -lGLU

/home/jh/dev/glslc/bin/glslc shaders/shader.vert -o shaders/vert.spv
/home/jh/dev/glslc/bin/glslc shaders/shader.frag -o shaders/frag.spv
g++ vk_glfw_test.cpp -g --std=c++20 -DNDEBUG -o testprogram.out -lglfw -lvulkan
