#include <iostream>
#include <rendering/window.hpp>
#include <rendering/renderer.hpp>
#include <assets/file.hpp>
#include <assets/assets.hpp>
#include <assets/shaders.hpp>

int main()
{
	Window window("Game", 800, 600);
	Renderer renderer(window);

	compileShader(Identifier("core", "vertex"), ShaderType::Vertex);

	while (!window.shouldClose()) {
		window.tick();
		renderer.tick(window);
	}

	renderer.end();

	return 0;
}
