#include <iostream>
#include <rendering/window.hpp>
#include <rendering/renderer.hpp>

int main()
{
	Window window("Game", 800, 600);
	Renderer renderer(window);

	while (!window.shouldClose()) {
		//std::cout << "tick" << std::endl;
		window.tick();
		renderer.tick();
	}

	return 0;
}
