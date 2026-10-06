#include <itu_engine.hpp>


using namespace std;
#include <list>

using namespace std;
#include <string>

using namespace std;
#include <map>



const char* const PATH_MUSIC[] =
{
    "data/opengameart.org/music_level_0.ogg",
    "data/opengameart.org/music_level_1.ogg"
};

const char* const PATH_SFXS[] = 
{
    "data/kenney/SFX/footstep00.ogg",
    "data/kenney/SFX/footstep01.ogg"
};

struct Animator // for changing sprites in spritesheet
{
    vec2f texture_size;
    float y_pos; // what height on the sheet are we?
    int start;
    int finish;
    float cur_time = 0;
    float timer_limit = 1;
};

struct Player
{
    vec2f position;
    float velocity;
    float direction;
    Transform2D transform;
    Sprite      sprite;
    Animator *  animator;
    MIX_Track* footstep_track;
};

struct Interpolater
{
    EasingFunction easingFunction;
    vec2f startPos;
    vec2f endPos;
    float time;
};

struct Npc
{
    Transform2D transform;
    Sprite      sprite;
    Interpolater * interpolater;
    vec2f offset;
    vec2f startPos;
    vec2f endPos;
    float speed;
    bool switchy = false;
};

struct GameState
{
    SDL_Texture* tex_atlas_player;
    SDL_Texture* tex_atlas_space;
    SDL_FRect*   player_rect;

    MIX_Mixer* mixer;
    MIX_Audio* audio_music[array_size(PATH_MUSIC)];
    MIX_Audio* audio_sfxs[array_size(PATH_SFXS)];


    MIX_Track* track_main_bg;

    float master_volume = 1;
    float music_volume = 0.15;
    float sound_volume = 0.15;

    int music_current;

    Player player;
    Npc npcs[3];
};

void game_init  (EngineContext* context, GameState* state);
void game_reset (EngineContext* context, GameState* state);
void game_update(EngineContext* context, GameState* state);
void game_render(EngineContext* context, GameState* state);
void game_debug (EngineContext* context, GameState* state);

void update_player(EngineContext* context, GameState* state);
void update_npcs  (EngineContext* context, GameState* state);

int main(void)
{
    bool quit = false;
    EngineConfig  config;
    config.camera_pixel_per_unit = 96;
    config.texture_pixels_per_unit = 96;
    EngineContext context = { 0 };
    GameState     state   = { 0 };

    itu_lib_context_init(&config, &context);

    itu_lib_imgui_setup(&context, false);

    game_init (&context, &state);
    game_reset(&context, &state);

    itu_lib_context_frame_timing_setup(&context);
    while(!quit)
    {
        quit = itu_lib_input_process_events(&context);

        itu_lib_imgui_frame_begin(&context);

        SDL_SetRenderDrawColor(context.renderer, 0x0C, 0x42, 0xA1, 0x00);
        SDL_RenderClear(context.renderer);

        if(context.btn_isjustpressed[BTN_TYPE_DEBUG_RESET])
            game_reset(&context, &state);

        game_update(&context, &state);
        game_render(&context, &state);

        if(context.debug_ui_show)
            game_debug(&context, &state);

        itu_lib_imgui_frame_end(&context);
        SDL_RenderPresent(context.renderer);

        itu_lib_context_frame_timing_update(&context);
    }

    return 0;
}

void game_init(EngineContext* context, GameState* state)
{
    // input bindings
    itu_lib_input_set_mapping_keyboard(context, SDLK_W    , BTN_TYPE_UP);
    itu_lib_input_set_mapping_keyboard(context, SDLK_S    , BTN_TYPE_DOWN);
    itu_lib_input_set_mapping_keyboard(context, SDLK_A    , BTN_TYPE_LEFT);
    itu_lib_input_set_mapping_keyboard(context, SDLK_D    , BTN_TYPE_RIGHT);
    itu_lib_input_set_mapping_keyboard(context, SDLK_SPACE, BTN_TYPE_SPACE);
    itu_lib_input_set_mapping_keyboard(context, SDLK_F1   , BTN_TYPE_DEBUG_F1);
    itu_lib_input_set_mapping_keyboard(context, SDLK_F2   , BTN_TYPE_DEBUG_F2);
    itu_lib_input_set_mapping_keyboard(context, SDLK_F3   , BTN_TYPE_DEBUG_F3);
    itu_lib_input_set_mapping_keyboard(context, SDLK_TAB  , BTN_TYPE_DEBUG_RESET);

    // load resources
    state->tex_atlas_player = itu_resources_texture_create(
        context,
        "data/kenney/character_femalePerson_sheet.png",
        SDL_SCALEMODE_LINEAR
    );
    state->tex_atlas_space = itu_resources_texture_create(
        context,
        "data/kenney/simpleSpace_tilesheet_2.png",
        SDL_SCALEMODE_LINEAR
    );

    SDL_VALIDATE(MIX_Init());
    SDL_VALIDATE(state->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL));
    SDL_VALIDATE(MIX_SetMixerGain(state->mixer, state->master_volume));

    for(Uint64 i = 0; i < array_size(PATH_MUSIC); ++i)
        SDL_VALIDATE(state->audio_music[i] = MIX_LoadAudio(state->mixer, PATH_MUSIC[i], false));

    for(Uint64 i = 0; i < array_size(PATH_SFXS); ++i)
        SDL_VALIDATE(state->audio_sfxs[i] = MIX_LoadAudio(state->mixer, PATH_SFXS[i], false));
    
    SDL_VALIDATE(state->track_main_bg = MIX_CreateTrack(state->mixer));
    SDL_VALIDATE(MIX_SetTrackAudio(state->track_main_bg, state->audio_music[0]));
    SDL_VALIDATE(MIX_SetTrackGain(state->track_main_bg, state->music_volume));
    SDL_VALIDATE(MIX_PlayTrack(state->track_main_bg, 0));
}

void game_reset(EngineContext* context, GameState* state)
{
    // reset context
    context->camera_default.world_position = VEC2F_ZERO;

    Animator * animator = new Animator();

  

    state->player.transform.scale = VEC2F_ONE;
    state->player.transform.position = { 2, -2 };
    SDL_FRect rect = { 0, 0, 96, 128 };
    state->player_rect = &rect;
    itu_lib_sprite_init(&state->player.sprite, state->tex_atlas_player, *state->player_rect);
    SDL_VALIDATE(state->player.footstep_track = MIX_CreateTrack(state->mixer));
    SDL_VALIDATE(MIX_SetTrackAudio(state->player.footstep_track, state->audio_sfxs[0]));
    SDL_VALIDATE(MIX_TagTrack(state->player.footstep_track, "SFX"));
    SDL_VALIDATE(MIX_SetTrackGain(state->player.footstep_track, state->sound_volume));

    animator->start = 0;
    animator->finish = 7;
    animator->cur_time = 0;
    animator->texture_size = {rect.w, rect.h};
    animator->y_pos = 512;
    animator->timer_limit = 1;

    state->player.animator = animator;

    for(int i = 0; i < 3; ++i)
    {
        Interpolater * interpolater = new Interpolater();
        interpolater->time = 0;
        interpolater->easingFunction = EASING_LINEAR;
        
        Npc* npc = &state->npcs[i];
        npc->transform.scale = VEC2F_ONE * 0.5f;
        npc->transform.position = { 0, (float)i };
        itu_lib_sprite_init(&npc->sprite, state->tex_atlas_space, { 5*128.0f, i*128.0f, 128.0f, 128.0f });
        npc->offset = { 3, 0 };
        npc->speed = 1; // units per second
        npc->interpolater = interpolater;

        npc->startPos = npc->transform.position;
        npc->endPos = npc->transform.position + npc->offset;

    }
}

void game_update(EngineContext* context, GameState* state)
{
    update_player(context, state);
    update_npcs  (context, state);
}

void game_render(EngineContext* context, GameState* state)
{
    itu_lib_sprite_render(context, &state->player.sprite, &state->player.transform);

    for(int i = 0; i < 3; ++i)
    {
        Npc* npc = &state->npcs[i];
        itu_lib_sprite_render(context, &npc->sprite, &npc->transform);
    }
}

void game_debug(EngineContext* context, GameState* state)
{
    ImGui::Begin("game", NULL, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowSize(ImVec2(300, 600));
    ImGui::SetWindowPos(ImVec2(0, 0));

    ImGui::SeparatorText("Music");
    {
        if(ImGui::Combo("Music Track", &state->music_current, PATH_MUSIC, array_size(PATH_MUSIC)))
            SDL_VALIDATE(MIX_SetTrackAudio(state->track_main_bg, state->audio_music[state->music_current]));
    
        if(ImGui::DragFloat("Master Volume", &state->master_volume, 0.01, 0, 1))
            SDL_VALIDATE(MIX_SetMixerGain(state->mixer, state->master_volume));

        if(ImGui::DragFloat("Music Volume", &state->music_volume, 0.01, 0, 1))
            SDL_VALIDATE(MIX_SetTrackGain(state->track_main_bg, state->music_volume));

    }

    ImGui::SeparatorText("Player");
    {
        ImGui::PushID(-1);
        ImGui::DragFloat2("Pos", &state->player.transform.position.x);
        ImGui::DragFloat ("Rot", &state->player.transform.rotation);
        ImGui::DragFloat2("Scale", &state->player.transform.scale.x);
        ImGui::PopID();
    }

    for(int i = 0; i < 3; ++i)
    {
        Npc* npc = &state->npcs[i];
        char buf[32];
        SDL_snprintf(buf, 32, "NPC %d", i);
        ImGui::SeparatorText(buf);
        ImGui::PushID(i);
        ImGui::DragFloat2("Pos"  , &npc->transform.position.x);
        ImGui::DragFloat ("Rot"  , &npc->transform.rotation);
        ImGui::DragFloat2("Scale", &npc->transform.scale.x);
        ImGui::DragFloat2("Anim offset", &npc->offset.x);
        ImGui::DragFloat ("Speed", &npc->speed);
        ImGui::Combo("Easing", (int*)&npc->interpolater->easingFunction, easing_names, array_size(easing_names));
        ImGui::PopID();
        
        vec2f pos_a = itu_lib_context_point_global_to_screen(context, npc->transform.position);
        vec2f pos_b = itu_lib_context_point_global_to_screen(context, npc->transform.position + npc->offset);
        itu_lib_render_screen_point(context->renderer, pos_a, 5, COLOR_YELLOW);
        itu_lib_render_screen_point(context->renderer, pos_b, 5, COLOR_YELLOW);
    }

    ImGui::End();
}

float animate(EngineContext* context, Sprite * sprite, Animator * animator, float speed){

    float ratio = animator->cur_time / animator->timer_limit;

    float cur_x = (animator->texture_size.x + animator->start) + 
                animator->texture_size.x * floorf(ratio * (animator->finish - animator->start));
    
    animator->cur_time += context->delta * speed;

    if ( animator->cur_time > animator->timer_limit){
        animator->cur_time = 0;
    }

    sprite->rect = {cur_x, animator->y_pos, animator->texture_size.x, animator->texture_size.y};

    return cur_x;
}

float playtrack(EngineContext* context, MIX_Track * track, float timer, float MAX_timer){

    if (timer > MAX_timer){
        SDL_VALIDATE(MIX_PlayTrack(track, 0));
        return 0;
    }

    return timer - context->delta;
}


void update_player(EngineContext* context, GameState* state)
{
    const float SPEED = 2.0f;
    float dir = 0.0f;
    if(context->btn_isdown[BTN_TYPE_LEFT])  
    {
        dir -= 1.0f;
    }
    if(context->btn_isdown[BTN_TYPE_RIGHT]) {
        dir += 1.0f;
    }

    if (dir < 0){
        state->player.transform.scale.x = -1;
    }
    else{
        state->player.transform.scale.x = 1;
    }
    if (dir != 0){
        float frame = animate(context, &state->player.sprite, state->player.animator, SPEED);
        if (frame == 1 || frame == 3){
            if ( frame == 1){
                SDL_VALIDATE(MIX_SetTrackAudio(state->player.footstep_track, state->audio_sfxs[0]));
            } else{
                SDL_VALIDATE(MIX_SetTrackAudio(state->player.footstep_track, state->audio_sfxs[1]))
            }
            SDL_VALIDATE(MIX_PlayTrack(state->player.footstep_track, 0));
        }
    }
    else{
        state->player.sprite.rect = { 0, 0, 96, 128 };
        state->player.animator->cur_time = 0;
    }

    state->player.direction = dir;
    state->player.velocity = dir * SPEED;

    state->player.transform.position.x += state->player.velocity * context->delta;

}

float interpolate(EngineContext* context, Interpolater * interpolater, Npc * npc){
    
    if(interpolater->time > 1){
        npc->switchy = !npc->switchy;
        interpolater->time = 0;
        return 0;
    }
    float easer = easing(interpolater->time, interpolater->easingFunction);
    interpolater->time += context->delta * npc->speed;
    return easer;

}

void update_npcs(EngineContext* context, GameState* state)
{
    
    for (int i = 0; i < 3; ++i){
        float movement = interpolate(context, state->npcs[i].interpolater, &state->npcs[i]);
        Npc * npc = &state->npcs[i];
        if (npc->switchy){
            npc->transform.position = lerp(npc->startPos, npc->endPos, movement);
        }
        else{
            npc->transform.position = lerp(npc->endPos, npc->startPos, movement);
        }
        state->npcs[i].offset = npc->endPos - npc->transform.position;
    }
    
}
