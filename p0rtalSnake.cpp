#include <iostream>
#include <fstream>
#include <sstream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <list>
#include <ctime>
#include <cmath>

using namespace std;

template<typename T>
string toStr(const T &val)
{
    ostringstream ss;
    ss << val;
    return ss.str();
}

enum Direction
{
    North,
    South,
    East,
    West
};

struct Portal
{
    sf::Vector2i pos1, pos2;
    sf::Color color;
};

class PortalPos
{
public:
    PortalPos(const sf::Vector2i &newVal) : val(newVal)
    {

    }

    bool operator() (const Portal &p)
    {
        return (p.pos1 == val) || (p.pos2 == val);
    }
private:
    const sf::Vector2i val;
};


int main()
{
    const unsigned int magicKey = 1340993809;
    bool wrapAround = false;
    bool shadow = false;
    bool flip = false;

    srand(time(NULL));

    sf::RenderWindow window(sf::VideoMode(800, 600), "p0rtalSnake");
    window.SetFramerateLimit(60);
    sf::Vector2i sizei = sf::Vector2i(window.GetWidth(), window.GetHeight()) / 20;
    sf::Vector2f sizef(window.GetWidth() / sizei.x, window.GetHeight() / sizei.y);

    sf::SoundBuffer foodBuffer, hurtBuffer, portalBuffer, wrapBuffer, flipBuffer;
    foodBuffer.LoadFromFile("res/food.wav");
    hurtBuffer.LoadFromFile("res/hurt.wav");
    portalBuffer.LoadFromFile("res/portal.wav");
    wrapBuffer.LoadFromFile("res/wrap.wav");
    flipBuffer.LoadFromFile("res/flip.wav");
    sf::Sound foodSound(foodBuffer), hurtSound(hurtBuffer), portalSound(portalBuffer), wrapSound(wrapBuffer), flipSound(flipBuffer);

    sf::Font font = sf::Font::GetDefaultFont();

    sf::Text scoreText;
    scoreText.SetFont(font);
    scoreText.SetCharacterSize(30);
    scoreText.SetColor(sf::Color(255, 255, 255, 196));
    scoreText.SetPosition(sf::Vector2f(20.f, 20.f));

    sf::Text maxScoreText;
    maxScoreText.SetFont(font);
    maxScoreText.SetCharacterSize(15);
    maxScoreText.SetColor(sf::Color(255, 255, 255, 196));
    maxScoreText.SetPosition(sf::Vector2f(25.f, 55.f));

    unsigned int maxScore;
    {
        ifstream fin("res/score.dat");
        if (fin >> maxScore)
            maxScore ^= magicKey;
        else
            maxScore = 0;
    }

    list<sf::Vector2i> snake;
    snake.push_back(sf::Vector2i(10, 10));
    snake.push_back(sf::Vector2i(10, 11));
    snake.push_back(sf::Vector2i(10, 12));
    snake.push_back(sf::Vector2i(10, 13));
    Direction dir = North;
    sf::Clock moveClock;

    list<sf::Vector2i> foods;
    sf::Clock foodClock;

    list<sf::Vector2i> walls;
    for (int i = 0; i < (sizei.x * sizei.y * 0.01); i++)
    {
        sf::Vector2i pos;
        do
        {
            pos.x = rand() % sizei.x;
            pos.y = rand() % sizei.y;
        }
        while ((find(snake.begin(), snake.end(), pos) != snake.end()) || (find(walls.begin(), walls.end(), pos) != walls.end()));
        walls.push_back(pos);
    }

    list<Portal> portals;
    for (int i = 0; i < 3; i++)
    {
        sf::Vector2i pos1, pos2;
        do
        {
            pos1.x = rand() % sizei.x;
            pos1.y = rand() % sizei.y;
        }
        while ((find(snake.begin(), snake.end(), pos1) != snake.end()) || (find(walls.begin(), walls.end(), pos1) != walls.end()) || (find_if(portals.begin(), portals.end(), PortalPos(pos1)) != portals.end()));

        do
        {
            pos2.x = rand() % sizei.x;
            pos2.y = rand() % sizei.y;
        }
        while ((find(snake.begin(), snake.end(), pos2) != snake.end()) || (find(walls.begin(), walls.end(), pos2) != walls.end()) || (find_if(portals.begin(), portals.end(), PortalPos(pos2)) != portals.end()));

        sf::Color color(rand() % 256, rand() % 256, rand() % 256);

        portals.push_back(Portal{pos1, pos2, color});
    }

    while (window.IsOpen())
    {
        sf::Event e;
        while (window.PollEvent(e))
        {
            if (e.Type == sf::Event::Closed)
            {
                window.Close();
            }
            else if (e.Type == sf::Event::KeyPressed)
            {
                const sf::Vector2i &head = snake.front();
                switch (e.Key.Code)
                {
                    case sf::Keyboard::Up:
                        if (find(snake.begin(), snake.end(), head + sf::Vector2i(0, -1)) == snake.end())
                            dir = North;
                        break;
                    case sf::Keyboard::Down:
                        if (find(snake.begin(), snake.end(), head + sf::Vector2i(0, 1)) == snake.end())
                            dir = South;
                        break;
                    case sf::Keyboard::Right:
                        if (find(snake.begin(), snake.end(), head + sf::Vector2i(1, 0)) == snake.end())
                            dir = East;
                        break;
                    case sf::Keyboard::Left:
                        if (find(snake.begin(), snake.end(), head + sf::Vector2i(-1, 0)) == snake.end())
                            dir = West;
                        break;
                    case sf::Keyboard::Space:
                    {
                        if (flip)
                        {
                            /*
                            bug when snake.size() == 2 and flipping while being wrapped around
                            */
                            snake.reverse();
                            const sf::Vector2i &head = snake.front();
                            list<sf::Vector2i>::iterator second = snake.begin();
                            second++;

                            sf::Vector2i d = head - *second;
                            if (d.y < 0)
                                dir = North;
                            else if (d.y > 0)
                                dir = South;
                            else if (d.x > 0)
                                dir = East;
                            else if (d.x < 0)
                                dir = West;
                            else
                                throw "Shit happened!";

                            flipSound.Play();
                        }
                        break;
                    }
                    case sf::Keyboard::S:
                        shadow = !shadow;
                        break;
                    case sf::Keyboard::W:
                        wrapAround = !wrapAround;
                        break;
                    case sf::Keyboard::F:
                        flip = !flip;
                        break;
                    default:
                        break;
                }
            }
        }

        if (foodClock.GetElapsedTime().AsSeconds() >= 2)
        {
            for (int i = 0; i < sizei.x * sizei.y; i++)
            {
                sf::Vector2i pos(rand() % sizei.x, rand() % sizei.y);
                if ((find(snake.begin(), snake.end(), pos) == snake.end()) && (find(walls.begin(), walls.end(), pos) == walls.end()) && (find(foods.begin(), foods.end(), pos) == foods.end()))
                {
                    foods.push_back(pos);
                    break;
                }
            }
            foodClock.Restart();
        }

        if (moveClock.GetElapsedTime().AsMilliseconds() >= (450 / sqrt(snake.size())))
        {
            sf::Vector2i head = snake.front();
            switch (dir)
            {
                case North:
                    head.y--;
                    break;
                case South:
                    head.y++;
                    break;
                case East:
                    head.x++;
                    break;
                case West:
                    head.x--;
                    break;
            }

            if (wrapAround)
            {
                if (head.x < 0)
                {
                    head.x += sizei.x;
                    wrapSound.Play();
                }
                else if (head.x >= sizei.x)
                {
                    head.x %= sizei.x;
                    wrapSound.Play();
                }

                if (head.y < 0)
                {
                    head.y += sizei.y;
                    wrapSound.Play();
                }
                else if (head.y >= sizei.y)
                {
                    head.y %= sizei.y;
                    wrapSound.Play();
                }
            }

            for (list<Portal>::iterator it = portals.begin(); it != portals.end(); ++it)
            {
                bool through = false;
                if (head == it->pos1)
                {
                    head = it->pos2;
                    through = true;
                }
                else if (head == it->pos2)
                {
                    head = it->pos1;
                    through = true;
                }

                if (through)
                {
                    switch (dir)
                    {
                        case North:
                            head.y--;
                            break;
                        case South:
                            head.y++;
                            break;
                        case East:
                            head.x++;
                            break;
                        case West:
                            head.x--;
                            break;
                    }

                    if (wrapAround)
                    {
                        if (head.x < 0)
                            head.x += sizei.x;
                        else
                            head.x %= sizei.x;

                        if (head.y < 0)
                            head.y += sizei.y;
                        else
                            head.y %= sizei.y;
                    }

                    portalSound.Play();
                    break;
                }
            }


            if ((find(snake.begin(), snake.end(), head) == snake.end()) && (find(walls.begin(), walls.end(), head) == walls.end()) && (head.x >= 0) && (head.x < sizei.x) && (head.y >= 0) && (head.y < sizei.y))
            {
                snake.push_front(head);

                list<sf::Vector2i>::iterator foodIt = find(foods.begin(), foods.end(), head);
                if (foodIt != foods.end())
                {
                    foods.erase(foodIt);
                    foodSound.Play();
                }
                else
                    snake.pop_back();
            }
            else
            {
                // hits wall/snake
                if (snake.size() > 2)
                {
                    snake.pop_back();
                    hurtSound.Play();
                }
            }

            moveClock.Restart();
        }

        if (snake.size() > maxScore)
            maxScore = snake.size();

        scoreText.SetString(toStr(snake.size()));
        maxScoreText.SetString(toStr(maxScore));


        window.Clear();

        sf::RectangleShape rect;
        rect.SetSize(sizef - sf::Vector2f(2.f, 2.f));
        rect.SetOrigin(sf::Vector2f(-1.f, -1.f));

        rect.SetFillColor(sf::Color(64, 64, 64));
        for (int y = 0; y < sizei.y; y++)
        {
            for (int x = 0; x < sizei.x; x++)
            {
                rect.SetPosition(sf::Vector2f(x * sizef.x, y * sizef.y));
                window.Draw(rect);
            }
        }

        rect.SetFillColor(sf::Color(32, 32, 32));
        for (list<sf::Vector2i>::iterator it = walls.begin(); it != walls.end(); ++it)
        {
            sf::Vector2i &pos = *it;
            rect.SetPosition(sf::Vector2f(pos.x * sizef.x, pos.y * sizef.y));
            window.Draw(rect);
        }

        rect.SetFillColor(sf::Color::Green);
        for (list<sf::Vector2i>::iterator it = foods.begin(); it != foods.end(); ++it)
        {
            sf::Vector2i &pos = *it;
            rect.SetPosition(sf::Vector2f(pos.x * sizef.x, pos.y * sizef.y));
            window.Draw(rect);
        }

        for (list<sf::Vector2i>::iterator it = snake.begin(); it != snake.end(); ++it)
        {
            sf::Vector2i &pos = *it;
            if (it == snake.begin())
                rect.SetFillColor(sf::Color::Red);
            else
            {
                int mix = max(255 - distance(snake.begin(), it), 32);
                rect.SetFillColor(sf::Color(mix, mix, 0));
            }
            rect.SetPosition(sf::Vector2f(pos.x * sizef.x, pos.y * sizef.y));
            window.Draw(rect);
        }

        for (list<Portal>::iterator it = portals.begin(); it != portals.end(); ++it)
        {
            rect.SetFillColor(it->color);

            rect.SetPosition(sf::Vector2f(it->pos1.x * sizef.x, it->pos1.y * sizef.y));
            window.Draw(rect);

            rect.SetPosition(sf::Vector2f(it->pos2.x * sizef.x, it->pos2.y * sizef.y));
            window.Draw(rect);
        }

        if (shadow)
        {
            rect.SetFillColor(sf::Color::Black);
            for (int y = 0; y < sizei.y; y++)
            {
                for (int x = 0; x < sizei.x; x++)
                {
                    if ((pow(x - snake.front().x, 2) + pow(y - snake.front().y, 2)) > pow(10, 2))
                    {
                        rect.SetPosition(sf::Vector2f(x * sizef.x, y * sizef.y));
                        window.Draw(rect);
                    }

                }
            }
        }

        window.Draw(scoreText);
        window.Draw(maxScoreText);

        window.Display();
    }

    {
        ofstream fout("res/score.dat");
        fout << (maxScore ^ magicKey);
    }
    return 0;
}
