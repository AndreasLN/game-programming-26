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
const int WINDOW_H = 800;

// amount of objects
const int ENTITY_COUNT   = 4096;
const int CELLS			 = 4;
const int split			 = sqrt(CELLS);
const int split_w		 = WINDOW_W / split;
const int split_h		 = WINDOW_H / split;


// better grammer for getting stuff from 2d array
#define idx(i,j, x) (j + i * x)


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

struct E02_entitygrid
{
	E02_Entity* entities;
	int entities_alive_count;
	int x_start;
	int x_end;
	int y_start;
	int y_end;

};

struct E02_GameState
{
	E02_Entity* player;

	// game-allocated memory
	E02_entitygrid* entity_grids;
	E02_Entity* entities;
	//int entities_alive_count;

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

bool inRange(unsigned x_low, unsigned x_high, unsigned x, unsigned y_low, unsigned y_high, unsigned y)
{
	//SDL_Log("%i < %i < %i. %i < %i < %i", x_low, x, x_high, y_low, y, y_high);
    return  ((x-x_low) < (x_high-x_low) && (y-y_low) < (y_high-y_low));
}

static E02_Entity* entity_create(E02_GameState* state, vec2f start_pos, vec2f size)
{
	E02_Entity* ret = NULL;
	E02_entitygrid * cur_grid;
	int cur_alive;
	for (int i = 0; i < CELLS; i++)
	{
		cur_grid = &state->entity_grids[i];
		if(inRange(cur_grid->x_start, cur_grid->x_end, start_pos.x, cur_grid->y_start, cur_grid->y_end, start_pos.y))
		{
			//SDL_Log("!!!!!!%i < %f < %i. %i < %f < %i", cur_grid->x_start, start_pos.x, cur_grid->x_end, cur_grid->y_start, start_pos.y, cur_grid->y_end);
			cur_alive = state->entity_grids[i].entities_alive_count;

			if(!(cur_alive < ENTITY_COUNT))
					return NULL;
			
			ret = &state->entity_grids[i].entities[cur_alive];
			++state->entity_grids[i].entities_alive_count;
		}
	}

	return ret;
}


// NOTE: this only works if nobody holds references to other entities!
static void entity_destroy(E02_GameState* state, E02_Entity* entity, vec2f pos, vec2f size)
{
	E02_entitygrid * cur_grid;
	int cur_alive;
	for (int i = 0; i < CELLS; i++)
	{
		cur_grid = &state->entity_grids[i];
		if(inRange(cur_grid->x_start, cur_grid->x_end, pos.x, cur_grid->y_start, cur_grid->y_end, pos.y))
		{
			SDL_Log("%i, DESTROY", cur_alive);
			SDL_Log("!!!!!!%i < %f < %i. %i < %f < %i", cur_grid->x_start, pos.x, cur_grid->x_end, cur_grid->y_start, pos.y, cur_grid->y_end);
			cur_alive = state->entity_grids[i].entities_alive_count;
			
			--state->entity_grids[i].entities_alive_count;
			*entity = state->entity_grids[i].entities[state->entity_grids[i].entities_alive_count];
		}
	}
	// NOTE: here we want to fail hard, nobody should pass us a pointer not gotten from `entity_create()`
	//SDL_assert(entity < state->entities || entity > state->entities + ENTITY_COUNT);

}
static void entity_destroy(E02_GameState* state, E02_Entity* entity, E02_entitygrid* cell)
{
	//SDL_assert(entity < state->entities || entity > state->entities + ENTITY_COUNT);
	--cell->entities_alive_count;
	*entity = cell->entities[cell->entities_alive_count];	
}


static void entity_move(E02_GameState* state, E02_Entity * entity, vec2f new_pos)
{
	E02_Entity new_entity = { 0 };
	E02_Entity * new_entity_ptr = &new_entity;
	E02_entitygrid * cur_grid;
	int cur_alive;
	for (int i = 0; i < CELLS; i++)
	{
		cur_grid = &state->entity_grids[i];
		if(inRange(cur_grid->x_start, cur_grid->x_end, new_pos.x, cur_grid->y_start, cur_grid->y_end, new_pos.y))
		{
			SDL_Log("!!!!!!%i < %f < %i. %i < %f < %i", cur_grid->x_start, new_pos.x, cur_grid->x_end, cur_grid->y_start, new_pos.y, cur_grid->y_end);
			cur_alive = state->entity_grids[i].entities_alive_count;
			//SDL_Log("AAAAHHHHHHH");

			if(!(cur_alive < ENTITY_COUNT))
					return;

			memcpy(&state->entity_grids[i].entities[cur_alive], entity, sizeof(E02_Entity));
			//state->entity_grids[i].entities[cur_alive] = *new_entity_ptr;
			if (entity == state->player){
				state->player = &state->entity_grids[i].entities[cur_alive];
				entity_destroy(state, entity, entity->position, entity->size);
			}
			++state->entity_grids[i].entities_alive_count;
		}
	}
	// // concise version
	//return &state->entities[state->entities_alive_count++];

	//E02_Entity* ret = &state->entities[idx(0, state->entities_alive_count)];
	//++state->entities_alive_count;

	return;
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
	E02_entitygrid * cur_grid;
	state->frame_collisions_count = 0;
	for(int grid_count = 0; grid_count < CELLS; ++grid_count){
		cur_grid = &state->entity_grids[grid_count];

		for(int i = 0; i < cur_grid->entities_alive_count - 1; ++i)
		{
		
			E02_Entity* e1 = &cur_grid->entities[i];
			if (e1->is_static){
				continue;
			}
			
			for(int j = i + 1; j < cur_grid->entities_alive_count; ++j)
			{
				E02_Entity* e2 = &cur_grid->entities[j];

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

static void clamp(E02_GameState* state){
	E02_entitygrid* cur_grid;
	for (int grid_count = 0; grid_count < CELLS; ++grid_count){
		cur_grid = &state->entity_grids[grid_count];

		for (int i = 0; i < cur_grid->entities_alive_count; ++i){
				E02_Entity * entity = &cur_grid->entities[i];

				if(entity->position.x + entity->collider_offset.x + entity->collider_radius > WINDOW_W){
					entity->position.x = WINDOW_W - entity->collider_offset.x - entity->collider_radius;
				}
				if(entity->position.x + entity->collider_offset.x - entity->collider_radius < 0){
					entity->position.x = entity->collider_radius;
				}
				if(entity->position.y + entity->collider_offset.y - entity->collider_radius < 0){
					entity->position.y = entity->collider_radius;
				}
				if(entity->position.y + entity->collider_offset.y + entity->collider_radius > WINDOW_H){
					entity->position.y = WINDOW_H - entity->collider_offset.y - entity->collider_radius;
				}
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
		//state->entities = (E02_Entity*)SDL_malloc(CELLS * sizeof(E02_Entity));
		
		state->entity_grids = (E02_entitygrid*)SDL_malloc(CELLS * sizeof(E02_entitygrid));
		SDL_assert(state->entity_grids);

		// setup grids
		int split = sqrt(CELLS);
		for(int i = 0; i < split; ++i)
		{
			for(int j = 0; j < split; ++j){

				E02_entitygrid init_grid = { 0 };
				E02_entitygrid * grid = &state->entity_grids[idx(i,j, split)];
				grid->entities = (E02_Entity*)SDL_malloc(ENTITY_COUNT * sizeof(E02_Entity));
				SDL_assert(grid->entities);
				grid->entities_alive_count = 0;
				
				grid->x_start = i * (WINDOW_W / split);
				grid->x_end = grid->x_start + WINDOW_W / split;

				grid->y_start = j * (WINDOW_H / split);
				grid->y_end = grid->y_start + WINDOW_H / split;

				//SDL_Log("x_start: %i, y_start: %i", state->entity_grids[idx(i,j,split)].x_start,state->entity_grids[idx(i,j,split)].y_start);
				//SDL_Log("x_end: %i, y_end: %i", state->entity_grids[idx(i,j,split)].x_end,state->entity_grids[idx(i,j,split)].y_end);
			}
		}		

		//SDL_Log("!!!!!x_start: %i, y_start: %i", state->entity_grids[0].x_start,state->entity_grids[0].y_start);
		//SDL_Log("!!!!!x_end: %i, y_end: %i", state->entity_grids[0].x_end,state->entity_grids[0].y_end);

		state->frame_collisions = (E02_EntityCollisionInfo*)SDL_malloc(MAX_COLLISIONS * sizeof(E02_EntityCollisionInfo));
		SDL_assert(state->frame_collisions);
	}

	// texture atlasesw
	state->atlas = texture_create(context, "data/kenney/simpleSpace_tilesheet_2.png");

}

static void game_reset(E02_SDLContext* context, E02_GameState* state)
{
	for (int i = 0; i < CELLS; ++i)
	{
		SDL_memset(state->entity_grids[i].entities, 0, ENTITY_COUNT * sizeof(E02_Entity));
		state->entity_grids->entities_alive_count = 0;

	}

	//SDL_memset(state->entity_grids, 0, CELLS * sizeof(E02_entitygrid));
	
	vec2f position = {(float)context->window_w - 20, (float)context->window_h / 2};
	vec2f size = vec2f{ 64, 64 };
	// entities
	E02_Entity* player = entity_create(state, position, size);
	// we always have a player. This should also always be the first entity created, so it should never fail
	SDL_assert(player);
	player->position = position;
	player->size = size;
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
		vec2f size = vec2f{ 12, 12 };
		vec2f coords = vec2f{ 1.5f + i % 64, 1.5f + i / 64};
		vec2f pos = mul_element_wise(size,  coords);
		E02_Entity* entity = entity_create(state, pos, size);
		if(!entity)
		{
			SDL_Log("[WARNING] too many entity spawned!");
			break;
		}
		
		entity->size = size;
		entity->position = pos;
		entity->sprite.texture = state->atlas;
		entity->sprite.rect = SDL_FRect{ 0, 4*128, 128, 128 };
		entity->sprite.tint = COLOR_WHITE,
		entity->sprite.pivot = vec2f{ 0.5f, 0.5f };
		entity->is_static = false;
		entity->collider_radius = 6;
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
	
	//SDL_Log("%f,   %f",  floor(state->player->position.x / split_w),floor((state->player->position.x + velocity.x) / split_w) );
	if(floor(state->player->position.x / split_w) != floor((state->player->position.x + velocity.x) / split_w) ||
		floor(state->player->position.y / split_h) != floor((state->player->position.y + velocity.y) / split_h))
		{
			E02_Entity * old_player = state->player;
			entity_move(state, state->player, state->player->position + velocity);
			//entity_destroy(state, old_player, state->player->position, state->player->size);
		}	
	
	state->player->position = state->player->position + velocity;
	

	E02_entitygrid * cur_grid;
	// reset tint
	for (int grid_count = 0; grid_count < CELLS; ++grid_count)
	{
		cur_grid = &state->entity_grids[grid_count];
		for(int i = 0; i < cur_grid->entities_alive_count; ++i)
			{
				E02_Entity* entity = &cur_grid->entities[i];
				entity->sprite.tint = COLOR_WHITE;
			}

		/* code */
	}
	
	

	collision_check(state);
	if(DEBUG_separate_collisions)
		collision_separate(state);
	

	clamp(state);
}

static void game_render(E02_SDLContext* context, E02_GameState* state)
{
	E02_entitygrid * cur_grid;
	for(int grid_count = 0; grid_count < CELLS; ++grid_count){
		cur_grid = &state->entity_grids[grid_count];
		
		// render
		for(int i = 0; i < cur_grid->entities_alive_count; ++i)
		{
			E02_Entity* entity = &cur_grid->entities[i];
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
	
	SDL_Log("INIT");
	game_init(&context, &state);	
	SDL_Log("RESET");

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
