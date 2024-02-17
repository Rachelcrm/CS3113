/**
* Author: Rachel Chen
 * Assignment: Simple 2D Scene
 * Date due: 2024-02-17, 11:59pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 
 **/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <OpenGL/gl.h>

const char PLAYER_SPRITE[] = "player.png";

GLuint g_player_texture_id;
GLuint g_player_texture2_id;

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

// Set background color which will update over time
float     BG_RED     = 0.1922f,
          BG_BLUE    = 0.549f,
          BG_GREEN   = 0.9059f,
          BG_OPACITY = 1.0f;

const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const int TRIANGLE_RED     = 1.0,
          TRIANGLE_BLUE    = 0.7,
          TRIANGLE_GREEN   = 0.6,
          TRIANGLE_OPACITY = 0.4;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND     = 90.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char PLAYER_SPRITE_FILEPATH[] = "/Users/rachelchen/Desktop/SDLSimple/chr1.png"; // The first texture
const char PLAYER_SPRITE_FILEPATH2[] = "/Users/rachelchen/Desktop/SDLSimple/chr2.png"; // The second texture

SDL_Window* g_display_window;

bool g_game_is_running = true;
bool g_is_growing      = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix,
          g_model_matrix, // Model matrix one
          second_model, // Model matrix two
          g_projection_matrix,
          g_trans_matrix;

float g_triangle_x      = 0.0f;
float g_triangle_rotate = 0.0f;
float g_previous_ticks  = 0.0f;
float g_triangle2_x = 0.0f; // Triangle which forms the second shape
float g_triangle2_rotate = 0.0f;

SDL_Joystick *g_player_one_controller;

// overall position
glm::vec3 g_player_position = glm::vec3(0.0f, 0.0f, 0.0f);

// movement tracker
glm::vec3 g_player_movement = glm::vec3(0.0f, 0.0f, 0.0f);

float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
        case x_coordinate:
            return ((coordinate / WINDOW_WIDTH) * 10.0f ) - (10.0f / 2.0f);
        case y_coordinate:
            return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
        default:
            return 0.0f;
    }
}

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    g_display_window = SDL_CreateWindow("Hello, Textures!",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    g_trans_matrix = g_model_matrix;
    
    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);
    // Notice we haven't set our model matrix yet!
    
    g_program.set_colour(TRIANGLE_RED, TRIANGLE_BLUE, TRIANGLE_GREEN, TRIANGLE_OPACITY);
    
    glUseProgram(g_program.get_program_id());

    
    glClearColor(BG_RED,BG_BLUE, BG_GREEN, BG_OPACITY);
    g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_player_texture2_id = load_texture(PLAYER_SPRITE_FILEPATH2);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_player_movement = glm::vec3(0.0f);
    
    SDL_Event event;
    
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_WINDOWEVENT_CLOSE:
            case SDL_QUIT:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        g_player_movement.x = 1.0f;
                        break;
                    case SDLK_LEFT:
                        g_player_movement.x = -1.0f;
                        break;
                    case SDLK_q:
                        g_game_is_running = false;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }
    
    const Uint8 *key_states = SDL_GetKeyboardState(NULL); // array of key states [0, 0, 1, 0, 0, ...]
    
    if (key_states[SDL_SCANCODE_LEFT])
    {
        g_player_movement.x = -1.0f;
    } else if (key_states[SDL_SCANCODE_RIGHT])
    {
        g_player_movement.x = 1.0f;
    }
    
    if (key_states[SDL_SCANCODE_UP])
    {
        g_player_movement.y = 1.0f;
    } else if (key_states[SDL_SCANCODE_DOWN])
    {
        g_player_movement.y = -1.0f;
    }
    
    if (glm::length(g_player_movement) > 1.0f)
    {
        g_player_movement = glm::normalize(g_player_movement);
    }
}

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;
    
    // change background color over time in a pattern
    BG_RED     += delta_time * (0.5f*sin(ticks*3.0f));
    BG_BLUE    += delta_time * (0.3f*sin(ticks*1.6f));
    BG_GREEN   += delta_time * (0.2f*sin(ticks*3.5f));

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    //Set the first shape's movement and rotation
    g_triangle_x += 0.4f * delta_time; // change location of the first image over time
    g_triangle_rotate += DEGREES_PER_SECOND * delta_time; // 90-degrees per second
    g_model_matrix = glm::mat4(1.0f);

    g_triangle2_x -= 0.15f * delta_time; // change location of the second image over time
    g_triangle2_rotate += DEGREES_PER_SECOND * delta_time;
    float new_x = g_triangle2_x - 0.1f * delta_time; // Move in the negative x direction
    float new_y = g_triangle2_x - 0.1f * delta_time; // negative y direction

    // Make the model move diagonally, not horizontally or verticaly
    g_model_matrix = glm::translate(g_model_matrix, glm::vec3(new_x, new_y, 0.0f));
    g_model_matrix = glm::rotate(g_model_matrix, glm::radians(g_triangle_rotate), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Set the second shape's movement, and build relation with the first one's movement
    float opposite_x = g_triangle2_x + 0.1f * delta_time; // change x coordinate to make the second image not overlapping the first one.
    float opposite_y = g_triangle2_x + 0.1f * delta_time; // change y coordinate
    second_model = glm::translate(g_model_matrix, glm::vec3(opposite_x, opposite_y, 0.0f));
    float scale_factor = 1.0f + 0.5f * sin(ticks);
    second_model = glm::scale(second_model, glm::vec3(scale_factor, scale_factor, 1.0f)); // change the scale of second image over time
}


void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // 6 for 2 trangles
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_program.set_model_matrix(g_model_matrix);
    
    // Locate the first shape
    float vertices[] =
    {
        -0.5f, -0.5f, 1.0f, -0.5f, 1.0f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 1.0f, 0.5f, -0.5f, 0.5f   // triangle 2
    };
    
    // Locate the second shape
    float vertices2[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };
    
    // Locate texture
    float texture_coordinates[] = {
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
            0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
        };
    
    //Set the first shape
    glVertexAttribPointer(g_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_program.get_position_attribute());
    glVertexAttribPointer(g_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_program.get_tex_coordinate_attribute());
    draw_object(g_model_matrix, g_player_texture_id);
    
    //Set the second shape
    g_program.set_model_matrix(second_model);
    glVertexAttribPointer(g_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices2);
    glEnableVertexAttribArray(g_program.get_position_attribute());
    glVertexAttribPointer(g_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_program.get_tex_coordinate_attribute());
    draw_object(second_model, g_player_texture2_id);
    
    // Disable two attribute arrays
    glDisableVertexAttribArray(g_program.get_position_attribute());
    glDisableVertexAttribArray(g_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() {SDL_JoystickClose(g_player_one_controller);
    SDL_Quit();}



int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
