#include <stdio.h>
#include <math.h>
#include <float.h>
#include <windows.h>
#include <vector>
#include <SFML\Graphics.hpp>
#include "Tools.hpp"
#include <math.h>

#define PI 3.14159265

//-----------------------------------------------------------------------------

struct Line
{
    sf::Vector2f p1;
    sf::Vector2f p2;
};

//-----------------------------------------------------------------------------

class GameObject
{
private:
    sf::Shape  *shape = 0;
    sf::Texture texture;

public:
    GameObject();

    void setShape  (sf::Shape   &shape);
    void setTexture(sf::Texture &texture);

    sf::Vector2f getPosition  ();
    sf::Vector2f getPoint     (int point);
    int          getPointCount();
    sf::Texture* getTexture   ();
};

//-----------------------------------------------------------------------------

class Camera
{
private:
    sf::Vector2f cameraPosition;
    float        rotationAngle;

    //Viewing range
    float viewRange;
    //Angle viewing in degrees
    float viewAngle;
    //Distance between Ray
    float distanceRays;

    std::vector<sf::Vector2f> pointsRays;

    bool crossingLines      (Line line_1, Line   line_2, sf::Vector2f *pCrossing);
    void getLineCoefficients(Line line,   float *k,      float        *b);

public:
    Camera(float _viewRange = 1000, float _viewAngle = 75, float _distanceRays = 0.1);

    void setPosition(sf::Vector2f position);
    void setRotation(float        angle);

    void rayCasting (std::vector<GameObject*> objects, sf::RenderWindow &window);
    void draw       (sf::RenderWindow &window, float scale);
};

//-----------------------------------------------------------------------------

float DegreesToRadians(float degrees);

template <typename type>
bool BelongingInterval(type min, type max, type num, float error = 0);

template <typename type>
void Swap(type *a, type *b);

sf::Color CorrectLightness(sf::Color color, int delta);

void Move  (sf::CircleShape *player, float velocity, float dt, float angle);
void Rotate(sf::CircleShape *player, float velocity, float dt, sf::RenderWindow &window);

void CreateMap(std::vector<GameObject*> &objects);
void DrawMap  (std::vector<GameObject*> objects, sf::RenderWindow &window, float scale = 1);

//-----------------------------------------------------------------------------

int main()
{
    sf::CircleShape player(10);
    player.setFillColor(sf::Color::Yellow);
    player.setPosition(200, 200);

    Camera camera;

    float rotateAngle = 0;

    const int WindowWidth  = 1000;
    const int WindowHeight = 700;

    sf::RenderWindow window(sf::VideoMode(WindowWidth, WindowHeight), "RayCasting");

    window.setMouseCursorVisible(false);

    std::vector<GameObject*> objects;
    CreateMap(objects);

    sf::RectangleShape ground;
    ground.setSize({WindowWidth, WindowHeight / 2});
    ground.setPosition(0, WindowHeight / 2);
    ground.setFillColor(sf::Color(200, 200, 0));

    sf::Clock clock;

    while(true)
    {
        float dt = clock.getElapsedTime().asMicroseconds();
		dt /= 1000;
		clock.restart();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) window.close();
        }

        window.clear({149, 202, 255});

        window.draw(ground);

        Rotate(&player, 0.009, dt, window);
        Move  (&player, 0.15,  dt, player.getRotation());

        DrawMap(objects, window, 0.3);

        camera.setPosition({ player.getPosition().x + 5, player.getPosition().y + 5});
        camera.setRotation(player.getRotation());
        camera.rayCasting(objects, window);

        camera.draw(window, 0.3);

        window.display();
    }
}

//-----------------------------------------------------------------------------
//{     Camera realization
//-----------------------------------------------------------------------------

Camera::Camera(float _viewRange, float _viewAngle, float _distanceRays): viewRange   (_viewRange),
                                                                         viewAngle   (_viewAngle),
                                                                         distanceRays(_distanceRays)
{
    if(distanceRays < 1)
    {
        pointsRays.resize(ceil(viewAngle / distanceRays));
    }
    else
    {
        pointsRays.resize(ceil(viewAngle / distanceRays) + 1);
    }
}

//-----------------------------------------------------------------------------

//Return point of crossing two lines
bool Camera::crossingLines(Line line_1, Line line_2, sf::Vector2f *pCrossing)
{
    float error = 0.5;

    bool isParallelY_1 = true;
    float k1 = FLT_MAX, b1 = FLT_MAX;
    if(abs(line_1.p1.x - line_1.p2.x) >= error)
    {
        getLineCoefficients(line_1, &k1, &b1);
        isParallelY_1 = false;
    }

    bool isParallelY_2 = true;
    float k2 = FLT_MAX, b2 = FLT_MAX;
    if(abs(line_2.p1.x - line_2.p2.x) >= error)
    {
        getLineCoefficients(line_2, &k2, &b2);
        isParallelY_2 = false;
    }

    if(k1 == k2)
    {
        return false;
    }

    if     (isParallelY_1)
    {
        pCrossing->x = line_1.p1.x;
        pCrossing->y = k2 * (pCrossing->x) + b2;
    }
    else if(isParallelY_2)
    {
        pCrossing->x = line_2.p1.x;
        pCrossing->y = k1 * (pCrossing->x) + b1;
    }
    else
    {
        pCrossing->x = (b2 - b1) / (k1 - k2);
        pCrossing->y = k1 * (pCrossing->x) + b1;
    }

    if(!BelongingInterval(line_1.p1.x, line_1.p2.x, pCrossing->x, error) ||
       !BelongingInterval(line_1.p1.y, line_1.p2.y, pCrossing->y, error) ||
       !BelongingInterval(line_2.p1.x, line_2.p2.x, pCrossing->x, error) ||
       !BelongingInterval(line_2.p1.y, line_2.p2.y, pCrossing->y, error))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

//y = k*x + b (return k and b)
void Camera::getLineCoefficients(Line line, float *k, float *b)
{
    if(line.p1.x == line.p2.x)
    {
        (*k) = 0;
        (*b) = line.p1.y;

        return;
    }
    (*k) = (line.p2.y - line.p1.y) / (line.p2.x - line.p1.x);

    (*b) = line.p1.y - (*k) * line.p1.x;
}

//-----------------------------------------------------------------------------

void Camera::setPosition(sf::Vector2f position)
{
    cameraPosition = position;
}

//-----------------------------------------------------------------------------

void Camera::setRotation(float angle)
{
    rotationAngle = angle;
}

//-----------------------------------------------------------------------------

void Camera::rayCasting(std::vector<GameObject*> objects, sf::RenderWindow &window)
{
    int countObjects = objects.size();

    float angleNow = -(viewAngle / 2);
    for(int i = 1; angleNow <= viewAngle / 2; i++)
    {
        float angleRadians = DegreesToRadians(rotationAngle + angleNow);

        sf::Vector2f pos = {0, 0};
        pos.x = cameraPosition.x + cos(angleRadians) * viewRange;
        pos.y = cameraPosition.y + sin(angleRadians) * viewRange;

        int obj   = 0;
        int point = 0;

        sf::Vector2f pCrossing;
        float minDistance = FLT_MAX;
        for(int j = 0; j < countObjects; j++)
        {
            int countPoints = objects[j]->getPointCount();
            for(int p = 0; p < countPoints; p++)
            {
                sf::Vector2f pointCrossing = {0, 0};
                if(crossingLines({ cameraPosition, pos },
                                 { objects[j]->getPoint(p)                     + objects[j]->getPosition(),
                                   objects[j]->getPoint((p + 1) % countPoints) + objects[j]->getPosition() },
                                 &pointCrossing))
                {
                    float distance = DistanceBetweenPoints(cameraPosition, pointCrossing);

                    distance *= cos(DegreesToRadians(angleNow));

                    if(distance < minDistance)
                    {
                        pCrossing = pointCrossing;
                        minDistance = distance;

                        obj   = j;
                        point = p;
                    }
                }
            }
        }

        if(minDistance == FLT_MAX)
        {
            pointsRays[i - 1] = pos;
        }
        else
        {
            sf::RectangleShape rectangle;

            pointsRays[i - 1] = pCrossing;

            int countPoints = objects[obj]->getPointCount();

            float widthWallObject = DistanceBetweenPoints(objects[obj]->getPoint(point), objects[obj]->getPoint((point + 1) % countPoints));

            float k = DistanceBetweenPoints(objects[obj]->getPoint(point), pCrossing) / widthWallObject;

            float width  = 1000 / (viewAngle / distanceRays);
            float height = 100  / minDistance * 400;

            rectangle.setSize    ({ width ,          height });
            rectangle.setPosition({ (i - 1) * width, (700 - height) / 2 });

            sf::Texture *texture = objects[obj]->getTexture();

            sf::Vector2f sizeTexture = (sf::Vector2f)texture->getSize();

            rectangle.setTexture(texture);

            rectangle.setTextureRect({{sizeTexture.x * k, 0}, {sizeTexture.x / (viewAngle / distanceRays), sizeTexture.y}});

            //sf::Color color(235, 26, 36);
            //color = CorrectLightness(color, std::min(0, -(int)(minDistance / 1.2)));
            //rectangle.setFillColor(color);

            window.draw(rectangle);
        }

        angleNow += distanceRays;
    }
}

//-----------------------------------------------------------------------------

void Camera::draw(sf::RenderWindow &window, float scale)
{
    sf::Vertex line[2];
    line[0] = sf::Vertex(cameraPosition * scale);
    line[0].color = sf::Color::Black;

    int countPoints = pointsRays.size();
    for(int i = 0; i < countPoints; i++)
    {
        line[1] = sf::Vertex(pointsRays[i] * scale);
        line[1].color = sf::Color::Black;
        window.draw(line, 2, sf::Lines);
    }
}

//-----------------------------------------------------------------------------
//}     End of Camera Block
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//{     GameObject realization
//-----------------------------------------------------------------------------

GameObject::GameObject()
{

}

//-----------------------------------------------------------------------------

void GameObject::setShape(sf::Shape &shape)
{
      this->shape = &shape;
}

//-----------------------------------------------------------------------------

void GameObject::setTexture(sf::Texture &texture)
{
    this->texture = texture;
}

//-----------------------------------------------------------------------------

sf::Vector2f GameObject::getPosition()
{
    return shape->getPosition();
}

//-----------------------------------------------------------------------------

sf::Vector2f GameObject::getPoint(int point)
{
    return shape->getPoint(point);
}

//-----------------------------------------------------------------------------

int GameObject::getPointCount()
{
    return shape->getPointCount();
}

//-----------------------------------------------------------------------------

sf::Texture* GameObject::getTexture()
{
    return &texture;
}

//-----------------------------------------------------------------------------
//}     End of GameObject
//-----------------------------------------------------------------------------

void CreateMap(std::vector<GameObject*> &objects)
{
    static sf::RectangleShape boundary;
    boundary.setSize({497, 497});

    objects.push_back(new GameObject);
    objects[0]->setShape(boundary);

    sf::Texture texture;
    texture.loadFromFile("Wall.jpg");

    static sf::RectangleShape rectangle_1;
    rectangle_1.setFillColor(sf::Color::Red);

    rectangle_1.setPosition({112, 0});
    rectangle_1.setSize    ({73,  212});

    objects.push_back(new GameObject);
    objects[1]->setShape  (rectangle_1);
    objects[1]->setTexture(texture);

    /*
    static sf::RectangleShape rectangle_2;
    rectangle_2.setFillColor(sf::Color::Red);

    rectangle_2.setPosition({419, 0});
    rectangle_2.setSize    ({80,  112});

    objects.push_back(new GameObject);
    objects[objects.size() - 1]->setShape(rectangle_2);

    static sf::RectangleShape rectangle_3;
    rectangle_3.setFillColor(sf::Color::Red);

    rectangle_3.setPosition({347, 256});
    rectangle_3.setSize    ({91,  48});

    objects.push_back(new GameObject);
    objects[objects.size() - 1]->setShape(rectangle_3);

    static sf::ConvexShape triangle(3);
    triangle.setFillColor(sf::Color::Red);

    triangle.setPoint(0, {177, 334});
    triangle.setPoint(1, {259, 443});
    triangle.setPoint(2, {98,  443});

    objects.push_back(new GameObject);
    objects[objects.size() - 1]->setShape(triangle);
    */
}

//-----------------------------------------------------------------------------

void DrawMap(std::vector<GameObject*> objects, sf::RenderWindow &window, float scale)
{
    int countObjects = objects.size();
    for(int i = 0; i < countObjects; i++)
    {
        //objects[i]->setScale(scale, scale);

        //sf::Vector2f position = objects[i]->getPosition();
        //objects[i]->setPosition(position * scale);

        //window.draw(*objects[i]);

        //objects[i]->setPosition(position);
    }
}

//-----------------------------------------------------------------------------

float DegreesToRadians(float degrees)
{
    return degrees * PI / 180;
}

//-----------------------------------------------------------------------------

template <typename type>
bool BelongingInterval(type min, type max, type num, float error)
{
    if(min > max) Swap(&min, &max);

    if(num >= min - error && num <= max + error) return true;
    else                                     return false;
}

//-----------------------------------------------------------------------------

template <typename type>
void Swap(type *a, type *b)
{
    type c = (*a);
    (*a) = (*b);
    (*b) = c;
}

//-----------------------------------------------------------------------------

sf::Color CorrectLightness(sf::Color color, int delta)
{
      int r = std::min(std::max(color.r + delta * 30 / 100, 0), 255);
      int g = std::min(std::max(color.g + delta * 59 / 100, 0), 255);
      int b = std::min(std::max(color.b + delta * 11 / 100, 0), 255);

      return sf::Color(r, g, b);
}

//-----------------------------------------------------------------------------

void Move(sf::CircleShape *player, float velocity, float dt, float angle)
{
    float angleRadians = DegreesToRadians(angle);

    sf::Vector2f moveVector = {0, 0};

    if     (GetAsyncKeyState('W'))
    {
        moveVector = {velocity * dt * cos(angleRadians), velocity * dt * sin(angleRadians)};
    }
    else if(GetAsyncKeyState('S'))
    {
        moveVector = {-velocity * dt * cos(angleRadians), -velocity * dt * sin(angleRadians)};
    }

    if     (GetAsyncKeyState('D'))
    {
        moveVector = {moveVector.x + velocity * dt * cos(angleRadians + PI/2), moveVector.y + velocity * dt * sin(angleRadians + PI/2)};
    }
    else if(GetAsyncKeyState('A'))
    {
        moveVector = {moveVector.x + velocity * dt * cos(angleRadians - PI/2), moveVector.y + velocity * dt * sin(angleRadians - PI/2)};
    }

    player->move(moveVector);
}

//-----------------------------------------------------------------------------

void Rotate(sf::CircleShape *player, float velocity, float dt, sf::RenderWindow &window)
{
    float rotateAngle = 0;
    static sf::Vector2f startPosition = GetCursorPosition(window);
           sf::Vector2f nowPosition   = GetCursorPosition(window);

    if     (nowPosition.x - startPosition.x > 0)
    {
        player->rotate((nowPosition.x - startPosition.x) * velocity);

        sf::Mouse::setPosition({100, 100}, window);
        startPosition = GetCursorPosition(window);
    }
    else if(nowPosition.x - startPosition.x < 0)
    {
        player->rotate((nowPosition.x - startPosition.x) * velocity);

        sf::Mouse::setPosition({100, 100}, window);
        startPosition = GetCursorPosition(window);
    }
}
