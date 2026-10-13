#include <iostream>
#include "gameObject.hpp"

#include "component.hpp"
#include "monobehavior.hpp"
#include "sprite2D.hpp"

Sprite2D::Sprite2D(GameObject &gameObject, SDL_FRect rect) : MonoBehavior(gameObject)
{
    this->rect = rect;
}

Sprite2D::Sprite2D(GameObject &gameObject, SDL_FRect rect, Uint8 r, Uint8 g, Uint8 b,  Uint8 a) : MonoBehavior(gameObject)
{
    this->name = "Sprite2D";
    this->rect = rect;
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

void Sprite2D::RectRender(SDL_Renderer * renderer)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
	SDL_RenderFillRect(renderer, &rect);
}

void Sprite2D::Print() 
{
    std::cout << "This is a Sprite2D class";
}

void Sprite2D::Update(SDL_Time time_elapsed_frame)
{
    rect.x = gameObject.GetPos().x;
    rect.y = gameObject.GetPos().y;
}





