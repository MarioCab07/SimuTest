#include <SFML/Graphics.hpp>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <cmath>
#include <random>
#include <queue>
#include <set>
#include <thread>

const int CELL_SIZE = 40;

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
bool yaConvertidos = false;
struct Estado
{
    int x, y;
    int turnos;
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
            if (grid[y][x].type == EMPTY && !(x == playerX && y == playerY) &&
                std::abs(x - playerX) + std::abs(y - playerY) != 1)
                vacios.emplace_back(x, y);

    std::shuffle(vacios.begin(), vacios.end(), std::mt19937{std::random_device{}()});
    for (int i = 0; i < std::min((int)vacios.size(), cantidad); ++i)
    {
        grid[vacios[i].second][vacios[i].first].type = WALL;
        grid[vacios[i].second][vacios[i].first].shape.setFillColor(sf::Color::Black);
    }
}
void showPath(std::vector<sf::Vector2i> &caminoFinal, std::vector<std::vector<Cell>> &grid, int gridSize, sf::RenderWindow &window);

bool calcularCamino(std::vector<std::vector<Cell>> &grid, int startX, int startY, sf::RenderWindow &window, int gridSize);

int main()
{
    std::ifstream file("assets/map.txt");
    std::string line;
    std::vector<std::vector<Cell>> grid;
    int playerX = 0, playerY = 0;
    int turnos = 0;
    bool hasCollectedGoal = false, yaSeConvirtieron = false;
    int gridSize = 0;

    while (std::getline(file, line))
    {
        std::vector<Cell> row;
        std::istringstream iss(line);
        std::string token;
        int col = 0;
        while (iss >> token)
        {
            Cell cell;
            if (token == "#")
                cell.type = WALL, cell.shape.setFillColor(sf::Color::Black);
            else if (token == "*")
                cell.type = GOAL, cell.shape.setFillColor(sf::Color::Green);
            else if (token == "x")
                cell.type = EXIT, cell.shape.setFillColor(sf::Color(150, 75, 0));
            else if (token == "?")
                cell.type = GRAVITY_DOWN, cell.shape.setFillColor(sf::Color(135, 206, 250));
            else if (token == "!")
                cell.type = WALL_TOGGLE, cell.shape.setFillColor(sf::Color(105, 105, 105));
            else if (token == "s")
            {
                cell.type = START;
                cell.shape.setFillColor(sf::Color::Yellow);
                playerX = col;
                playerY = grid.size();
            }
            else
                cell.type = EMPTY, cell.shape.setFillColor(sf::Color::White);
            cell.shape.setSize(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
            cell.shape.setPosition(col * CELL_SIZE + 1, grid.size() * CELL_SIZE + 1);
            row.push_back(cell);
            col++;
        }
        grid.push_back(row);
    }

    int rows = grid.size(), cols = grid[0].size();
    gridSize = rows * cols;
    sf::RenderWindow window(sf::VideoMode(cols * CELL_SIZE, rows * CELL_SIZE), "Escape the Grid");

    sf::Font font;
    font.loadFromFile("assets/Roboto-Light.ttf");

    sf::Text turnText("Turno: 0", font, 20);
    turnText.setFillColor(sf::Color::Black);
    turnText.setPosition(10, 10);

    sf::RectangleShape resolveButton(sf::Vector2f(100, 35));
    resolveButton.setFillColor(sf::Color(70, 130, 180));
    resolveButton.setPosition(window.getSize().x - 110, 10);

    sf::Text resolveText("Resolver", font, 16);
    resolveText.setFillColor(sf::Color::White);
    resolveText.setPosition(resolveButton.getPosition().x + 10, resolveButton.getPosition().y + 5);

    sf::CircleShape player(CELL_SIZE / 2 - 5);
    player.setFillColor(sf::Color::Blue);
    player.setPosition(playerX * CELL_SIZE + 5, playerY * CELL_SIZE + 5);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed)
            {
                sf::Vector2f click(event.mouseButton.x, event.mouseButton.y);

                if (resolveButton.getGlobalBounds().contains(click))
                {
                    if (turnos >= 10 && !yaConvertidos)
                    {
                        constexpr int MUROS_A_CONVERTIR = 5;
                        convertirAleatoriosEnMuros(grid, MUROS_A_CONVERTIR, playerX, playerY);
                        yaConvertidos = true;
                    }
                    calcularCamino(grid, playerX, playerY, window, gridSize);
                }

                int mx = click.x / CELL_SIZE, my = click.y / CELL_SIZE;
                if (mx >= 0 && my >= 0 && mx < cols && my < rows)
                {
                    int dx = mx - playerX, dy = my - playerY;
                    bool isAdjacent = (std::abs(dx) == 1 && dy == 0) || (dx == 0 && std::abs(dy) == 1);
                    bool isDownward = (dx == 0 && dy == 1);
                    CellType current = grid[playerY][playerX].type;
                    bool allowed = (current == GRAVITY_DOWN) ? isDownward : isAdjacent;
                    CellType destino = grid[my][mx].type;
                    if (allowed && destino != WALL && !(destino == WALL_TOGGLE && turnos % 2 != 0))
                    {
                        playerX = mx;
                        playerY = my;
                        player.setPosition(playerX * CELL_SIZE + 5, playerY * CELL_SIZE + 5);
                        turnText.setString("Turno: " + std::to_string(++turnos));

                        if (destino == GOAL)
                        {
                            hasCollectedGoal = true;
                            grid[my][mx].type = EMPTY;
                            grid[my][mx].shape.setFillColor(sf::Color::White);
                        }

                        if (destino == EXIT)
                        {
                            if (hasCollectedGoal)
                            {
                                std::cout << "¡Has escapado del laberinto!\n";
                                window.close();
                            }
                            else
                                std::cout << "Aún no has recogido el objetivo.\n";
                        }

                        if (turnos == 10 && !yaSeConvirtieron)
                        {
                            convertirAleatoriosEnMuros(grid, gridSize / 20, playerX, playerY);
                            yaSeConvirtieron = true;
                        }

                        for (int y = 0; y < rows; ++y)
                            for (int x = 0; x < cols; ++x)
                                if (grid[y][x].type == WALL_TOGGLE)
                                    grid[y][x].shape.setFillColor((turnos % 2 == 0) ? sf::Color::White : sf::Color(105, 105, 105));
                    }
                }
            }
        }

        window.clear();
        for (auto &row : grid)
            for (auto &cell : row)
                window.draw(cell.shape);

        window.draw(player);
        window.draw(turnText);
        window.draw(resolveButton);
        window.draw(resolveText);
        window.display();
    }

    return 0;
}

bool calcularCamino(std::vector<std::vector<Cell>> &grid, int startX, int startY, sf::RenderWindow &window, int gridSize)
{
    int rows = grid.size();
    int cols = grid[0].size();

    std::queue<Estado> q;
    std::set<std::tuple<int, int, int, bool>> visitado;
    q.emplace(startX, startY, 0, false, std::vector<sf::Vector2i>{sf::Vector2i(startX, startY)});
    visitado.insert({startX, startY, 0, false});
    std::vector<sf::Vector2i> caminoFinal;

    sf::Vector2i posObjetivo(-1, -1), posSalida(-1, -1);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
        {
            if (grid[y][x].type == GOAL)
                posObjetivo = {x, y};
            if (grid[y][x].type == EXIT)
                posSalida = {x, y};
        }

    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    while (!q.empty())
    {
        Estado actual = q.front();
        q.pop();

        int x = actual.x, y = actual.y, turno = actual.turnos;
        bool tieneObjetivo = actual.tieneObjetivo;
        std::vector<sf::Vector2i> camino = actual.camino;

        CellType tipoActual = grid[y][x].type;
        if (tipoActual == GRAVITY_DOWN)
        {
            int nx = x, ny = y + 1;
            if (ny >= 0 && ny < rows)
            {
                CellType destino = grid[ny][nx].type;
                if (destino == WALL || (destino == WALL_TOGGLE && (turno % 2 != 0)))
                    continue;

                bool recogido = tieneObjetivo || (destino == GOAL);
                if (destino == EXIT && recogido)
                {
                    camino.emplace_back(nx, ny);
                    caminoFinal = camino;
                    showPath(caminoFinal, grid, gridSize, window);
                    return true;
                }

                auto clave = std::make_tuple(nx, ny, (turno + 1) % 2, recogido);
                if (!visitado.count(clave))
                {
                    camino.emplace_back(nx, ny);
                    visitado.insert(clave);
                    q.emplace(nx, ny, turno + 1, recogido, camino);
                }
            }
            continue;
        }

        for (int d = 0; d < 4; ++d)
        {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx < 0 || ny < 0 || nx >= cols || ny >= rows)
                continue;

            CellType destino = grid[ny][nx].type;
            if (destino == WALL || (destino == WALL_TOGGLE && (turno % 2 != 0)))
                continue;

            bool recogido = tieneObjetivo || (destino == GOAL);
            if (destino == EXIT && recogido)
            {
                camino.emplace_back(nx, ny);
                caminoFinal = camino;
                showPath(caminoFinal, grid, gridSize, window);
            }

            auto clave = std::make_tuple(nx, ny, (turno + 1) % 2, recogido);
            if (!visitado.count(clave))
            {
                std::vector<sf::Vector2i> nuevoCamino = camino;
                nuevoCamino.emplace_back(nx, ny);
                visitado.insert(clave);
                q.emplace(nx, ny, turno + 1, recogido, nuevoCamino);
            }
        }
    }

    std::cout << "El laberinto NO puede resolverse." << std::endl;
    return false;
}

void showPath(std::vector<sf::Vector2i> &caminoFinal, std::vector<std::vector<Cell>> &grid, int gridSize, sf::RenderWindow &window)
{
    sf::RectangleShape animador(sf::Vector2f(CELL_SIZE - 10, CELL_SIZE - 10));
    animador.setFillColor(sf::Color(128, 0, 128));

    for (size_t i = 0; i < caminoFinal.size(); i++)
    {
        const auto &p = caminoFinal[i];
        int turnoActual = static_cast<int>(i);

        // Actualiza colores y posición...
        animador.setPosition(p.x * CELL_SIZE + 5, p.y * CELL_SIZE + 5);
        for (int y = 0; y < grid.size(); ++y)
            for (int x = 0; x < grid[0].size(); ++x)
                if (grid[y][x].type == WALL_TOGGLE)
                    grid[y][x].shape.setFillColor((turnoActual % 2 == 0)
                                                      ? sf::Color::White
                                                      : sf::Color(105, 105, 105));

        // Dibuja
        window.clear();
        for (auto &row : grid)
            for (auto &cell : row)
                window.draw(cell.shape);
        window.draw(animador);
        window.display();

        // **Procesa eventos** para que la ventana no quede “congelada”
        sf::Event evt;
        while (window.pollEvent(evt))
        {
            if (evt.type == sf::Event::Closed)
                window.close();
            // (puedes ignorar el resto o añadir manejos extra)
        }

        // Pausa animada
        sf::sleep(sf::milliseconds(500));
    }
}