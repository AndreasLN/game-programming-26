#include <iostream>
#include <gameObjectsLib.hpp>


Player::Player(GameObject &gameObject, double speed, EngineContext * context) : Character(gameObject, speed, context)
{
    this->name = "Player";
}


void Player::Print() 
{
    std::cout << "This is a Player class";
}

void Player::Start()
{ 
    Character::Start();
}

void Player::Update(float delta)
{

    const float player_speed = 2;

		vec2f mov = { 0 };
		if(context->btn_isdown_up){
			mov.y += 1;
			direction = UP;
		}		
		if(context->btn_isdown_down){
			mov.y -= 1;
			direction = DOWN;
		}	
		if(context->btn_isdown_left){
			mov.x -= 1;
			direction = LEFT;	
		}	
		if(context->btn_isdown_right){
			mov.x += 1;
			direction = RIGHT;
		}
				
		if(direction == LEFT && !sprite->sprite->flip_horizontal){
			sprite->sprite->flip_horizontal = true;
		}
		else if (direction == RIGHT && sprite->sprite->flip_horizontal){
			sprite->sprite->flip_horizontal = false;
		}

		gameObject.transform.position = gameObject.transform.position + mov * (speed * context->delta);

		// camera follows player
		context->camera_active->world_position = gameObject.transform.position;
}

void Player::Destroy(){
    delete this;
}


