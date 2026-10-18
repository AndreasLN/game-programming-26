#include <iostream>
#include <gameObjectsLib.hpp>


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
    this->sprite = this->gameObject.GetComponentofType<Sprite2D>("Sprite2D");
}

void Player::Update(float delta)
{
}





