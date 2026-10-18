#include <iostream>
#include <gameObjectsLib.hpp>


NPC::NPC(GameObject &gameObject, double speed, EngineContext * context) : Character(gameObject, speed, context)
{
    this->name = "NPC";
    this->state = IDLE;
    this->idle_timer = new Random_Timer(1, 3);
    this->moving_timer = new Random_Timer(2, 4);
}


void NPC::Print() 
{
    std::cout << "This is an NPC class";
}

void NPC::Start()
{ 
    Character::Start();
}

void NPC::Update(float delta)
{
    vec2f mov = { 0 };
		switch (state)
		{
			case IDLE:

				if (idle_timer->time > 0.0f){
					idle_timer->time -= context->delta;
				}
				else{
					state = MOVING;
					moving_timer->time = moving_timer->max + SDL_randf() * (moving_timer->min - moving_timer->max);
					direction = PlayerDirection(SDL_rand(LAST - 1));

				}
				break;
			case MOVING:
				if (moving_timer->time > 0){
					switch (direction)
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
					if(direction == LEFT && !this->sprite->sprite->flip_horizontal){
						this->sprite->sprite->flip_horizontal = true;
					}
					else if (direction == RIGHT && this->sprite->sprite->flip_horizontal){
						this->sprite->sprite->flip_horizontal = false;
					}

					gameObject.transform.position = gameObject.transform.position + mov * (speed * context->delta);
					moving_timer->time -= context->delta;
				}
				else{
					state = IDLE;
					idle_timer->time = idle_timer->max + SDL_randf() * (idle_timer->min - idle_timer->max);
				}
				break;

			default:
				break;
			}

   
}





