#include <iostream>
#include <functional>

#include "window.hpp"
#include "context.hpp"
#include "events.hpp"
#include "config.hpp"
#include "timing.hpp"
#include "shader.hpp"
#include "game.hpp"
#include "vfs.hpp"

ttvfs::Root VFS;

GameManager::GameManager()
{

    m_window = new MgCore::Window();
    m_context = new MgCore::Context(3, 2, 0);
    m_events = new MgCore::Events();
    m_clock = new MgCore::FpsTimer();
    m_running = true;

    m_window->make_current(m_context);

    VFS.AddLoader(new ttvfs::DiskLoader);
    VFS.AddArchiveLoader(new ttvfs::VFSZipArchiveLoader);

    VFS.Mount( "../data", "" );

    if(ogl_LoadFunctions() == ogl_LOAD_FAILED)
    {
        std::cout << "Error: glLoadGen failed to load.";
    }

    
    m_lis.handler = std::bind(&GameManager::event_handler, this, std::placeholders::_1);
    m_lis.mask = MgCore::EventType::Quit | MgCore::EventType::WindowSized;


    m_events->add_listener(m_lis);

    m_fpsTime = 0.0;

    m_ss = std::cout.precision();

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    MgCore::ShaderInfo vertInfo {GL_VERTEX_SHADER, "shaders/main.vs"};
    MgCore::ShaderInfo fragInfo {GL_FRAGMENT_SHADER, "shaders/main.fs"};

    MgCore::Shader vertShader(vertInfo);
    MgCore::Shader fragShader(fragInfo);

    m_program = new MgCore::ShaderProgram(&vertShader, &fragShader);
    m_program->use();

    static const GLfloat vertData[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
    };

    m_vertLoc = m_program->vertex_attribute("position");

    glClearColor(0.5, 0.5, 0.5, 1.0);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertData), vertData, GL_STATIC_DRAW);
}

GameManager::~GameManager()
{

    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);

    m_window->make_current(nullptr);
    delete m_window;
    delete m_context;
    delete m_events;
    delete m_clock;
    delete m_program;

}

void GameManager::start()
{

    while (m_running)
    {
        m_fpsTime += m_clock->tick();
        m_events->process();
        
        update();
        render();

        m_window->flip();
        if (m_fpsTime >= 2.0) {
            std::cout.precision (5);
            std::cout << "FPS: " << m_clock->get_fps() << std::endl;
            std::cout.precision (m_ss);
            m_fpsTime = 0;
        }
    }

}

bool GameManager::event_handler(MgCore::Event &event)
{
    MgCore::EventType type = event.type;
    switch(type) {
        case MgCore::EventType::Quit:
            m_running = false;
            break;
        case MgCore::EventType::WindowSized:
            std::cout << event.event.windowSized.width  << " "
                      << event.event.windowSized.height << " "
                      << event.event.windowSized.id     << std::endl;
            break;

    }
    return true;
}

void GameManager::update()
{

}

void GameManager::render()
{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_program->use();

        glEnableVertexAttribArray(m_vertLoc);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glVertexAttribPointer( m_vertLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr );
         
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(m_vertLoc);
}
