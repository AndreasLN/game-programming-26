#define ENABLE_DIAGNOSTICS

#include <itu_engine.hpp>

#include <gameObjectsLib.hpp>


enum Direction{
	D_LEFT,
	D_RIGHT,
	D_UP,
	D_DOWN,
	D_LAST
};

using namespace std;
#include <list>

using namespace std;
#include <string>

using namespace std;
#include <map>

bool DEBUG_render_textures = true;
bool DEBUG_render_outlines = true;
const int ENTITY_COUNT = 4096;
const int PLAYER_SPEED = 2;


bool operator<(const vec2f& lhs, const vec2f& rhs)
{
    if (lhs.x != rhs.x) {
        return lhs.x < rhs.x;
    } else {
        return lhs.y < rhs.y;
    }
}
bool operator==(const vec2f& lhs, const vec2f& rhs)
{
	if(lhs.x == rhs.x && lhs.y == rhs.y){
   		 return true;
	}
	else{
		return false;
	}
};

SDL_FRect dirt_1 = SDL_FRect{1, 4, 16, 16};

SDL_FRect dirt_2 = SDL_FRect{0, 4, 16, 16};

SDL_FRect guy = SDL_FRect{1, 7, 16, 16};

map<vec2f, SDL_FRect> map_example = {
	{{ -6, 3 }, dirt_2}, {{ -5, 3 }, dirt_2}, {{ -4, 3 }, dirt_1}, {{ -3, 3 }, dirt_1}, {{ -2, 3 }, dirt_2}, {{ -1, 3 }, dirt_1}, {{ 0, 3 }, dirt_2}, {{ 1, 3 }, dirt_1}, {{ 2, 3 }, dirt_1}, {{ 3, 3 }, dirt_1}, {{ 4, 3 }, dirt_2}, {{ 5, 3 }, dirt_2},
	{{ -6, 2 }, dirt_2}, {{ -5, 2 }, dirt_2}, {{ -4, 2 }, dirt_1}, {{ -3, 2 }, dirt_1}, {{ -2, 2 }, dirt_2}, {{ -1, 2 }, dirt_1}, {{ 0, 2 }, dirt_2}, {{ 1, 2 }, dirt_1}, {{ 2, 2 }, dirt_1}, {{ 3, 2 }, dirt_1}, {{ 4, 2 }, dirt_2}, {{ 5, 2 }, dirt_2},
	{{ -6, 1 }, dirt_2}, {{ -5, 1 }, dirt_2}, {{ -4, 1 }, dirt_1}, {{ -3, 1 }, dirt_1}, {{ -2, 1 }, dirt_2}, {{ -1, 1 }, dirt_1}, {{ 0, 1 }, dirt_2}, {{ 1, 1 }, dirt_1}, {{ 2, 1 }, dirt_1}, {{ 3, 1 }, dirt_1}, {{ 4, 1 }, dirt_2}, {{ 5, 1 }, dirt_2},
	{{ -6, 0 }, dirt_1}, {{ -5, 0 }, dirt_1}, {{ -4, 0 }, dirt_1}, {{ -3, 0 }, dirt_1}, {{ -2, 0 }, dirt_1}, {{ -1, 0 }, dirt_1}, {{ 0, 0 }, dirt_2}, {{ 1, 0 }, dirt_1}, {{ 2, 0 }, dirt_2}, {{ 3, 0 }, dirt_1}, {{ 4, 0 }, dirt_1}, {{ 5, 0 }, dirt_1},
	{{ -6, -1}, dirt_1}, {{ -5,-1 }, dirt_1}, {{ -4,-1 }, dirt_1}, {{ -3,-1 }, dirt_1}, {{ -2,-1 }, dirt_1}, {{ -1,-1 }, dirt_1}, {{ 0,-1 }, dirt_2}, {{ 1,-1 }, dirt_1}, {{ 2,-1 }, dirt_2}, {{ 3,-1 }, dirt_1}, {{ 4,-1 }, dirt_1}, {{ 5,-1 }, dirt_1},
	{{ -6,-2 }, dirt_1}, {{ -5,-2 }, dirt_1}, {{ -4,-2 }, dirt_1}, {{ -3,-2 }, dirt_1}, {{ -2,-2 }, dirt_1}, {{ -1,-2 }, dirt_1}, {{ 0,-2 }, dirt_2}, {{ 1,-2 }, dirt_1}, {{ 2,-2 }, dirt_2}, {{ 3,-2 }, dirt_1}, {{ 4,-2 }, dirt_1}, {{ 5,-2 }, dirt_1},
};


struct E06_GameState
{
	// shortcut references

	// game-allocated memory

	vector<MonoBehavior*> starts;
	vector<MonoBehavior*> updates;
	vector<Rendered*> renders;

	// SDL-allocated structures
	SDL_Texture* atlas;
	SDL_Texture* bg;
};

static void create_monobehavior_script(E06_GameState* state, MonoBehavior * script)
{
	state->starts.push_back(script);
	script->Awake();
}

static void add_to_render(E06_GameState* state, Rendered * script)
{
	state->renders.push_back(script);
}



static void Start(E06_GameState* state){
	for (size_t i = 0; i < state->starts.size(); ++i)
	{
		state->starts[i]->Start();
		state->updates.push_back(state->starts[i]);
	}

	state->starts.clear();
}

static void Update(EngineContext* context, E06_GameState* state){
	for (size_t i = 0; i < state->updates.size(); ++i)
	{
		state->updates[i]->Update(context->delta);
	}
	
}

static void Render(EngineContext* context, E06_GameState* state){
	for (size_t i = 0; i < state->renders.size(); i++)
	{
		state->renders[i]->Render(context);
	}
}

static void game_init(EngineContext* context, E06_GameState* state)
{

	// TODO allocate space for tile info (when we'll load those from file)
	// texture atlases
	state->atlas = itu_resources_texture_create(context, "data/kenney/tiny_dungeon_packed.png", SDL_SCALEMODE_NEAREST);
	state->bg    = itu_resources_texture_create(context, "data/kenney/prototype_texture_dark/texture_13.png", SDL_SCALEMODE_NEAREST);

	state->updates = { };
	state->starts = { };
}

static void mouse_tint(EngineContext* context, E06_GameState* state){
		// TINT WITH MOUSE
		vec2f point = itu_lib_context_point_screen_to_global(context, context->mouse_pos);
		try
		{
			//auto entity = state->tilemap->entities.at(vec2f{floorf(point.x), floorf(point.y)});
			//entity->sprite.tint.r = 100;
		}
		catch(const std::exception& e)
		{
		}
}

static void game_reset(EngineContext* context, E06_GameState* state)
{
	
	
	GameObject * root = new GameObject(nullptr, "root", {0,0});

	// Background
	{
		GameObject * bg = new GameObject(root, "bg", {0,0});
		bg->transform.scale = VEC2F_ONE;
		bg->transform.rotation = 0;
		bg->transform.position = VEC2F_ZERO;
		
		Sprite2D * bg_sprite2d = new Sprite2D(*bg, state->bg, {0, 0, 1024, 1024});
		create_monobehavior_script(state, bg_sprite2d);
		add_to_render(state, bg_sprite2d);
	}

	// Tilemap
	{
		GameObject * tilemap = new GameObject(root, "tilemap", {0,0});
		tilemap->transform.scale = VEC2F_ONE;
		tilemap->transform.rotation = 0;
		tilemap->transform.position = VEC2F_ZERO;

		Tilemap * tilemap_sprite = new Tilemap(*tilemap, state->atlas, map_example);
		create_monobehavior_script(state, tilemap_sprite);
		add_to_render(state, tilemap_sprite);
	}

	//player
	{
		GameObject * child = new GameObject(root, "child", {0,0});

		
		Sprite2D * sprite2d = new Sprite2D(*child, state->atlas, {1, 10, 16, 16});
		create_monobehavior_script(state, sprite2d);
		add_to_render(state, sprite2d);
		
		child->transform.position = VEC2F_ZERO;
		child->transform.scale = VEC2F_ONE;
		child->transform.rotation = 0;	

		Player * playerComponent = new Player(*child, PLAYER_SPEED, context);
		create_monobehavior_script(state, playerComponent);

		sprite2d->Print();

	}
	

	// NPC
	{
		for(int i = 0 ; i < 5; ++i){
			GameObject * npc = new GameObject(root, "child" + i, {0,0});

			npc->transform.position = VEC2F_ZERO;
			npc->transform.scale = VEC2F_ONE;
			npc->transform.rotation = 0;	
			
			Sprite2D * npc_sprite2d = new Sprite2D(*npc, state->atlas, {guy.x, guy.y, guy.w, guy.h});
			create_monobehavior_script(state, npc_sprite2d);
			add_to_render(state, npc_sprite2d);
			
			NPC * ncpComponent = new NPC(*npc, 1.5, context);
			create_monobehavior_script(state, ncpComponent);

		}
	}

}

static void game_update(EngineContext* context, E06_GameState* state)
{
	{
		Update(context, state);
	}

	const float npc_speed = 1.5;

}

static void game_render(EngineContext* context, E06_GameState* state)
{
	Render(context, state);

	// debug window
	SDL_SetRenderDrawColor(context->renderer, 0xFF, 0x00, 0xFF, 0xff);
	SDL_RenderRect(context->renderer, NULL);
}

int main(void)
{
	EngineConfig config;
	config.application_name = "ES03 - Coordinate Systems";
	config.texture_pixels_per_unit = 16;
	config.camera_pixel_per_unit = 64;
	config.step_per_second_fluid = 60;

	bool quit = false;
	SDL_Window* window;
	EngineContext context = { 0 };
	E06_GameState  state   = {  };

	itu_lib_context_init(&config, &context);
	itu_lib_context_set_active_camera(&context, &context.camera_default);


	game_init(&context, &state);
	game_reset(&context, &state);
	
	Start(&state);

	itu_lib_context_frame_timing_setup(&context);

	while(!quit)
	{
		mouse_tint(&context, &state);

		// input
		SDL_Event event;
		itu_lib_input_clear(&context);
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_EVENT_QUIT:
					quit = true;
					break;
				case SDL_EVENT_MOUSE_MOTION:
				{
	
					vec2f world_point = itu_lib_context_point_screen_to_window(&context, context.mouse_pos);
					
					context.mouse_pos.x = event.motion.x;
					context.mouse_pos.y = event.motion.y;
					vec2f point = itu_lib_context_point_screen_to_global(&context, context.mouse_pos);
										
					context.mouse_delta.x = event.motion.xrel;
					context.mouse_delta.y = event.motion.yrel;
					break;
				}
				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP:
					switch(event.key.key)
					{
						case SDLK_W: itu_lib_input_key_process(&context, BTN_TYPE_UP, &event);        break;
						case SDLK_A: itu_lib_input_key_process(&context, BTN_TYPE_LEFT, &event);      break;
						case SDLK_S: itu_lib_input_key_process(&context, BTN_TYPE_DOWN, &event);      break;
						case SDLK_D: itu_lib_input_key_process(&context, BTN_TYPE_RIGHT, &event);     break;
						case SDLK_Q: itu_lib_input_key_process(&context, BTN_TYPE_ACTION_0, &event);  break;
						case SDLK_E: itu_lib_input_key_process(&context, BTN_TYPE_ACTION_1, &event);  break;
						case SDLK_SPACE: itu_lib_input_key_process(&context, BTN_TYPE_SPACE, &event); break;
					}

					// debug keys
					if(event.key.down && !event.key.repeat)
					{
						switch(event.key.key)
						{
							case SDLK_TAB: game_reset(&context, &state); break;
							case SDLK_F1: DEBUG_render_textures = !DEBUG_render_textures; break;
							case SDLK_F2: DEBUG_render_outlines = !DEBUG_render_outlines; break;
						}
					}
					break;
			}
		}

		SDL_SetRenderDrawColor(context.renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(context.renderer);

		// update
		game_update(&context, &state);
		game_render(&context, &state);
		
#ifdef ENABLE_DIAGNOSTICS
		{
			SDL_SetRenderDrawColor(context.renderer, 0x0, 0x00, 0x00, 0xCC);
			SDL_FRect rect = SDL_FRect{ 5, 5, 225, 55 };
			SDL_RenderFillRect(context.renderer, &rect);

			SDL_SetRenderDrawColor(context.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDebugTextFormat(context.renderer, 10, 10, "work: %9.6f ms/f", (float)context.elapsed_work  / (float)MILLIS(1));
			SDL_RenderDebugTextFormat(context.renderer, 10, 20, "tot : %9.6f ms/f", (float)context.elapsed_frame / (float)MILLIS(1));
			SDL_RenderDebugTextFormat(context.renderer, 10, 30, "[TAB] reset ");
			SDL_RenderDebugTextFormat(context.renderer, 10, 40, "[F1]  render textures   %s", DEBUG_render_textures   ? " ON" : "OFF");
			SDL_RenderDebugTextFormat(context.renderer, 10, 50, "[F2]  render outlines   %s", DEBUG_render_outlines   ? " ON" : "OFF");
		}
#endif
		// render
		SDL_RenderPresent(context.renderer);

		itu_lib_context_frame_timing_update(&context);
	}
}
