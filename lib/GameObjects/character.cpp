#include <iostream>
#include <gameObjectsLib.hpp>


Character::Character(GameObject &gameObject, double speed, EngineContext * context) : MonoBehavior(gameObject)
{
    this->name = "Character";
    this->speed = speed;
    this->direction = RIGHT;
    this->context = context;
    SDL_Log("HELLO?");
}

void Character::Print() 
{
    std::cout << "This is a Character class";
}

void Character::Start()
{ 
    this->sprite = this->gameObject.GetComponentofType<Sprite2D>("Sprite2D");
}

void Character::Destroy(){
    delete this;
}