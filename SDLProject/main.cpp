/**
* Author: Isabelle Larson
* Assignment: Lunar Lander
* Date due: 2025-3-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUM_OF_ENEMIES 4
#define FIXED_TIMESTEP 0.0166666f
#define GRAVITY -0.2f
#define NUM_OF_BUILDINGS 3
#define BOARDER_CT 1

#ifdef _WINDOWS
    #include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "Entity.h"
#include <vector>
#include <ctime>
#include "cmath"

struct GameState
{
    Entity* background;
    Entity* pigon;
    Entity* buidlings;
};



// variables again

constexpr int WINDOW_WIDTH  = 640 * 1.5,
              WINDOW_HEIGHT = 480 * 1.5;

constexpr float BG_RED     = 0.0,
                BG_GREEN   = 0.0,
                BG_BLUE    = 0.0,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const int FONTBANK_SIZE = 16;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL    = 0;
constexpr GLint TEXTURE_BORDER     = 0;

enum AppStatus { RUNNING, TERMINATED };

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char BACKGROUND_FILEPATH[] = "background.png",
               BUILDING_FILEPATH[] = "buildings.png",
               PIGON_FILEPATH[] = "pigon.png",
               FONT_FILEPATH[]  = "font1.png";


GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program = ShaderProgram();
GLuint g_font_texture_id;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;


// ADD building locations ADJUST THESE VALUES
glm::vec3 g_building_locations[] = {glm::vec3(-3.2f, -3.0f, 0.0f),
    glm::vec3(0.5f, -3.75f, 0.0f),
    glm::vec3(4.3f, -2.8f, 0.0f)};


void initialise();
void process_input();
void update();
void render();
void shutdown();
//
//-------Collision stuff-------//

bool check_collision(float left1, float right1, float top1, float bottom1,
                     float left2, float right2, float top2, float bottom2)
{
    return (right1 < left2 || left1 > right2 || top1 < bottom2 || bottom1 > top2);
}

// building 1
glm::vec3 building1_pos = g_building_locations[0];
glm::vec3 building_scale = glm::vec3(1.5f, 2.0f, 0.0f);
float building1_left = building1_pos.x - building_scale.x / 2;
float building1_right = building1_pos.x + building_scale.x / 2;
float building1_top = building1_pos.y + building_scale.y / 2;
float building1_bottom = building1_pos.y - building_scale.y / 2;


// building 2
glm::vec3 building2_pos = g_building_locations[1];
float building2_left = building2_pos.x - building_scale.x / 2;
float building2_right = building2_pos.x + building_scale.x / 2;
float building2_top = building2_pos.y + building_scale.y / 2;
float building2_bottom = building2_pos.y - building_scale.y / 2;


// building 3
glm::vec3 building3_pos = g_building_locations[2];
float building3_left = building3_pos.x - building_scale.x / 2;
float building3_right = building3_pos.x + building_scale.x / 2;
float building3_top = building3_pos.y + building_scale.y / 2;
float building3_bottom = building3_pos.y - building_scale.y / 2;


// pigon
glm::vec3 pigon_pos = g_game_state.pigon->get_position();
glm::vec3 pigon_scale = g_game_state.pigon->get_scale();
float pigon_left = pigon_pos.x - pigon_scale.x / 2;
float pigon_right = pigon_pos.x + pigon_scale.x / 2;
float pigon_top = pigon_pos.y + pigon_scale.y / 2;
float pigon_bottom = pigon_pos.y - pigon_scale.y / 2;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
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

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    for (int i = 0; i < text.size(); i++) {
        int spritesheet_index = (int) text[i];
        float offset = (screen_size + spacing) * i;
        
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

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

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pigon Lander",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);


    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    //-------FONT-------//
    g_font_texture_id = load_texture(FONT_FILEPATH);
    
    
    //-------BACKGROUND-------//
    GLuint background_texture_id = load_texture(BACKGROUND_FILEPATH);
    
    g_game_state.background = new Entity(background_texture_id, 1.0f);
    g_game_state.background->set_position(glm::vec3(0.0f));
    g_game_state.background->set_scale(glm::vec3(10.0f,7.5f,0.0f));
    g_game_state.background->update(0.0f);

    

    //-------PIGON-------//
    
    GLuint pigon_texture_id = load_texture(PIGON_FILEPATH);

   
    g_game_state.pigon = new Entity(pigon_texture_id, 1.0f);    // initalize with the texture ID and speed,
    g_game_state.pigon->set_position(glm::vec3(0.0f,3.0f,0.0f));
    g_game_state.pigon->set_movement(glm::vec3(0.0f));
    g_game_state.pigon->set_scale(glm::vec3(0.5f,0.5f,0.0f));
    g_game_state.pigon->set_speed(1.0f);
    g_game_state.pigon->set_accel(glm::vec3(0.0f, GRAVITY, 0.0f));
    g_game_state.pigon->update(0.0f);
    
    
    //-------BUILDINGS-------//
    
    g_game_state.buidlings = new Entity[NUM_OF_BUILDINGS];
    for (int x = 0; x < NUM_OF_BUILDINGS ; x++){
        g_game_state.buidlings[x].set_position(g_building_locations[x]);
        g_game_state.buidlings[x].set_scale(glm::vec3(1.5f, 2.0f, 0.0f));
        g_game_state.buidlings[x].m_texture_id = load_texture(BUILDING_FILEPATH);
        g_game_state.buidlings[x].update(0.0f);
        
    }
    

    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.pigon->set_accel(glm::vec3(0.0f, GRAVITY, 0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q: g_app_status = TERMINATED; break;
                    default: break;
                }

            default:
                break;
        }
    }


    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    
    // if there is fuel left
    if(g_game_state.pigon->get_fuel() >0){
        if(key_state[SDL_SCANCODE_LEFT]){
            g_game_state.pigon->move_left();
            g_game_state.pigon->burn_fuel(0.5f);
          
        }
        else if (key_state[SDL_SCANCODE_RIGHT]){
            g_game_state.pigon->move_right();
            g_game_state.pigon->burn_fuel(0.5f);

        }
        else if (key_state[SDL_SCANCODE_UP]){
            g_game_state.pigon->move_up();
            g_game_state.pigon->burn_fuel(1.0f);
            
        }
        else if (key_state[SDL_SCANCODE_DOWN]){
            g_game_state.pigon->move_down();
           // g_game_state.pigon->burn_fuel(1.0f);
        }
        
        if (glm::length(g_game_state.pigon->get_movement()) > 1.0f ){
            g_game_state.pigon->normalise_movement();
        }
    }
    

  
}

void update()
{
    // --- DELTA TIME CALCULATIONS --- //
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_time_accumulator;
    
    if (delta_time < FIXED_TIMESTEP){
        g_time_accumulator = delta_time;
        return;
    }
   
    while (delta_time>= FIXED_TIMESTEP){
        g_game_state.pigon->update(delta_time);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_time_accumulator = delta_time;
    g_game_state.pigon->update(delta_time);

    

    

}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_game_state.background->render(&g_shader_program);

    g_game_state.pigon->render(&g_shader_program);
    
    
    g_game_state.buidlings[0].render(&g_shader_program);
    g_game_state.buidlings[1].render(&g_shader_program);
    g_game_state.buidlings[2].render(&g_shader_program);
    
    // if collided with building 1
    if (check_collision(building1_left, building1_right, building1_top, building1_bottom, pigon_left, pigon_right, pigon_top, pigon_bottom)){
        draw_text(&g_shader_program, g_font_texture_id, std::string("Success!"),
                       0.8f, -0.3f, glm::vec3(-1.65f, 0.5f, 0.0f));
        g_game_state.pigon->set_velocity(glm::vec3(0.0f));

    }
    
    // if collided with building 2
    else if (check_collision(building2_left, building2_right, building2_top, building2_bottom, pigon_left, pigon_right, pigon_top, pigon_bottom)){

        draw_text(&g_shader_program, g_font_texture_id, std::string("Success!"),
                       0.8f, -0.3f, glm::vec3(-1.65f, 0.5f, 0.0f));
        g_game_state.pigon->set_velocity(glm::vec3(0.0f));

    }
    
    
    // if collided with building 3
    else if(check_collision(building3_left, building3_right, building3_top, building3_bottom, pigon_left, pigon_right, pigon_top, pigon_bottom)){
             draw_text(&g_shader_program, g_font_texture_id, std::string("Success!"),
                       0.8f, -0.3f, glm::vec3(-1.65f, 0.5f, 0.0f));
        g_game_state.pigon->set_velocity(glm::vec3(0.0f));
    }
    

    

    
   

    

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}


