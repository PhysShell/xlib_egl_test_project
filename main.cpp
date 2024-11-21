#include <X11/Xlib.h>
#include <X11/Xutil.h>  // Добавляем для XVisualInfo
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <iostream>
#include <vector>
#include <cstring>

// Вершинный шейдер
const char* vertexShaderSource = R"(
    attribute vec4 position;
    void main() {
        gl_Position = position;
    }
)";

// Фрагментный шейдер
const char* fragmentShaderSource = R"(
    precision mediump float;
    void main() {
        gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

// Функция проверки ошибок EGL
void checkEGLError(const char* location) {
    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        std::cerr << "EGL error at " << location << ": 0x" << std::hex << error << std::endl;
    }
}

// Функция для компиляции шейдера
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        std::cerr << "Failed to create shader of type " << type << std::endl;
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::cerr << "Shader compilation failed: " << log.data() << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

int main() {
    // Инициализация X11
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X display\n";
        return 1;
    }

    std::cout << "X11 display opened successfully\n";
    std::cout << "Display name: " << XDisplayString(display) << std::endl;

    // Вывод информации о доступных EGL конфигурациях
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL EGL_DEFAULT_DISPLAY display, trying native display...\n";
        eglDisplay = eglGetDisplay((EGLNativeDisplayType)display);
        return 1;
    }

        if (eglDisplay == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL native display, trying with eglGetPlatformDisplay...\n";
        typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYPROC) (EGLenum platform, void * native_display, const EGLint * attrib_list);
        PFNEGLGETPLATFORMDISPLAYPROC eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
        if (eglGetPlatformDisplayEXT)
        {
            eglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_KHR, (void*) display, NULL);
        }
        else
        {
            std::cerr << "Failed to get eglGetPlatformDisplayEXT\n";
            return 1;
        }
    }

        if (eglDisplay == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display using all methods, suck dicks then...\n";
        return 1;
    }

    std::cout << "Successfully got EGL display\n";

    EGLint majorVersion, minorVersion;
    if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion)) {
        std::cerr << "Failed to initialize EGL\n";
        return 1;
    }

    std::cout << "EGL initialized: version " << majorVersion << "." << minorVersion << std::endl;

    std::cout << "EGL vendor: " <<eglQueryString(eglDisplay, EGL_VENDOR) << std::endl;
    std::cout << "EGL version: " <<eglQueryString(eglDisplay, EGL_VERSION) << std::endl;
    std::cout << "EGL extensions: " <<eglQueryString(eglDisplay, EGL_EXTENSIONS) << std::endl;

    // Выводим поддерживаемые клиентские API
    const char* apis = eglQueryString(eglDisplay, EGL_CLIENT_APIS);
    std::cout << "Supported client APIs: " << apis << std::endl;


    // Конфигурация EGL
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    
    EGLint numConfigs;
    EGLConfig eglConfig;
    if (!eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &numConfigs)) {
        std::cerr << "Failed to choose EGL config\n";
        return 1;
    }

    std::cout << "Found " << numConfigs << " matching EGL configurations\n";

     // Получаем нативный визуал для выбранной конфигурации
    EGLint nativeVisualId;
    eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &nativeVisualId);

    XVisualInfo visualTemplate;
    visualTemplate.visualid = nativeVisualId;
    int numVisuals;
    XVisualInfo* visualInfo = XGetVisualInfo(display, VisualIDMask, &visualTemplate, &numVisuals);

    if (!visualInfo) {
        std::cerr << "Failed to get X visual\n";
        return 1;
    }

    // Создаем окно X11
    Window root = DefaultRootWindow(display);
    XSetWindowAttributes windowAttributes;
    windowAttributes.background_pixel = 0;
    windowAttributes.border_pixel = 0;
    windowAttributes.colormap = XCreateColormap(display, root, visualInfo->visual, AllocNone);
    windowAttributes.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask;

    unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    Window window = XCreateWindow(display, 
                                root,
                                0, 0, 800, 600,
                                0,
                                visualInfo->depth,
                                InputOutput,
                                visualInfo->visual,
                                mask,
                                &windowAttributes);

    XFree(visualInfo);

    // Устанавливаем свойства окна
    XSizeHints* sizeHints = XAllocSizeHints();
    sizeHints->flags = PMinSize | PMaxSize;
    sizeHints->min_width = sizeHints->max_width = 800;
    sizeHints->min_height = sizeHints->max_height = 600;
    XSetWMNormalHints(display, window, sizeHints);
    XFree(sizeHints);

    // Устанавливаем WM_DELETE_WINDOW атом
    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    // Показываем окно
    XMapWindow(display, window);
    XStoreName(display, window, "OpenGL Triangle");

    // Ждем, пока окно станет видимым
    XEvent xev;
    while (1) {
        XNextEvent(display, &xev);
        if (xev.type == MapNotify)
            break;
    }

    // Убеждаемся, что все команды X11 выполнены
    XSync(display, False);

    std::cout << "before eglSurface\n";

    // Создаем поверхность EGL
    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)window, nullptr);
    if (eglSurface == EGL_NO_SURFACE) {
        std::cerr << "Failed to create EGL surface, error: 0x" << std::hex << eglGetError() << std::endl;
        return 1;
    }

    std::cout << "after eglSurface\n";


    // Создание контекста EGL
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (eglContext == EGL_NO_CONTEXT) {
        checkEGLError("eglCreateContext");
        return 1;
    }

    if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        checkEGLError("eglMakeCurrent");
        return 1;
    }

    std::cout << "OpenGL ES version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL ES vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL ES renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Компиляция шейдеров
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (!vertexShader) return 1;
    
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return 1;
    }

    // Создание шейдерной программы
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
        std::cerr << "Shader program linking failed: " << log.data() << std::endl;
        return 1;
    }

    // Вершины треугольника
    GLfloat vertices[] = {
        0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

 while (true) {
        while (XPending(display)) {
            XNextEvent(display, &xev);
            if (xev.type == ClientMessage) {
                if ((Atom)xev.xclient.data.l[0] == wmDeleteMessage) {
                    goto cleanup;  // Выходим из обоих циклов
                }
            }
            if (xev.type == KeyPress)
                goto cleanup;  // Выходим из обоих циклов
        }

        // Отрисовка
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(posAttrib);

        eglSwapBuffers(eglDisplay, eglSurface);
    }

cleanup:
    // Очистка ресурсов
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    eglDestroySurface(eglDisplay, eglSurface);
    eglDestroyContext(eglDisplay, eglContext);
    eglTerminate(eglDisplay);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}