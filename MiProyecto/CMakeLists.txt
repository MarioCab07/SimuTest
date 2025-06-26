cmake_minimum_required(VERSION 3.15)
project(MiSFMLApp LANGUAGES CXX)

# Indica dónde está SFML
set(SFML_ROOT "$ENV{SFML_ROOT}")

# Busca SFML (versión al menos 2.5)
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Apunta al estándar C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Agrega tu ejecutable
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# Enlaza SFML
target_link_libraries(${PROJECT_NAME}
    PRIVATE sfml-graphics sfml-window sfml-system
)
