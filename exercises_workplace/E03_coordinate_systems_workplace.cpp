#define ENABLE_DIAGNOSTICS

#include <itu_engine.hpp>

using namespace std;
#include <list>

using namespace std;
#include <string>

using namespace std;
#include <map>

bool DEBUG_render_textures = true;
bool DEBUG_render_outlines = true;
const int ENTITY_COUNT = 4096;

enum Direction{
	LEFT,
	RIGHT,
	UP,
	DOWN,
	LAST
};

enum NPCSTATE{
	MOVING,
	IDLE,
};

struct E03_Entity
{
	Sprite sprite;
	Transform2D transform;

	E03_Entity() {
	}

	E03_Entity(Sprite sprite, Transform2D  transform) 
	: sprite(sprite), transform(transform) {}

};

struct E03_Character_Entity: public E03_Entity
{
	Direction direction;

	E03_Character_Entity() {}

	E03_Character_Entity(Sprite  sprite, Transform2D  transform, Direction direction = DOWN)
	: E03_Entity(sprite, transform), direction(direction) {}

};

struct Random_Timer
{
	float min;
	float max;

	float time = 2;

	Random_Timer(float min, float max) 
	: min(min), max(max) {time = 2;}

};

struct E03_CharacterNPC_Entity: public E03_Character_Entity
{
	Random_Timer * idle_timer;
	Random_Timer * moving_timer;
	NPCSTATE state;

	E03_CharacterNPC_Entity() {}

	E03_CharacterNPC_Entity(Sprite sprite, Transform2D transform, Direction direction, Random_Timer * idle, Random_Timer * moving)
	: E03_Character_Entity(sprite, transform, direction), idle_timer(idle), moving_timer(moving) {state = IDLE;}
};




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

struct E03_TileMap
{
	map<vec2f, E03_Entity*> entities; // easier lookup for entities in tilemap;
	map<vec2f, SDL_FRect> map; // Each sprite has unique transform
	Transform2D transform;
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


struct E03_GameState
{
	// shortcut references
	E03_Character_Entity* player;
	E03_TileMap* tilemap;

	// game-allocated memory
	list<E03_Entity*> entities;
	list<E03_CharacterNPC_Entity*> npcs;
	int entities_alive_count;

	// SDL-allocated structures
	SDL_Texture* atlas;
	SDL_Texture* bg;
};

static void entity_create(E03_GameState* state, E03_Entity * entity)
{
	if(!(state->entities_alive_count < ENTITY_COUNT))
		// NOTE: this might as well be an assert, if we don't have a way to recover/handle it
		return;

	// // concise version
	//return &state->entities[state->entities_alive_count++];

	state->entities.push_back(entity);
	
	++state->entities_alive_count;
	return;
}

// NOTE: this only works if nobody holds references to other entities!
//       if that were the case, we couldn't swap them around.
//       We will see in later lectures how to handle this kind of problems
static void entity_destroy(E03_GameState* state, E03_Entity* entity)
{
	// NOTE: here we want to fail hard, nobody should pass us a pointer not gotten from `entity_create()`
	
	--state->entities_alive_count;
	state->entities.remove(entity);
}

static void game_init(EngineContext* context, E03_GameState* state)
{
	// allocate memory
	state->entities = { };

	// TODO allocate space for tile info (when we'll load those from file)
	// texture atlases
	state->atlas = itu_resources_texture_create(context, "data/kenney/tiny_dungeon_packed.png", SDL_SCALEMODE_NEAREST);
	state->bg    = itu_resources_texture_create(context, "data/kenney/prototype_texture_dark/texture_13.png", SDL_SCALEMODE_NEAREST);
}

static void mouse_tint(EngineContext* context, E03_GameState* state){
		// TINT WITH MOUSE
		vec2f point = itu_lib_context_point_screen_to_global(context, context->mouse_pos);
		try
		{
			auto entity = state->tilemap->entities.at(vec2f{floorf(point.x), floorf(point.y)});
			entity->sprite.tint.r = 100;
		}
		catch(const std::exception& e)
		{
		}
}

static void game_reset(EngineContext* context, E03_GameState* state)
{
	state->entities_alive_count = 0;
	// entities
	{
		E03_Entity * bg = new E03_Entity();

		entity_create(state, bg);
		SDL_FRect sprite_rect = SDL_FRect{ 0, 0, 1024, 1024};
		itu_lib_sprite_init(
			&bg->sprite,
			state->bg,
			itu_lib_sprite_get_source_rect(0, 0, 1024, 1024)
		);
		bg->transform.scale = VEC2F_ONE;
		bg->transform.rotation = 0;
		bg->transform.position = VEC2F_ZERO;
	}

	// TILEMAP
	{
		state->tilemap = new E03_TileMap();
		state->tilemap->transform.position = VEC2F_ZERO;
		state->tilemap->transform.scale = VEC2F_ONE;
		state->tilemap->map = map_example;
		state->tilemap->entities = {};

		for(auto pair : state->tilemap->map){
			E03_Entity* entity = new E03_Entity();
			entity_create(state, entity);
			entity->transform.scale = state->tilemap->transform.scale;
			entity->transform.position = state->tilemap->transform.position + pair.first;
			entity->transform.rotation = 0;

			itu_lib_sprite_init(
				&entity->sprite,
				state->atlas,
				itu_lib_sprite_get_source_rect(pair.second.x, pair.second.y, pair.second.w, pair.second.h)
			);
			entity->sprite.pivot.y = 0.0f;
			entity->sprite.pivot.x = 0.0f;
			state->tilemap->entities[pair.first] = entity;
		}
	}

	
	// NPC
	{
		for(int i = 0 ; i < 10; ++i){
			Random_Timer * idle = new Random_Timer(1, 3);
			Random_Timer * moving = new Random_Timer(2, 4);
			E03_CharacterNPC_Entity* npc = new E03_CharacterNPC_Entity();
			entity_create(state, npc);
			npc->idle_timer = idle;
			npc->moving_timer = moving;
			npc->transform.scale = VEC2F_ONE;
			npc->transform.position = VEC2F_ONE;
			npc->transform.rotation = 0;
			npc->state = IDLE;
			itu_lib_sprite_init(
				&npc->sprite,
				state->atlas,
				itu_lib_sprite_get_source_rect(guy.x, guy.y, guy.w, guy.h)
			);
			SDL_Log("%d", npc->state);
			//npc->sprite.pivot.y = 0.3f;
			state->npcs.push_front(npc);
		}
	}


	{
		state->player = new E03_Character_Entity();
		entity_create(state, state->player);
		state->player->transform.position = VEC2F_ZERO;
		state->player->transform.scale = VEC2F_ONE;
		state->player->transform.rotation = 0;
		itu_lib_sprite_init(
			&state->player->sprite,
			state->atlas,
			itu_lib_sprite_get_source_rect(1, 10, 16, 16)
		);

		// raise sprite a bit, so that the position concides with the center of the image
		state->player->sprite.pivot.y = 0.3f;
	}
}

static void game_update(EngineContext* context, E03_GameState* state)
{
	{
		const float player_speed = 2;

		E03_Entity* entity = state->player;
		vec2f mov = { 0 };
		if(context->btn_isdown_up){
			mov.y += 1;
			state->player->direction = UP;
		}		
		if(context->btn_isdown_down){
			mov.y -= 1;
			state->player->direction = DOWN;
		}	
		if(context->btn_isdown_left){
			mov.x -= 1;
			state->player->direction = LEFT;	
		}	
		if(context->btn_isdown_right){
			mov.x += 1;
			state->player->direction = RIGHT;
		}
				
		if(state->player->direction == LEFT && !state->player->sprite.flip_horizontal){
			state->player->sprite.flip_horizontal = true;
		}
		else if (state->player->direction == RIGHT && state->player->sprite.flip_horizontal){
			state->player->sprite.flip_horizontal = false;
		}

		entity->transform.position = entity->transform.position + mov * (player_speed * context->delta);

		// camera follows player
		context->camera_active->world_position = entity->transform.position;
	}

	const float npc_speed = 1.5;

	for(E03_CharacterNPC_Entity * npc : state->npcs){
		vec2f mov = { 0 };
		//SDL_Log("%d", npc->state);
		switch (npc->state)
		{
			case IDLE:

				if (npc->idle_timer->time > 0.0f){
					npc->idle_timer->time -= context->delta;
				}
				else{
					npc->state = MOVING;
					npc->moving_timer->time = npc->moving_timer->max + SDL_randf() * (npc->moving_timer->min - npc->moving_timer->max);
					npc->direction = Direction(SDL_rand(LAST - 1));

				}
				break;
			case MOVING:
				if (npc->moving_timer->time > 0){
					switch (npc->direction)
					{
					case UP:
						mov.y += 1;
						break;
					case DOWN:
						mov.y -= 1;
						break;
					case LEFT:
						mov.x -= 1;
						break;
					case RIGHT:
						mov.x += 1;
						break;
					default:
						break;
					}
					if(npc->direction == LEFT && !npc->sprite.flip_horizontal){
						npc->sprite.flip_horizontal = true;
					}
					else if (npc->direction == RIGHT && npc->sprite.flip_horizontal){
						npc->sprite.flip_horizontal = false;
					}

					npc->transform.position = npc->transform.position + mov * (npc_speed * context->delta);
					npc->moving_timer->time -= context->delta;
				}
				else{
					npc->state = IDLE;
					npc->idle_timer->time = npc->idle_timer->max + SDL_randf() * (npc->idle_timer->min - npc->idle_timer->max);
				}
				break;

			default:
				break;
			}
	}

}

static void game_render(EngineContext* context, E03_GameState* state)
{
	for(E03_Entity * entity : state->entities)
	{
		// render texture
		SDL_FRect rect_src = entity->sprite.rect;
		SDL_FRect rect_dst;

		if(DEBUG_render_textures)
			itu_lib_sprite_render(context, &entity->sprite, &entity->transform);

		if(DEBUG_render_outlines)
			itu_lib_sprite_render_debug(context, &entity->sprite, &entity->transform);
		
		entity->sprite.tint.r = 1;
	}

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
	E03_GameState  state   = { 0 };

	itu_lib_context_init(&config, &context);
	itu_lib_context_set_active_camera(&context, &context.camera_default);


	game_init(&context, &state);
	game_reset(&context, &state);

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
					//SDL_Log("WORLD: MOUSE POS: X, %f, POS: Y, %f", world_point.x, world_point.y);
					//SDL_Log("CAMERA: MOUSE POS: X, %f, POS: Y, %f", context.mouse_pos.x, context.mouse_pos.y);
					context.mouse_pos.x = event.motion.x;
					context.mouse_pos.y = event.motion.y;
					vec2f point = itu_lib_context_point_screen_to_global(&context, context.mouse_pos);
					//SDL_Log("SCREEN: MOUSE POS: X, %f, POS: Y, %f", point.x, point.y);
					//SDL_Log("SCREEN FLOORED: MOUSE POS: X, %f, POS: Y, %f", floorf(point.x), floor(point.y));
					
					
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
