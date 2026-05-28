#include <iostream>
#include <sfml/Graphics.hpp>

using namespace std;

int main()
{
    // Create the game window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Block Bloom");

    // Main game loop
    while (window.isOpen())
    {
        sf::Event event;

        // Check events
        while (window.pollEvent(event))
        {
            // Close window button
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        // Clear window with black color
        window.clear();

        // Display everything
        window.display();
    }

    return 0;
}

 