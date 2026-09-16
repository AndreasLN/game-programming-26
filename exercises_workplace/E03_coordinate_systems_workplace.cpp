#define ENABLE_DIAGNOSTICS

#include <itu_engine.hpp>

bool DEBUG_render_textures = true;
bool DEBUG_render_outlines = true;
const int ENTITY_COUNT = 4096;

struct E03_Entity
{
	Sprite sprite;
	Transform2D transform;
};

struct E03_GameState
{
	// shortcut references
	E03_Entity* player;

	// game-allocated memory
	E03_Entity* entities;
	int entities_alive_count;

	// SDL-allocated structures
	SDL_Texture* atlas;
	SDL_Texture* bg;
};

static E03_Entity* entity_create(E03_GameState* state)
{
	if(!(state->entities_alive_count < ENTITY_COUNT))
		// NOTE: this might as well be an assert, if we don't have a way to recover/handle it
		return NULL;

	// // concise version
	//return &state->entities[state->entities_alive_count++];

	E03_Entity* ret = &state->entities[state->entities_alive_count];
	++state->entities_alive_count;
	return ret;
}

// NOTE: this only works if nobody holds references to other entities!
//       if that were the case, we couldn't swap them around.
//       We will see in later lectures how to handle this kind of problems
static void entity_destroy(E03_GameState* state, E03_Entity* entity)
{
	// NOTE: here we want to fail hard, nobody should pass us a pointer not gotten from `entity_create()`
	SDL_assert(entity < state->entities ||entity > state->entities + ENTITY_COUNT);

	--state->entities_alive_count;
	*entity = state->entities[state->entities_alive_count];
}

static void game_init(EngineContext* context, E03_GameState* state)
{
	// allocate memory
	state->entities = (E03_Entity*)SDL_calloc(ENTITY_COUNT, sizeof(E03_Entity));
	SDL_assert(state->entities);

	// TODO allocate space for tile info (when we'll load those from file)
	// texture atlases
	state->atlas = itu_resources_texture_create(context, "data/kenney/tiny_dungeon_packed.png", SDL_SCALEMODE_NEAREST);
	state->bg    = itu_resources_texture_create(context, "data/kenney/prototype_texture_dark/texture_13.png", SDL_SCALEMODE_LINEAR);
}

static void game_reset(EngineContext* context, E03_GameState* state)
{
	state->entities_alive_count = 0;
	// entities
	{
		E03_Entity* bg = entity_create(state);
		SDL_FRect sprite_rect = SDL_FRect{ 0, 0, 1024, 1024};
		itu_lib_sprite_init(
			&bg->sprite,
			state->bg,
			itu_lib_sprite_get_source_rect(0, 0, 1024, 1024)
		);
		bg->transform.scale = VEC2F_ONE;
	}

	{
		state->player = entity_create(state);
		state->player->transform.position = VEC2F_ZERO;
		state->player->transform.scale = VEC2F_ONE;
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
		const float player_speed = 1;

		E03_Entity* entity = state->player;
		vec2f mov = { 0 };
		if(context->btn_isdown_up)
			mov.y += 1;
		if(context->btn_isdown_down)
			mov.y -= 1;
		if(context->btn_isdown_left)
			mov.x -= 1;
		if(context->btn_isdown_right)
			mov.x += 1;
	
		//SDL_Log("MOUSE POS: X: %f, Y: %f", context->mouse_pos.x, context->mouse_pos.y);
		entity->transform.position = entity->transform.position + mov * (player_speed * context->delta);

		// camera follows player
		context->camera_active->world_position = entity->transform.position;
	}
}

static void game_render(EngineContext* context, E03_GameState* state)
{
	for(int i = 0; i < state->entities_alive_count; ++i)
	{
		E03_Entity* entity = &state->entities[i];
		// render texture
		SDL_FRect rect_src = entity->sprite.rect;
		SDL_FRect rect_dst;

		if(DEBUG_render_textures)
			itu_lib_sprite_render(context, &entity->sprite, &entity->transform);

		if(DEBUG_render_outlines)
			itu_lib_sprite_render_debug(context, &entity->sprite, &entity->transform);
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
	config.camera_pixel_per_unit = 16;
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
					SDL_Log("WORLD: MOUSE POS: X, %f, POS: Y, %f", world_point.x, world_point.y);
					SDL_Log("CAMERA: MOUSE POS: X, %f, POS: Y, %f", context.mouse_pos.x, context.mouse_pos.y);
					context.mouse_pos.x = event.motion.x;
					context.mouse_pos.y = event.motion.y;
					vec2f point = itu_lib_context_point_screen_to_global(&context, context.mouse_pos);
					SDL_Log("SCREEN: MOUSE POS: X, %f, POS: Y, %f", point.x, point.y);
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
