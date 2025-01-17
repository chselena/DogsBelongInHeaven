/**
* Author: Selena Cheung
* Assignment: Rise of the AI
* Date due: 2024-03-30, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 3
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL2_Mixer/SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"

// ————— GAME STATE ————— //
struct GameState
{
    Entity *player;
    Entity *enemies;
    Entity *weapon;
    
    Map *map;
    
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
};

// ————— CONSTANTS ————— //
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char GAME_WINDOW_NAME[] = "Dog in Heaven";

const char V_SHADER_PATH[] = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const char SPRITESHEET_FILEPATH[] = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/assets/dog.png",
            ENEMY_FILEPATH[] = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/assets/choco_bar.png",
           MAP_TILESET_FILEPATH[] = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/assets/nuvem.png",
           BGM_FILEPATH[]         = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/assets/Paradise_Found.mp3",
           JUMP_SFX_FILEPATH[]    = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/assets/poof.mp3",
FONT_FILE_PATH[] = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/assets/font1.png",
BONE_FILE_PATH[] = "/Users/selenacheung/Desktop/game-prog/Rise_OfAI/Simple2DScene/assets/bone.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;
const int FONTBANK_SIZE = 16;
GLuint font_texture_id;
bool won = false;
bool lost = false;
bool summon_bone = false;

unsigned int LEVEL_1_DATA[] =
{
    0, 0, 0, 0, 3, 1, 1, 2, 0, 0, 0, 0, 3, 2,
    0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1, 2,
    3, 2, 0, 0, 0, 3, 1, 2, 0, 0, 0, 0, 0, 0,
    0, 3, 1, 1, 1, 2, 0, 0, 3, 2, 0, 0, 0, 0
};

// ————— VARIABLES ————— //
GameState g_state;
float message_display_time = 0.0f;
SDL_Window* m_display_window;
bool m_game_is_running = true;

ShaderProgram m_program;
glm::mat4 m_view_matrix, m_projection_matrix;

float m_previous_ticks = 0.0f;
float m_accumulator    = 0.0f;

// ————— GENERAL FUNCTIONS ————— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return texture_id;
}

void initialise()
{
    // ————— GENERAL ————— //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    m_display_window = SDL_CreateWindow(GAME_WINDOW_NAME,
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(m_display_window);
    SDL_GL_MakeCurrent(m_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // ————— VIDEO SETUP ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    m_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    m_view_matrix = glm::mat4(1.0f);
    m_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    m_program.set_projection_matrix(m_projection_matrix);
    m_program.set_view_matrix(m_view_matrix);
    
    glUseProgram(m_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    font_texture_id = load_texture(FONT_FILE_PATH);
    
    // ————— MAP SET-UP ————— //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH);
    g_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, 3, 1);
    
    // ————— DOG SET-UP ————— //
    // Existing
    g_state.player = new Entity();
    g_state.player->set_entity_type(PLAYER);
    g_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->set_speed(2.5f);
    g_state.player->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
    // Walking
    g_state.player->m_walking[g_state.player->LEFT]  = new int[4] { 5, 6, 7, 4 };
    g_state.player->m_walking[g_state.player->RIGHT] = new int[4] { 9, 10, 11, 8 };
    g_state.player->m_walking[g_state.player->UP]    = new int[4] { 1, 2, 3, 0 };
    //g_state.player->m_walking[g_state.player->DOWN]  = new int[4] { 13, 14, 15, 16 };

    g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];
    g_state.player->m_animation_frames = 4;
    g_state.player->m_animation_index  = 0;
    g_state.player->m_animation_time   = 0.0f;
    g_state.player->m_animation_cols   = 4;
    g_state.player->m_animation_rows   = 4;
    g_state.player->set_height(0.9f);
    g_state.player->set_width(0.9f);
    
    // Jumping
    g_state.player->m_jumping_power = 5.0f;
    
    // BONE SET_UP //
    g_state.weapon = new Entity();
    g_state.weapon->set_entity_type(WEAPON);
    g_state.weapon->set_position(g_state.player->m_position);
    g_state.weapon->set_movement(glm::vec3(0.0f));
    g_state.weapon->set_speed(2.5f);
    g_state.weapon->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.weapon->m_texture_id = load_texture(BONE_FILE_PATH);
    
    // CHOCOLATE ENEMY //
    GLuint enemy_texture_id = load_texture(ENEMY_FILEPATH);

    g_state.enemies = new Entity[ENEMY_COUNT];
    g_state.enemies[0].set_entity_type(ENEMY);
    g_state.enemies[0].set_ai_type(GUARD);
    g_state.enemies[0].set_ai_state(IDLE);
    g_state.enemies[0].m_texture_id = enemy_texture_id;
    g_state.enemies[0].set_position(glm::vec3(10.0f, 0.0f, 0.0f));
    g_state.enemies[0].set_movement(glm::vec3(0.0f));
    g_state.enemies[0].set_speed(0.5f);
    g_state.enemies[0].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    
    g_state.enemies[1].set_entity_type(ENEMY);
    g_state.enemies[1].set_ai_type(WALKER);
    g_state.enemies[1].set_ai_state(IDLE);
    g_state.enemies[1].m_texture_id = enemy_texture_id;
    g_state.enemies[1].set_position(glm::vec3(3.5f, -1.0f, 0.0f));
    g_state.enemies[1].set_movement(glm::vec3(0.0f));
    g_state.enemies[1].set_speed(0.5f);
    g_state.enemies[1].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    
    g_state.enemies[2].set_entity_type(ENEMY);
    g_state.enemies[2].set_ai_type(JUMPER);
    g_state.enemies[2].set_ai_state(IDLE);
    g_state.enemies[2].m_texture_id = enemy_texture_id;
    g_state.enemies[2].set_position(glm::vec3(8.5f, -1.0f, 0.0f));
    g_state.enemies[2].set_movement(glm::vec3(0.0f));
    g_state.enemies[2].set_speed(0.5f);
    g_state.enemies[2].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.enemies[2].m_jumping_power = 4.0f;

    

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    
    g_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4.0f);
    
    g_state.jump_sfx = Mix_LoadWAV(JUMP_SFX_FILEPATH);
    
    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                m_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        m_game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        
                        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->UP];
                        // Jump
                        if (g_state.player->m_collided_bottom)
                        {
                            g_state.player->m_is_jumping = true;
                            Mix_PlayChannel(-1, g_state.jump_sfx, 0);
            
                        }
                        break;
                        
                    case SDLK_m:
                        // Mute volume
                        Mix_HaltMusic();
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->move_left();
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->move_right();
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];
    }
//    else if (key_state[SDL_SCANCODE_DOWN]){
//        summon_bone = true;
//        g_state.weapon->m_position = glm::vec3(g_state.player->get_position().x + 1.0f, 0.0f, 0.0f);
//    }
    else if (key_state[SDL_SCANCODE_D]){
        summon_bone = true;
        g_state.weapon->m_position = glm::vec3(g_state.player->get_position().x, g_state.player->get_position().y, 0.0f);
        g_state.weapon->move_right();
        
    }
    else if (key_state[SDL_SCANCODE_A]){
        summon_bone = true;
        g_state.weapon->m_position = glm::vec3(g_state.player->get_position().x, g_state.player->get_position().y, 0.0f);
        g_state.weapon->move_left();
    }
    
    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_state.player->m_movement) > 1.0f)
    {
        g_state.player->m_movement = glm::normalize(g_state.player->m_movement);
    }
}

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(m_program.get_position_attribute());
    glVertexAttribPointer(m_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(m_program.get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(m_program.get_position_attribute());
    glDisableVertexAttribArray(m_program.get_tex_coordinate_attribute());
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - m_previous_ticks;
    m_previous_ticks = ticks;
    
    delta_time += m_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        m_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, g_state.player, NULL, 0, g_state.map);
        if (summon_bone){
            g_state.weapon->update(FIXED_TIMESTEP, g_state.weapon, g_state.enemies, 3, g_state.map);
        }
        
        for (int i = 0; i < ENEMY_COUNT; i++) g_state.enemies[i].update(FIXED_TIMESTEP, g_state.enemies, g_state.player, 1, g_state.map);
        
        delta_time -= FIXED_TIMESTEP;
    }
    
    m_accumulator = delta_time;
    
    
    int total_gone = 0;
    for (int i = 0; i < ENEMY_COUNT; i++){
        if (g_state.enemies[i].m_is_active == false){
            total_gone+=1;
        }
    }
    
    if (total_gone == ENEMY_COUNT){
        won = true;
        message_display_time += delta_time;
    }
    if (g_state.player->m_is_active == false){
        lost = true;
        message_display_time += delta_time;
    }
    m_view_matrix = glm::mat4(1.0f);
    m_view_matrix = glm::translate(m_view_matrix, glm::vec3(-g_state.player->get_position().x, 0.0f, 0.0f));
    
    //Check if the delay has passed
    if (message_display_time >= 2.0f)
    {
        m_game_is_running = false;
    }
}


void render()
{
    m_program.set_view_matrix(m_view_matrix);
    
    glClear(GL_COLOR_BUFFER_BIT);
    if (summon_bone){
        g_state.weapon->render(&m_program);
    }
    g_state.player->render(&m_program);
    
    for (int i = 0; i < ENEMY_COUNT; i++)    g_state.enemies[i].render(&m_program);
    g_state.map->render(&m_program);
    
    if (won){
        draw_text(&m_program, font_texture_id, "You Win!", 1.0f, 0.01f, glm::vec3(g_state.player->m_position.x - 3.5f, 0.0f, 0.0f));
    }
    if (lost){
        draw_text(&m_program, font_texture_id, "You Lose", 1.0f, 0.01f, glm::vec3(g_state.player->m_position.x - 3.5f, 0.0f, 0.0f));
    }
    
    SDL_GL_SwapWindow(m_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_state.enemies;
    delete    g_state.player;
    delete    g_state.map;
    delete    g_state.weapon;
    Mix_FreeChunk(g_state.jump_sfx);
    Mix_FreeMusic(g_state.bgm);
}

// ————— GAME LOOP ————— //
int main(int argc, char* argv[])
{
    initialise();
    
    while (m_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
