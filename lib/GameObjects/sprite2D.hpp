#ifndef SPRITE2D
#define SPRITE2D

#include <itu_engine.hpp>
#include "monobehavior.hpp"
#include "rendered.hpp"
#include "itu_lib_render2d.hpp"

class Sprite2D : public MonoBehavior, public Rendered
{
    private:
    Uint8 r = 0x00, g = 0x00, b = 0x00, a = 0XFF;
    SDL_FRect rect;
    
    public:
    Sprite * sprite;
    
    Sprite2D(GameObject &gameObject, SDL_Texture * texture, SDL_FRect srcRect);
    Sprite2D(GameObject &gameObject, SDL_FRect rect, Uint8 r, Uint8 g, Uint8 b,  Uint8 a);

    void Awake() override;
    void Start() override;
    void Update(float delta) override;
    void RectRender(SDL_Renderer * renderer);
    void Print();
    void Render(EngineContext * context);
    void Destroy();
};


#endif