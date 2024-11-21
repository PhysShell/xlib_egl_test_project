cmake_minimum_required(VERSION 3.10)
project(opengl_triangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим необходимые пакеты
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

# Добавляем исполняемый файл
add_executable(triangle main.cpp)

# Подключаем заголовочные файлы
target_include_directories(triangle PRIVATE 
    ${X11_INCLUDE_DIR}
)

# Подключаем библиотеки
target_link_libraries(triangle PRIVATE 
    ${X11_LIBRARIES}
    OpenGL::GL
    EGL
    GLESv2
)