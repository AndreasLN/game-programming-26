#define STB_IMAGE_IMPLEMENTATION
#define ITU_UNITY_BUILD

#define ENABLE_DIAGNOSTICS

#include <SDL3/SDL.h>
#include <stb_image.h>

#include <itu_common.hpp>
#include <itu_lib_render_screen.hpp>
#include <itu_lib_overlaps.hpp>

// frame rate
const SDL_Time TARGET_FRAMERATE = SECONDS(1) / 60;
// window size
const int WINDOW_W = 1200;
const int WINDOW_H = 1000;

// amount of objects
const int ENTITY_COUNT   = 4096;
const int MAX_COLLISIONS = 1024; // num max collisions per frame

bool DEBUG_separate_collisions   = true;
bool DEBUG_render_colliders      = true;
bool DEBUG_render_texture_border = false;

struct E02_Entity;
struct E02_EntityCollisionInfo;

struct E02_SDLContext
{
	SDL_Renderer* renderer;
	float zoom;     // render zoom
	float window_w;	// current window width after render zoom has been applied
	float window_h;	// current window width after render zoom has been applied

	float delta;    // in seconds
	float uptime;   // in seconds

	bool btn_isdown_up;
	bool btn_isdown_down;
	bool btn_isdown_left;
	bool btn_isdown_right;
	bool btn_isdown_space;
};

struct E02_GameState
{
	E02_Entity* player;

	// game-allocated memory
	E02_Entity* entities;
	int entities_alive_count;

	E02_EntityCollisionInfo* frame_collisions;
	int frame_collisions_count;

	// SDL-allocated structures
	SDL_Texture* atlas;
};

static SDL_Texture* texture_create(E02_SDLContext* context, const char* path)
{
	int w=0, h=0, n=0;
	unsigned char* pixels = stbi_load(path, &w, &h, &n, 0);
	SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ABGR8888, pixels, w * n);

	SDL_Texture* ret = SDL_CreateTextureFromSurface(context->renderer, surface);

	SDL_DestroySurface(surface);
	stbi_image_free(pixels);

	return ret;
}

// =====================================================================================================================
// sprite
// =====================================================================================================================

struct E02_Sprite
{
	SDL_Texture* texture;
	SDL_FRect    rect;
	color        tint;
	vec2f        pivot;
};

// quick sprite rendering function that takes care of most of the functionalities
// NOTE: this function is still temporary since ATM we can't really deal with game worlds bigger than the rendering window
//       we will address it in lecture 03, and then we will just create a final sprite system and be done with it
static void sprite_render(E02_SDLContext* context, vec2f position, vec2f size, E02_Sprite* sprite)
{
	SDL_FRect dst_rect;
	// define size of sprite
	dst_rect.w = size.x;
	dst_rect.h = size.y;
	// Set position of rect based on start position. 
	// dst_rect.w * sprite->pivot.x is center if pivot.x is 0.5
	dst_rect.x = position.x - dst_rect.w * sprite->pivot.x;
	dst_rect.y = position.y - dst_rect.h * sprite->pivot.y;
		

	SDL_SetTextureColorModFloat(sprite->texture, sprite->tint.r, sprite->tint.g, sprite->tint.b);
	SDL_SetTextureAlphaModFloat(sprite->texture, sprite->tint.a);
	SDL_RenderTexture(context->renderer, sprite->texture, &sprite->rect, &dst_rect);

	if(DEBUG_render_texture_border)
	{
		SDL_SetRenderDrawColorFloat(context->renderer, 1, 1, 1, 1);
		SDL_RenderRect(context->renderer, &dst_rect);
	}
}

// =====================================================================================================================
// entity
// =====================================================================================================================

struct E02_Entity
{
	vec2f position;
	vec2f size;


	E02_Sprite sprite;
	bool is_static;
	// collider info
	float collider_radius;
	vec2f collider_offset;
};

static E02_Entity* entity_create(E02_GameState* state)
{
	if(!(state->entities_alive_count < ENTITY_COUNT))
		return NULL;

	// // concise version
	//return &state->entities[state->entities_alive_count++];

	E02_Entity* ret = &state->entities[state->entities_alive_count];
	++state->entities_alive_count;

	return ret;
}

// NOTE: this only works if nobody holds references to other entities!
static void entity_destroy(E02_GameState* state, E02_Entity* entity)
{
	// NOTE: here we want to fail hard, nobody should pass us a pointer not gotten from `entity_create()`
	SDL_assert(entity < state->entities || entity > state->entities + ENTITY_COUNT);

	--state->entities_alive_count;
	*entity = state->entities[state->entities_alive_count];
}
// =====================================================================================================================
// collisions
// =====================================================================================================================

struct E02_EntityCollisionInfo
{
	E02_Entity* e1;
	E02_Entity* e2;

	vec2f normal;
	float separation;
};

static void collision_check(E02_GameState* state)
{
	state->frame_collisions_count = 0;
	for(int i = 0; i < state->entities_alive_count - 1; ++i)
	{
		
		E02_Entity* e1 = &state->entities[i];
		if (e1->is_static){
			continue;
		}
		
		for(int j = i + 1; j < state->entities_alive_count; ++j)
		{
			E02_Entity* e2 = &state->entities[j];

			if(itu_lib_overlaps_circle_circle(
				e1->position + e1->collider_offset, e1->collider_radius,
				e2->position + e2->collider_offset, e2->collider_radius
			))
			{
				e1->sprite.tint = COLOR_RED;
				e2->sprite.tint = COLOR_RED;

				if(state->frame_collisions_count >= MAX_COLLISIONS)
				{
					SDL_Log("[WARNING] too many collisions!");
					return;
				}

				// NOTE: here we are redoing a bunch of work that we already done in the overlap test. An easy optimization is do to have the test return the collision info
				vec2f v = (e2->position + e2->collider_offset) - (e1->position + e1->collider_offset);
				float l = length(v);
				float separation_vector = e1->collider_radius + e2->collider_radius - l;
				int new_collision_idx = state->frame_collisions_count++;

				state->frame_collisions[new_collision_idx].e1 = e1;
				state->frame_collisions[new_collision_idx].e2 = e2;
				state->frame_collisions[new_collision_idx].normal = v / l; // normalize vector (we already need the length, so we don't need to call normalize which would do that anyway)
				state->frame_collisions[new_collision_idx].separation = separation_vector;
			}
		}
	}
}

static void collision_separate(E02_GameState* state)
{
	vec2f sep;
	for(int i = 0; i < state->frame_collisions_count; ++i)
	{
		E02_EntityCollisionInfo entity_collision_info = state->frame_collisions[i];
		if(entity_collision_info.e1->is_static || entity_collision_info.e2->is_static){
			sep = entity_collision_info.normal * entity_collision_info.separation;
		}
		else{
			sep = entity_collision_info.normal * entity_collision_info.separation / 2;
		}
		//vec2f sep = entity_collision_info.normal * entity_collision_info.separation / 2;

		if(!entity_collision_info.e1->is_static){
			entity_collision_info.e1->position -= sep;
		}
		if(!entity_collision_info.e2->is_static){
			entity_collision_info.e2->position += sep;
		}

	}
}

// =====================================================================================================================
// game
// =====================================================================================================================

static void game_init(E02_SDLContext* context, E02_GameState* state)
{
	// contiguous memory
	{
		state->entities = (E02_Entity*)SDL_malloc(ENTITY_COUNT * sizeof(E02_Entity));
		SDL_assert(state->entities);

		state->frame_collisions = (E02_EntityCollisionInfo*)SDL_malloc(MAX_COLLISIONS * sizeof(E02_EntityCollisionInfo));
		SDL_assert(state->frame_collisions);
	}

	// texture atlasesw
	state->atlas = texture_create(context, "data/kenney/simpleSpace_tilesheet_2.png");

}

static void game_reset(E02_SDLContext* context, E02_GameState* state)
{
	SDL_memset(state->entities, 0, ENTITY_COUNT * sizeof(E02_Entity));
	state->entities_alive_count = 0;

	// entities
	E02_Entity* player = entity_create(state);
	// we always have a player. This should also always be the first entity created, so it should never fail
	SDL_assert(player);
	player->position.x = (float)context->window_w / 2;
	player->position.y = (float)context->window_h / 2;
	player->size = vec2f{ 64, 64 };
	player->sprite.texture = state->atlas;
	player->sprite.rect = SDL_FRect{ 0, 0, 128, 128 };
	player->sprite.tint = COLOR_WHITE;
	player->sprite.pivot = vec2f{ 0.5f, 0.5f };
	player->collider_radius = 16;
	player->is_static = false;
	state->player = player;

	// grid pattern
	for(int i = 0; i < ENTITY_COUNT - 10; ++i)
	{
		E02_Entity* entity = entity_create(state);
		if(!entity)
		{
			SDL_Log("[WARNING] too many entity spawned!");
			break;
		}
		
		vec2f coords = vec2f{ 1.5f + i % 3, 1.5f + i / 3};
		entity->size = vec2f{ 64, 64 };
		entity->position = mul_element_wise(entity->size,  coords);
		entity->sprite.texture = state->atlas;
		entity->sprite.rect = SDL_FRect{ 0, 4*128, 128, 128 };
		entity->sprite.tint = COLOR_WHITE,
		entity->sprite.pivot = vec2f{ 0.5f, 0.5f };
		entity->is_static = true;
		entity->collider_radius = 32;
	}
}

static void game_update(E02_SDLContext* context, E02_GameState* state)
{
	vec2f mov = { 0 };
	if(context->btn_isdown_up)
		mov.y -= 1;
	if(context->btn_isdown_down)
		mov.y += 1;
	if(context->btn_isdown_left)
		mov.x -= 1;
	if(context->btn_isdown_right)
		mov.x += 1;

	vec2f velocity = normalize(mov) * (128 * context->delta);
	state->player->position = state->player->position + velocity;

	// reset tint
	for(int i = 0; i < state->entities_alive_count; ++i)
	{
		E02_Entity* entity = &state->entities[i];
		entity->sprite.tint = COLOR_WHITE;
	}

	collision_check(state);
	if(DEBUG_separate_collisions)
		collision_separate(state);
}

static void game_render(E02_SDLContext* context, E02_GameState* state)
{
	// render
	for(int i = 0; i < state->entities_alive_count; ++i)
	{
		E02_Entity* entity = &state->entities[i];
		sprite_render(context, entity->position, entity->size, &entity->sprite);

		if(DEBUG_render_colliders)
		{
			itu_lib_render_screen_point(context->renderer, entity->position + entity->collider_offset, 5, COLOR_GREEN);
			itu_lib_render_screen_circle(
				context->renderer,
				entity->position + entity->collider_offset,
				entity->collider_radius,
				16, COLOR_GREEN
			);
		}
	}
		
	// debug window
	SDL_SetRenderDrawColor(context->renderer, 0xFF, 0x00, 0xFF, 0xff);
	SDL_RenderRect(context->renderer, NULL);
}

int main(void)
{
	int a = sizeof(int*);
	bool quit = false;
	SDL_Window* window;
	E02_SDLContext context = { 0 };
	E02_GameState  state   = { 0 };

	context.window_w = WINDOW_W;
	context.window_h = WINDOW_H;

	SDL_CreateWindowAndRenderer("E02 - Collisions", context.window_w, context.window_h, 0, &window, &context.renderer);

	SDL_SetRenderDrawBlendMode(context.renderer, SDL_BLENDMODE_BLEND);
	
	// increase the zoom to make debug text more legible
	// (ie, on the class projector, we will usually use 2)
	{
		context.zoom = 1;
		context.window_w /= context.zoom;
		context.window_h /= context.zoom;
		SDL_SetRenderScale(context.renderer, context.zoom, context.zoom);
	}

	game_init(&context, &state);
	game_reset(&context, &state);

	SDL_Time walltime_frame_beg = 0;
	SDL_Time walltime_frame_end = 0;
	SDL_Time walltime_work_end  = 0;
	SDL_Time time_elapsed_work       = 0;
	SDL_Time time_elapsed_frame      = 0;

	SDL_GetCurrentTime(&walltime_frame_beg);
	walltime_frame_end = walltime_frame_beg;

	while(!quit)
	{
		// input
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_EVENT_QUIT:
					quit = true;
					break;
					
				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP:
					switch(event.key.key)
					{
						case SDLK_W: context.btn_isdown_up    = event.key.down; break;
						case SDLK_A: context.btn_isdown_left  = event.key.down; break;
						case SDLK_S: context.btn_isdown_down  = event.key.down; break;
						case SDLK_D: context.btn_isdown_right = event.key.down; break;
						case SDLK_SPACE: context.btn_isdown_space = event.key.down; break;
					}

					// debug keys
					if(event.key.down && !event.key.repeat)
					{
						switch(event.key.key)
						{
							case SDLK_TAB: game_reset(&context, &state); break;
							case SDLK_F1: DEBUG_separate_collisions   = !DEBUG_separate_collisions;   break;
							case SDLK_F2: DEBUG_render_colliders      = !DEBUG_render_colliders;      break;
							case SDLK_F3: DEBUG_render_texture_border = !DEBUG_render_texture_border; break;
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
			SDL_FRect rect = SDL_FRect{ 5, 5, 225, 65 };
			SDL_RenderFillRect(context.renderer, &rect);
			SDL_SetRenderDrawColor(context.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDebugTextFormat(context.renderer, 10, 10, "work: %9.6f ms/f", (float)time_elapsed_work  / (float)MILLIS(1));
			SDL_RenderDebugTextFormat(context.renderer, 10, 20, "tot : %9.6f ms/f", (float)time_elapsed_frame / (float)MILLIS(1));
			SDL_RenderDebugTextFormat(context.renderer, 10, 30, "[TAB] reset ");
			SDL_RenderDebugTextFormat(context.renderer, 10, 40, "[F1]  collisions        %s", DEBUG_separate_collisions   ? " ON" : "OFF");
			SDL_RenderDebugTextFormat(context.renderer, 10, 50, "[F2]  render colliders  %s", DEBUG_render_colliders      ? " ON" : "OFF");
			SDL_RenderDebugTextFormat(context.renderer, 10, 60, "[F3]  render tex border %s", DEBUG_render_texture_border ? " ON" : "OFF");
		}
#endif

		// render
		SDL_RenderPresent(context.renderer);

		SDL_GetCurrentTime(&walltime_work_end);
		time_elapsed_work = walltime_work_end - walltime_frame_beg;

		if(time_elapsed_work < TARGET_FRAMERATE)
			SDL_DelayPrecise(TARGET_FRAMERATE - time_elapsed_work);
		SDL_GetCurrentTime(&walltime_frame_end);
		time_elapsed_frame = walltime_frame_end - walltime_frame_beg;

		context.delta = (float)time_elapsed_frame / (float)SECONDS(1);
		context.uptime += context.delta;
		walltime_frame_beg = walltime_frame_end;
	}
}
