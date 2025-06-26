#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <cmath>
#include <random>
#include <queue>
#include <set>

const int CELL_SIZE = 25;

enum CellType
{
    EMPTY,
    WALL,
    GOAL,
    EXIT,
    GRAVITY_DOWN,
    START,
    WALL_TOGGLE
};

struct Cell
{
    CellType type;
    sf::RectangleShape shape;
};

bool murosConvertidos = false;

struct Estado
{
    int x, y, turnos;
    bool tieneObjetivo;
    std::vector<sf::Vector2i> camino;
    Estado(int _x, int _y, int _turnos, bool _tieneObjetivo, std::vector<sf::Vector2i> _camino)
        : x(_x), y(_y), turnos(_turnos), tieneObjetivo(_tieneObjetivo), camino(std::move(_camino)) {}
};

void convertirAleatoriosEnMuros(std::vector<std::vector<Cell>> &grid, int cantidad, int playerX, int playerY)
{
    std::vector<std::pair<int, int>> vacios;
    for (int y = 0; y < grid.size(); ++y)
        for (int x = 0; x < grid[y].size(); ++x)
            if (grid[y][x].type == EMPTY && !(x == playerX && y == playerY) && std::abs(x - playerX) + std::abs(y - playerY) != 1)
                vacios.emplace_back(x, y);
    std::shuffle(vacios.begin(), vacios.end(), std::mt19937{std::random_device{}()});
    for (int i = 0; i < cantidad && i < vacios.size(); ++i)
    {
        auto [vx, vy] = vacios[i];
        grid[vy][vx].type = WALL;
        grid[vy][vx].shape.setFillColor(sf::Color::Black);
    }
}

void showPath(const std::vector<sf::Vector2i> &caminoFinal, std::vector<std::vector<Cell>> &grid, sf::RenderWindow &window)
{
    sf::RectangleShape anim(sf::Vector2f(CELL_SIZE - 10, CELL_SIZE - 10));
    anim.setFillColor(sf::Color(128, 0, 128));
    for (int i = 0; i < caminoFinal.size(); ++i)
    {
        for (int y = 0; y < grid.size(); ++y)
            for (int x = 0; x < grid[0].size(); ++x)
                if (grid[y][x].type == WALL_TOGGLE)
                    grid[y][x].shape.setFillColor((i % 2 == 0) ? sf::Color::White : sf::Color(105, 105, 105));
        window.clear();
        for (auto &row : grid)
            for (auto &c : row)
                window.draw(c.shape);
        anim.setPosition(caminoFinal[i].x * CELL_SIZE + 5, caminoFinal[i].y * CELL_SIZE + 5);
        window.draw(anim);
        window.display();
        sf::Event e;
        while (window.pollEvent(e))
            if (e.type == sf::Event::Closed)
                window.close();
        sf::sleep(sf::milliseconds(500));
    }
}

bool calcularCamino(std::vector<std::vector<Cell>> &grid, int startX, int startY, sf::RenderWindow &window)
{
    int rows = grid.size(), cols = grid[0].size();
    std::queue<Estado> q;
    std::set<std::tuple<int, int, int, bool>> visit;
    q.emplace(startX, startY, 0, false, std::vector<sf::Vector2i>{{startX, startY}});
    visit.insert({startX, startY, 0, false});
    std::vector<sf::Vector2i> finalCamino;
    sf::Vector2i posGoal(-1, -1), posExit(-1, -1);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
        {
            if (grid[y][x].type == GOAL)
                posGoal = {x, y};
            if (grid[y][x].type == EXIT)
                posExit = {x, y};
        }
    const int dx[4] = {0, 0, -1, 1}, dy[4] = {-1, 1, 0, 0};
    while (!q.empty())
    {
        auto cur = q.front();
        q.pop();
        int x = cur.x, y = cur.y, t = cur.turnos;
        bool got = cur.tieneObjetivo;
        auto path = cur.camino;
        CellType tp = grid[y][x].type;
        if (tp == GRAVITY_DOWN)
        {
            int nx = x, ny = y + 1;
            if (ny >= 0 && ny < rows)
            {
                CellType dest = grid[ny][nx].type;
                if (dest != WALL && !(dest == WALL_TOGGLE && t % 2 != 0))
                {
                    bool ng = got || (dest == GOAL);
                    if (dest == EXIT && ng)
                    {
                        path.emplace_back(nx, ny);
                        finalCamino = path;
                        showPath(finalCamino, grid, window);
                        return true;
                    }
                    auto key = std::make_tuple(nx, ny, (t + 1) % 2, ng);
                    if (!visit.count(key))
                    {
                        path.emplace_back(nx, ny);
                        visit.insert(key);
                        q.emplace(nx, ny, t + 1, ng, path);
                    }
                }
            }
        }
        else
        {
            for (int d = 0; d < 4; ++d)
            {
                int nx = x + dx[d], ny = y + dy[d];
                if (nx < 0 || ny < 0 || nx >= cols || ny >= rows)
                    continue;
                CellType dest = grid[ny][nx].type;
                if (dest == WALL || (dest == WALL_TOGGLE && t % 2 != 0))
                    continue;
                bool ng = got || (dest == GOAL);
                if (dest == EXIT && ng)
                {
                    path.emplace_back(nx, ny);
                    finalCamino = path;
                    showPath(finalCamino, grid, window);
                    return true;
                }
                auto key = std::make_tuple(nx, ny, (t + 1) % 2, ng);
                if (!visit.count(key))
                {
                    auto newPath = path;
                    newPath.emplace_back(nx, ny);
                    visit.insert(key);
                    q.emplace(nx, ny, t + 1, ng, newPath);
                }
            }
        }
    }
    std::cout << "El laberinto NO puede resolverse.\n";
    return false;
}

int main()
{
    std::ifstream f("assets/map.txt");
    if (!f)
    {
        std::cerr << "ERROR: no puedo abrir assets/map.txt\n";
        return -1;
    }
    std::string line;
    std::vector<std::vector<Cell>> grid;
    int playerX = 0, playerY = 0, turnos = 0;
    while (std::getline(f, line))
    {
        std::istringstream iss(line);
        std::vector<Cell> row;
        std::string tok;
        int col = 0;
        while (iss >> tok)
        {
            Cell c;
            if (tok == "#")
            {
                c.type = WALL;
                c.shape.setFillColor(sf::Color::Black);
            }
            else if (tok == "*")
            {
                c.type = GOAL;
                c.shape.setFillColor(sf::Color::Green);
            }
            else if (tok == "x")
            {
                c.type = EXIT;
                c.shape.setFillColor(sf::Color(150, 75, 0));
            }
            else if (tok == "?")
            {
                c.type = GRAVITY_DOWN;
                c.shape.setFillColor(sf::Color(135, 206, 200));
            }
            else if (tok == "!")
            {
                c.type = WALL_TOGGLE;
                c.shape.setFillColor(sf::Color(105, 105, 105));
            }
            else if (tok == "s")
            {
                c.type = START;
                c.shape.setFillColor(sf::Color::Yellow);
                playerX = col;
                playerY = grid.size();
            }
            else
            {
                c.type = EMPTY;
                c.shape.setFillColor(sf::Color::White);
            }
            c.shape.setSize({CELL_SIZE - 2, CELL_SIZE - 2});
            c.shape.setPosition(col * CELL_SIZE + 1, grid.size() * CELL_SIZE + 1);
            row.push_back(c);
            ++col;
        }
        grid.push_back(row);
    }
    int rows = grid.size(), cols = grid[0].size();
    sf::RenderWindow window({cols * CELL_SIZE, rows * CELL_SIZE}, "Escape the Grid");
    sf::Font font;
    font.loadFromFile("assets/Roboto-Light.ttf");
    sf::Text turnText("Turno: 0", font, 20);
    turnText.setFillColor(sf::Color::Black);
    turnText.setPosition(10, 10);
    sf::RectangleShape btn({100, 35});
    btn.setFillColor(sf::Color(70, 130, 180));
    btn.setPosition(window.getSize().x - 110, 10);
    sf::Text btnText("Resolver", font, 16);
    btnText.setFillColor(sf::Color::White);
    btnText.setPosition(btn.getPosition().x + 10, btn.getPosition().y + 5);
    sf::CircleShape player(CELL_SIZE / 2 - 5);
    player.setFillColor(sf::Color::Blue);
    player.setPosition(playerX * CELL_SIZE + 5, playerY * CELL_SIZE + 5);
    while (window.isOpen())
    {
        sf::Event e;
        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed)
                window.close();
            if (e.type == sf::Event::MouseButtonPressed)
            {
                sf::Vector2f cl(e.mouseButton.x, e.mouseButton.y);
                if (btn.getGlobalBounds().contains(cl))
                {
                    if (!murosConvertidos)
                    {
                        convertirAleatoriosEnMuros(grid, 5, playerX, playerY);
                        murosConvertidos = true;
                    }
                    calcularCamino(grid, playerX, playerY, window);
                }
                int mx = e.mouseButton.x / CELL_SIZE;
                int my = e.mouseButton.y / CELL_SIZE;
                if (mx >= 0 && my >= 0 && mx < cols && my < rows)
                {
                    int dx = mx - playerX;
                    int dy = my - playerY;
                    bool adj = (std::abs(dx) == 1 && dy == 0) || (dx == 0 && std::abs(dy) == 1);
                    bool down = (dx == 0 && dy == 1);
                    CellType cur = grid[playerY][playerX].type;
                    bool allowed = (cur == GRAVITY_DOWN) ? down : adj;
                    CellType dest = grid[my][mx].type;
                    if (allowed && dest != WALL && !(dest == WALL_TOGGLE && turnos % 2 != 0))
                    {
                        playerX = mx;
                        playerY = my;
                        player.setPosition(playerX * CELL_SIZE + 5, playerY * CELL_SIZE + 5);
                        turnText.setString("Turno: " + std::to_string(++turnos));
                        if (dest == GOAL)
                        {
                            grid[my][mx].type = EMPTY;
                            grid[my][mx].shape.setFillColor(sf::Color::White);
                        }
                    }
                }
            }
        }
        window.clear();
        for (auto &r : grid)
            for (auto &c : r)
                window.draw(c.shape);
        window.draw(player);
        window.draw(turnText);
        window.draw(btn);
        window.draw(btnText);
        window.display();
    }
    return 0;
}
