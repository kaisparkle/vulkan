#include <vk/renderer.h>

int main(int argc, char *argv[]) {
    VkRenderer::Renderer renderer;

    renderer.init();
    renderer.run();
    renderer.cleanup();

    return 0;
}
