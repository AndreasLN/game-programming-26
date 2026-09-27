#define ENABLE_DIAGNOSTICS

#include <itu_engine.hpp>

using namespace std;
#include <list>

using namespace std;
#include <string>

using namespace std;
#include <map>


const int ENTITY_COUNT = 1024;

const int COLLISION_FILTER_PLAYER         = 0b00001;
const int COLLISION_FILTER_GROUND         = 0b00010;
const int COLLISION_FILTER_CLUTTER        = 0b00100;
const int COLLISION_FILTER_CLUTTER_SENSOR = 0b01000;
const float GRAVITY      = -0.0f;
bool DEBUG_render_textures = true;
bool DEBUG_render_outlines = false;
bool DEBUG_physics = true;
int DEBUG_simulation_type_current = 0;

const char* const DEBUG_simulation_types[] =
{
	"Dynamic", "\"Kinematic\""
};

b2DebugDraw debug_draw;

enum SimulationType
{
	SIMULATION_TYPE_DYNAMIC,
	SIMULATION_TYPE_KINEMATIC,
};

struct E04_Entity
{
	Sprite      sprite;
	Transform2D transform;
	b2BodyId    body_id;
	vec2f       velocity;
};

struct E04_PlayerData
{
	// definitions
	float h;   // jump height
	float x_h; // jump horizontal distance

	// runtime (jupm)
	float g;   // gravity (for current jump)
	vec2f p_0; // initial position (for current jump)

	// DETERMINES WHERE BILLARD POLE HITS
	b2Vec2 startClick;
	b2Vec2 endClick;

	bool started; // has player clicked once?

	float v_0; // initial VERTICAL velocity (for current jump)
	float v_x; // initial foot speed (for current jump)
	float t_h; // jump duration (for current jump)
};

static float player_dynamic_gravity = 0.0f;
static float player_dynamic_jump_impulse = 3;
static float player_dynamic_mov_force = 10;

struct E04_GameState
{
	// shortcut references
	E04_Entity* player;

	// game-allocated memory
	list<E04_Entity*> entities;
	list<b2ShapeId> ball_shape_ids;
	int entities_alive_count;
	E04_PlayerData player_data;

	// SDL-allocated structures
	SDL_Texture* atlas;

	// box2d
	b2WorldId world_id;
};

static void entity_create(E04_GameState* state, E04_Entity * entity)
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
static void entity_destroy(E04_GameState* state, E04_Entity* entity)
{
	// NOTE: here we want to fail hard, nobody should pass us a pointer not gotten from `entity_create()`

	state->entities.remove(entity);
	--state->entities_alive_count;
}

void compute_jump_parameters(E04_PlayerData* data)
{
	data->v_0 = (2*data->h*data->v_x) / (data->x_h);
	data->g   = (-2*data->h * data->v_x * data->v_x) / (data->x_h * data->x_h);
}

void clutter_apply_impulse_random(b2ShapeId clutte_entity, b2Vec2 direction, float amount, float spread)
{

	float angle = (SDL_randf() - 0.5f) * spread;

	b2Vec2 point   = b2Vec2_zero;
	b2Vec2 impulse = b2RotateVector(b2MakeRot(angle), direction);
	impulse = b2MulSV(amount, impulse);

	b2BodyId body_id = b2Shape_GetBody(clutte_entity);
	b2Body_ApplyLinearImpulse(body_id, impulse, point, true);
}

static void game_init(EngineContext* context, E04_GameState* state)
{
	itu_lib_input_set_mapping_keyboard(context, SDLK_W, BTN_TYPE_UP);
	itu_lib_input_set_mapping_keyboard(context, SDLK_A, BTN_TYPE_LEFT);
	itu_lib_input_set_mapping_keyboard(context, SDLK_S, BTN_TYPE_DOWN);
	itu_lib_input_set_mapping_keyboard(context, SDLK_D, BTN_TYPE_RIGHT);
	itu_lib_input_set_mapping_keyboard(context, SDLK_Q, BTN_TYPE_ACTION_0);
	itu_lib_input_set_mapping_keyboard(context, SDLK_E, BTN_TYPE_ACTION_1);
	itu_lib_input_set_mapping_keyboard(context, SDLK_SPACE, BTN_TYPE_SPACE);

	itu_lib_input_set_mapping_mouse(context, 1, BTN_TYPE_ACTION_0); //leftclick
	itu_lib_input_set_mapping_mouse(context, 3, BTN_TYPE_ACTION_1); //rightclick


	itu_lib_input_set_mapping_keyboard(context, SDLK_F1, BTN_TYPE_DEBUG_F1);
	itu_lib_input_set_mapping_keyboard(context, SDLK_TAB, BTN_TYPE_DEBUG_RESET);

	// allocate memory
	state->entities = {};

	state->world_id = { 0 };

	state->player_data = { 0 };
	state->player_data.g = -66.67f;
	state->player_data.h   = 3.0f;
	state->player_data.x_h = 1.5f;
	state->player_data.v_x = 5.0f;

	// texture atlases
	state->atlas = itu_resources_texture_create(context, "data/kenney/tiny_dungeon_packed.png", SDL_SCALEMODE_NEAREST);
}

static void game_reset(EngineContext* context, E04_GameState* state)
{
	// TMP reset uptime (should probably be two different variables
	context->uptime = 0;

	if(b2World_IsValid(state->world_id))
		b2DestroyWorld(state->world_id);
	b2WorldDef def_world = b2DefaultWorldDef();
	def_world.gravity.y = DEBUG_simulation_type_current == SIMULATION_TYPE_KINEMATIC
		? GRAVITY
		: player_dynamic_gravity;
	state->world_id = b2CreateWorld(&def_world);
	b2World_SetHitEventThreshold(state->world_id, 0);

	state->entities_alive_count = 0;
	state->ball_shape_ids = {};

	// player
	{
		E04_Entity* entity = new E04_Entity();
		entity_create(state, entity);
		state->player = entity;
		entity->transform.position = VEC2F_ZERO;
		entity->transform.scale = VEC2F_ONE;
		itu_lib_sprite_init(
			&entity->sprite,
			state->atlas,
			itu_lib_sprite_get_source_rect(0, 9, 16, 16)
		);
		entity->sprite.pivot.y = 0;

		// box2d body, shape and polygon
		{
			vec2f size = itu_lib_sprite_get_world_size(context, &entity->sprite, &entity->transform);
			vec2f offset = -mul_element_wise(size, entity->sprite.pivot - vec2f{ 0.5f, 0.5f });

			b2BodyDef body_def = b2DefaultBodyDef();
			body_def.type = b2_dynamicBody;
			body_def.fixedRotation = true;
			body_def.position = b2Vec2{ 0, 0 };

			b2ShapeDef shape_def = b2DefaultShapeDef();
			shape_def.density = 1; // NOTE: default density of 0 will mess with collisions and gravity!
			shape_def.enableSensorEvents  = true;
			shape_def.enableContactEvents = true;
			shape_def.enableHitEvents     = true;
			shape_def.filter.categoryBits = COLLISION_FILTER_PLAYER;
			shape_def.filter.maskBits = COLLISION_FILTER_GROUND | COLLISION_FILTER_CLUTTER_SENSOR;
			b2Polygon polygon = b2MakeOffsetBox(size.x / 2, size.y / 2, value_cast(b2Vec2, offset), b2MakeRot(entity->transform.rotation));
			b2Circle circle;
			circle.radius = 0.5f;
			circle.center = value_cast(b2Vec2, offset);
			entity->body_id = b2CreateBody(state->world_id, &body_def);
		 	b2CreateCircleShape(entity->body_id, &shape_def, &circle);
		}
	}


	// Holes
	{



	}

	//Balls
	{
		b2BodyDef balls_body_def = b2DefaultBodyDef();
		balls_body_def.type = b2_dynamicBody;
		balls_body_def.fixedRotation = false;
		balls_body_def.linearDamping = 0.1;
		balls_body_def.angularDamping = 0.1;

		// collider shape (to enable collisions with the ground)
		b2ShapeDef balls_shape_def = b2DefaultShapeDef();
		balls_shape_def.density = 1;
		balls_shape_def.enableHitEvents = true;
		balls_shape_def.enableContactEvents = true;
		balls_shape_def.filter.categoryBits = COLLISION_FILTER_CLUTTER;
		balls_shape_def.filter.maskBits     = COLLISION_FILTER_GROUND | COLLISION_FILTER_CLUTTER;


		// sensor shape (to enable interaction with the player)
		b2ShapeDef balls_shape_def_clutter = b2DefaultShapeDef();
		balls_shape_def_clutter.density = 0;
		balls_shape_def_clutter.isSensor = true;
		balls_shape_def_clutter.enableSensorEvents = true;
		balls_shape_def_clutter.enableHitEvents = true;
		balls_shape_def_clutter.filter.categoryBits = COLLISION_FILTER_CLUTTER_SENSOR;
		balls_shape_def_clutter.filter.maskBits     = COLLISION_FILTER_GROUND;
		
		for(int i = 0; i < 8; ++i)
		{
			E04_Entity* ball_entity = new E04_Entity();
			entity_create(state, ball_entity);
			ball_entity->transform.scale = VEC2F_ONE;

			vec2f size = itu_lib_sprite_get_world_size(context, &ball_entity->sprite, &ball_entity->transform);
			vec2f offset = -mul_element_wise(size, ball_entity->sprite.pivot - vec2f{ 0.5f, 0.5f });

			b2Circle ball_circle;
			ball_circle.radius = 0.5f;
			ball_circle.center = value_cast(b2Vec2, offset);
			balls_body_def.position = b2Vec2{ 3.0f + (i % 4) * 1.5f, (i / 4) * 3.0f };
			balls_body_def.rotation = b2MakeRot(SDL_randf() * TAU);
			balls_body_def.angularVelocity = 1;
			ball_entity->body_id = b2CreateBody(state->world_id, &balls_body_def);

			b2ShapeId col_id = b2CreateCircleShape(ball_entity->body_id, &balls_shape_def, &ball_circle);
			b2Shape_SetRestitution(col_id, 0.8);

			state->ball_shape_ids.push_back(col_id); // ADD SHAPE FOR RAY CHECKING

			b2ShapeId id = b2CreateCircleShape(ball_entity->body_id, &balls_shape_def_clutter, &ball_circle);
			itu_lib_sprite_init(
				&ball_entity->sprite,
				state->atlas,
				itu_lib_sprite_get_source_rect(3, 8, 16, 16)
			);
			b2Vec2 impulse = b2Vec2 { 100 - SDL_randf() * 200 ,  100 - SDL_randf() * 200  };
			auto circle = b2Shape_GetCircle(col_id);
			
			b2Body_ApplyLinearImpulse(ball_entity->body_id, impulse, balls_body_def.position, true);
		}
		for(auto shape : state->ball_shape_ids){
			b2Circle circle =  b2Shape_GetCircle(shape);

		}

	}

	// floor
	{
		// BOTTOM
		b2BodyDef body_def_bottom = b2DefaultBodyDef();
		body_def_bottom.type = b2_staticBody;
		body_def_bottom.position = b2Vec2{ 0, -7 };
		b2ShapeDef shape_def_bottom = b2DefaultShapeDef();

		shape_def_bottom.filter.categoryBits = COLLISION_FILTER_GROUND;
		b2Polygon polygon_bottom = b2MakeBox(8.0f, 1.0f);

		E04_Entity* entity_bottom = new E04_Entity();
		entity_create(state, entity_bottom);
		entity_bottom->body_id = b2CreateBody(state->world_id, &body_def_bottom);
		b2CreatePolygonShape(entity_bottom->body_id, &shape_def_bottom, &polygon_bottom);

		//TOP
		b2BodyDef body_def_top = b2DefaultBodyDef();
		body_def_top.type = b2_staticBody;
		body_def_top.position = b2Vec2{ 0, 7 };
		b2ShapeDef shape_def_top = b2DefaultShapeDef();

		shape_def_top.filter.categoryBits = COLLISION_FILTER_GROUND;
		shape_def_top.filter.maskBits = COLLISION_FILTER_CLUTTER | COLLISION_FILTER_CLUTTER_SENSOR;

		b2Polygon polygon_top = b2MakeBox(8.0f, 1.0f);

		E04_Entity* entity_top = new E04_Entity(); 
		entity_create(state, entity_top);
		entity_top->body_id = b2CreateBody(state->world_id, &body_def_top);
		b2CreatePolygonShape(entity_top->body_id, &shape_def_top, &polygon_top);

		//LEFT
		b2BodyDef body_def_left = b2DefaultBodyDef();
		body_def_left.type = b2_staticBody;
		body_def_left.position = b2Vec2{ -9, 0 };
		b2ShapeDef shape_def_left = b2DefaultShapeDef();

		shape_def_left.filter.categoryBits = COLLISION_FILTER_GROUND;
		shape_def_left.filter.maskBits = COLLISION_FILTER_CLUTTER | COLLISION_FILTER_CLUTTER_SENSOR;

		b2Polygon polygon_left = b2MakeBox(1.0f, 8.0f);

		E04_Entity* entity_left = new E04_Entity(); 
		entity_create(state, entity_left);
		entity_left->body_id = b2CreateBody(state->world_id, &body_def_left);
		b2CreatePolygonShape(entity_left->body_id, &shape_def_left, &polygon_left);

		//RIGHT
		b2BodyDef body_def_right = b2DefaultBodyDef();
		body_def_right.type = b2_staticBody;
		body_def_right.position = b2Vec2{ 9, 0 };
		b2ShapeDef shape_def_right = b2DefaultShapeDef();

		shape_def_right.filter.categoryBits = COLLISION_FILTER_GROUND;
		b2Polygon polygon_right = b2MakeBox(1.0f, 8.0f);

		E04_Entity* entity_right = new E04_Entity();
		entity_create(state, entity_right);
		entity_right->body_id = b2CreateBody(state->world_id, &body_def_right);
		b2CreatePolygonShape(entity_right->body_id, &shape_def_right, &polygon_right);

	}
	for(auto shape : state->ball_shape_ids){
			b2Circle circle =  b2Shape_GetCircle(shape);
			
	}


#if 0
	// clutter
	{
		b2BodyDef body_def = b2DefaultBodyDef();
		body_def.type = b2_dynamicBody;
		body_def.fixedRotation = false;

		// collider shape (to enable collisions with the ground)
		b2ShapeDef shape_def = b2DefaultShapeDef();
		shape_def.density = 1;
		shape_def.filter.categoryBits = COLLISION_FILTER_CLUTTER;

		// sensor shape (to enable interaction with the player)
		b2ShapeDef shape_def_clutter = b2DefaultShapeDef();
		shape_def_clutter.density = 0;
		shape_def_clutter.isSensor = true;
		shape_def_clutter.enableSensorEvents = true;
		shape_def_clutter.filter.categoryBits = COLLISION_FILTER_CLUTTER_SENSOR;
		shape_def_clutter.filter.maskBits     = COLLISION_FILTER_PLAYER;

		b2Polygon polygon_box = b2MakeBox(0.5f, 0.5f);
		for(int i = 0; i < 32; ++i)
		{
			E04_Entity* entity = new E04_Entity();
			entity_create(state, entity);
			entity->transform.scale = VEC2F_ONE;

			vec2f size = itu_lib_sprite_get_world_size(context, &entity->sprite, &entity->transform);
			vec2f offset = -mul_element_wise(size, entity->sprite.pivot - vec2f{ 0.5f, 0.5f });

			body_def.position = b2Vec2{ 3.0f + (i % 4) * 1.5f, (i / 4) * 3.0f };
			body_def.rotation = b2MakeRot(SDL_randf() * TAU);
			body_def.angularVelocity = 1;
			entity->body_id = b2CreateBody(state->world_id, &body_def);
			b2CreatePolygonShape(entity->body_id, &shape_def, &polygon_box);
			b2CreatePolygonShape(entity->body_id, &shape_def_clutter, &polygon_box);
			itu_lib_sprite_init(
				&entity->sprite,
				state->atlas,
				itu_lib_sprite_get_source_rect(3, 5, 16, 16)
			);
		}
	}
#endif

	// debug draw
	debug_draw.context = context;
	debug_draw.drawShapes = true;
	debug_draw.DrawSolidPolygonFcn = fn_box2d_wrapper_draw_polygon;
	debug_draw.DrawSolidCircleFcn  = fn_box2d_wrapper_draw_circle;
}

static void game_update(EngineContext* context, E04_GameState* state)
{
	if(context->btn_isjustpressed[BTN_TYPE_DEBUG_RESET])
		game_reset(context, state);

	// player
	{
		E04_Entity* player = state->player;
		E04_PlayerData* data = &state->player_data;

		switch(DEBUG_simulation_type_current)
		{
			case SIMULATION_TYPE_DYNAMIC:
			{
				//SDL_Log("DYNAMIC");
				vec2f force = VEC2F_ZERO;
				vec2f impulse = VEC2F_ZERO;

				if(context->btn_isjustpressed_action0){
					// has set first position?
					if(data->started){
						vec2f mouse_pos = itu_lib_context_point_screen_to_global(context, context->mouse_pos);
						data->endClick = {mouse_pos.x, mouse_pos.y};
						b2Vec2 norm = b2Normalize(data->endClick - data->startClick); 

						
						b2RayCastInput ray = {data->startClick, norm, 25};
						
						b2ShapeId cur_shape; // chosen shape to shoot
						b2CastOutput cur_output;

						float min_distance = FLOAT_MAX_VAL;


						for (auto shapeID : state->ball_shape_ids)
						{
							b2Circle circle = b2Shape_GetCircle(shapeID);
							circle.center = b2Body_GetPosition(b2Shape_GetBody(shapeID));
							b2CastOutput output = b2RayCastCircle(&ray, &circle);
							if(output.hit){
								if(b2Distance(data->startClick, circle.center) < min_distance){
									min_distance = b2Distance(data->startClick, circle.center);
									cur_shape = shapeID;
									cur_output = output;
								}
							}
							SDL_Log("RADIUS: %f", circle.radius);
							SDL_Log("X: %f Y: %f", circle.center.x, circle.center.y);
							SDL_Log("MOUSE X: %f Y: %f", data->endClick.x, data->endClick.y);
							SDL_Log("%d, %f", output.hit, output.fraction, output.iterations);
							
							//b2Body_ApplyLinearImpulse(b2Shape_GetBody(shapeID), norm * b2Distance(data->startClick, data->endClick), output.point, true);
							/* code */
						}
						if(cur_shape.index1 != NULL){
							b2Body_ApplyLinearImpulse(b2Shape_GetBody(cur_shape), norm * b2Distance(data->startClick, data->endClick), cur_output.point, true);


						}
						

						SDL_Log("RAY: X: %f, Y: %f", ray.translation.x, ray.translation.y);

						//reset
						data->started = false;
					}
					else{
						vec2f mouse_pos = itu_lib_context_point_screen_to_global(context, context->mouse_pos);
						data->startClick = {mouse_pos.x, mouse_pos.y};
						data->started = true;
					}
					
				}
				if(context->btn_isjustpressed_action1){
					data->started = false;
				}
						
				

				b2Body_ApplyForceToCenter(player->body_id, value_cast(b2Vec2, force), true);
				b2Body_ApplyLinearImpulseToCenter(player->body_id, value_cast(b2Vec2, impulse), true);
				break;
			}
			case SIMULATION_TYPE_KINEMATIC:
			{
				//SDL_Log("KINEMATIC");
				vec2f velocity = player->velocity;

				b2Body_SetLinearVelocity(player->body_id, value_cast(b2Vec2, velocity));
				break;
			}
		}
	}

	// NOTE: we are compouding precision errors here (config specifies frequency in steps per second, we convert to period in nanos,
	//       and here we convert back to seconds), but specifying what you want once and expressing everything else in function of
	//       it avoids mismatching. A more advanced implementation would have independent loop frequencies for game and physics and
	//       synch them under the hood (you can try it in exercise 04.2, will be discussed in class during exercise review)
	b2World_Step(state->world_id, NS_TO_SECONDS(context->target_framerate_fixed_ns), 4);

	// entities
	for(E04_Entity * entity : state->entities)
	{
		b2Vec2 physics_vel = b2Body_GetLinearVelocity(entity->body_id);

		

		b2Vec2 physics_pos = b2Body_GetPosition(entity->body_id);
		b2Rot  physics_rot = b2Body_GetRotation(entity->body_id);
		entity->velocity = value_cast(vec2f, physics_vel);
		entity->transform.position = value_cast(vec2f, physics_pos);
		entity->transform.rotation = b2Rot_GetAngle(physics_rot);
	}

	// player
	{
		E04_Entity* entity = state->player;
		E04_PlayerData* data = &state->player_data;


		// NOTE: quick hack because I forgot a VLA in the code again. Exercise solution will do this nicely
		static int contad_data_size = 16;
		static b2ContactData* contact_data = (b2ContactData*)SDL_calloc(16, sizeof(b2ContactData));

		int contacts = b2Body_GetContactCapacity(entity->body_id);

		if(contacts > contad_data_size)
		{
			contad_data_size = contacts;
			contact_data = (b2ContactData*)SDL_realloc(contact_data, contad_data_size * sizeof(b2ContactData));
		}
		int actual_contacts = b2Body_GetContactData(state->player->body_id, contact_data, contacts);

	}
	b2ContactEvents contact_events = b2World_GetContactEvents(state->world_id);
	
	b2ContactHitEvent * hitevents = contact_events.hitEvents;


	// world

	{
		const float zoom_speed = 1;
		vec2f camera_offset = vec2f { 0.0f, 3.0f } / context->camera_active->zoom;
		// camera follows player
		context->camera_active->world_position = state->player->transform.position + camera_offset;
		context->camera_active->zoom += context->mouse_scroll * zoom_speed * context->delta;
	}
}

static void game_render(EngineContext* context, E04_GameState* state)
{
	itu_lib_render_draw_world_grid(context);

	if(context->btn_isjustpressed[BTN_TYPE_DEBUG_F1]) DEBUG_render_textures = !DEBUG_render_textures;
	if(context->btn_isjustpressed[BTN_TYPE_DEBUG_F2]) DEBUG_render_outlines = !DEBUG_render_outlines;
	if(context->btn_isjustpressed[BTN_TYPE_DEBUG_F3]) DEBUG_physics = !DEBUG_physics;

	// entities
	for(auto entity : state->entities)
	{
		// render texture
		SDL_FRect rect_src = entity->sprite.rect;
		SDL_FRect rect_dst;

		if(DEBUG_render_textures)
			itu_lib_sprite_render(context, &entity->sprite, &entity->transform);

		if(DEBUG_render_outlines)
			itu_lib_sprite_render_debug(context, &entity->sprite, &entity->transform);
	}

	if(DEBUG_physics)
		b2World_Draw(state->world_id, &debug_draw);

	// debug window
	itu_lib_render_draw_world_point(context, VEC2F_ZERO, 10, color { 1, 0, 1, 1 });

	SDL_SetRenderDrawColor(context->renderer, 0xFF, 0x00, 0xFF, 0xff);
	SDL_RenderRect(context->renderer, NULL);
}

int main(void)
{
	bool quit = false;
	EngineConfig config = { 0 };
	EngineContext context = { 0 };
	E04_GameState  state   = { 0 };

	config.application_name = "E04 - Physics";
	config.window_w = 800;
	config.window_h = 600;
	config.step_per_second_fixed = 60;
	config.step_per_second_fluid = 60;
	config.texture_pixels_per_unit = 16;
	config.camera_pixel_per_unit = 32;

	itu_lib_context_init(&config, &context);
	itu_lib_imgui_setup(&context, true);

	itu_lib_context_set_active_camera(&context, &context.camera_default);

	game_init(&context, &state);
	game_reset(&context, &state);


	itu_lib_context_frame_timing_setup(&context);

	while(!quit)
	{
		// input
		quit = itu_lib_input_process_events(&context);

		if(context.btn_isjustpressed[BTN_TYPE_DEBUG_F1])
			context.debug_ui_show = !context.debug_ui_show;

		SDL_SetRenderDrawColor(context.renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(context.renderer);

		itu_lib_imgui_frame_begin(&context);

		// update
		game_update(&context, &state);
		game_render(&context, &state);

#ifdef ENABLE_DIAGNOSTICS
		if(context.debug_ui_show)
		{
			ImGui::Begin("itu_diagnostics", &context.debug_ui_show, 0);
			ImGui::PushItemWidth(200);
			ImGui::SeparatorText("Timing");
			ImGui::LabelText("work", "%6.3f ms/f", (float)context.elapsed_work / (float)MILLIS(1));
			ImGui::LabelText("tot", "%6.3f ms/f", (float)context.elapsed_frame / (float)MILLIS(1));


			ImGui::SeparatorText("Simulation config");
			if(ImGui::Combo("Simulation type", &DEBUG_simulation_type_current, DEBUG_simulation_types, array_size(DEBUG_simulation_types)))
				// reset simulation on type change
				game_reset(&context, &state);
			b2Vec2 player_pos_physics = b2Body_GetPosition(state.player->body_id);
			ImGui::LabelText("player pos game ", "%4.2f   %4.2f", state.player->transform.position.x, state.player->transform.position.y);
			ImGui::LabelText("player pos box2d", "%4.2f   %4.2f", player_pos_physics.x, player_pos_physics.y);
			b2Vec2 velocity = b2Body_GetLinearVelocity(state.player->body_id);
			ImGui::LabelText("player velocity", "%4.2f   %4.2f", velocity.x, velocity.y);

			ImGui::SeparatorText("Simulation type params");
			switch(DEBUG_simulation_type_current)
			{
				case SIMULATION_TYPE_DYNAMIC:
				{
					if(ImGui::DragFloat("gravity", &player_dynamic_gravity))
						b2World_SetGravity(state.world_id, b2Vec2 { 0, player_dynamic_gravity });
					ImGui::DragFloat("jump impulse", &player_dynamic_jump_impulse);
					ImGui::DragFloat("mov force", &player_dynamic_mov_force);
					break;
				}
				case SIMULATION_TYPE_KINEMATIC:
				{
					ImGui::DragFloat("horizontal velocity", &state.player_data.v_x);
					ImGui::DragFloat("jump height", &state.player_data.h);
					ImGui::DragFloat("jump distance", &state.player_data.x_h);
					ImGui::LabelText("jump gravity", "%4.2f", state.player_data.g);
					ImGui::LabelText("jump initial vertical velocity", "%4.2f", state.player_data.v_0);
					break;
				}
			}

			ImGui::SeparatorText("Debug");
			if(ImGui::Button("[TAB] reset"))
				game_reset(&context, &state);
			ImGui::Checkbox("render textures", &DEBUG_render_textures);
			ImGui::Checkbox("render outlines", &DEBUG_render_outlines);
			ImGui::Checkbox("render physics", &DEBUG_physics);

			ImGui::PopItemWidth();
			ImGui::End();
		}
#endif

		itu_lib_imgui_frame_end(&context);

		// render
		SDL_RenderPresent(context.renderer);

		itu_lib_context_frame_timing_update(&context);
	}
}
