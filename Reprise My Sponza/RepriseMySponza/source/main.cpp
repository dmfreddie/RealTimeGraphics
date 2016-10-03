#include "MyController.hpp"

#include <tygra/Window.hpp>

#include <crtdbg.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[])
{
    // enable debug memory checks
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    try {

        auto controller = new MyController();
        auto window = tygra::Window::mainWindow();
        window->setController(controller);

        const int window_width = 1280;
        const int window_height = 720;
        const int number_of_samples = 4;

        if (window->open(window_width, window_height, number_of_samples, true))
        {
            while (window->isVisible()) {
                window->update();
            }
            window->close();
        }

        delete window;
        delete controller;

    }
    catch (...) {
        std::cerr << "Opps ... something went badly wrong:" << std::endl;
    }

    // pause to display any console debug messages
    system("PAUSE");
    return 0;
}
