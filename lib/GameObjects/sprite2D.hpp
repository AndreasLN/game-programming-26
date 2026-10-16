#ifndef SPRITE2D
#define SPRITE2D

#include <itu_engine.hpp>
#include "monobehavior.hpp"
#include "itu_lib_render2d.hpp"

class Sprite2D : public MonoBehavior{
    private:
    Sprite * sprite;
    Uint8 r = 0x00, g = 0x00, b = 0x00, a = 0XFF;
    SDL_FRect rect;
    
    public:
    Sprite2D(GameObject &gameObject, SDL_FRect rect);
    Sprite2D(GameObject &gameObject, SDL_FRect rect, Uint8 r, Uint8 g, Uint8 b,  Uint8 a);

    void Awake();
    void Start();
    void Update(SDL_Time time_elapsed_frame);
    void RectRender(SDL_Renderer * renderer);
    void Print();
};


#endif