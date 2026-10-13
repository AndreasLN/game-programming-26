#include <iostream>
#include "gameObject.hpp"

#include "component.hpp"
#include "monobehavior.hpp"
#include "player.hpp"
#include "sprite2D.hpp"


Player::Player(GameObject &gameObject, float player_size, double speed) : MonoBehavior(gameObject)
{
    this->name = "Player";
    this->horizontal = 0;
    this->vertical = 0;
    this->speed = speed;
}


void Player::Print() 
{
    std::cout << "This is a Player class";
}

void Player::Start()
{
    this->gameObject.GetComponents();
}


void Player::Update(SDL_Time time_elapsed_frame)
{
    
}





