#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>

using namespace std;


int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Block Bloom");

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear(sf::Color::Black);

        window.display();
    }

    return 0;
}