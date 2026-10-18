#include <iostream>
#include <gameObjectsLib.hpp>

Sprite2D::Sprite2D(GameObject &gameObject, SDL_Texture * texture, SDL_FRect srcRect) : MonoBehavior(gameObject), Rendered()
{
    this->name = "Sprite2D";
    this->rect = srcRect;
    this->sprite = new Sprite();

    itu_lib_sprite_init(
        this->sprite,
        texture,
        itu_lib_sprite_get_source_rect(srcRect.x, srcRect.y, srcRect.w, srcRect.h)
	);
}

Sprite2D::Sprite2D(GameObject &gameObject, SDL_FRect rect, Uint8 r, Uint8 g, Uint8 b,  Uint8 a) : MonoBehavior(gameObject), Rendered()
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

void Sprite2D::Update(float delta) {
    rect.x = gameObject.GetTransform2D().position.x;
    rect.y = gameObject.GetTransform2D().position.y;
}

void Sprite2D::Render(EngineContext * context){

    itu_lib_sprite_render(context, this->sprite, &gameObject.transform);
}

void Sprite2D::Start() {

}

void Sprite2D::Awake() {
    MonoBehavior::Awake();
}

void Sprite2D::Destroy(){
    delete this;
}

