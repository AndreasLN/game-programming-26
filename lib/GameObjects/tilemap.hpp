#ifndef TILEMAP
#define TILEMAP

using namespace std;
#include <list>

using namespace std;
#include <map>

#include <itu_engine.hpp>
#include "monobehavior.hpp"
#include "rendered.hpp"
#include "itu_lib_render2d.hpp"

class Tilemap : public MonoBehavior, public Rendered
{
    private:
    
    public:
    map<vec2f, Sprite*> tiles; // easier lookup for entities in tilemap;
	Transform2D transform;

    Tilemap(GameObject &gameObject, SDL_Texture * texture, map<vec2f, SDL_FRect> textureMap);
    Tilemap(GameObject &gameObject, SDL_FRect rect, Uint8 r, Uint8 g, Uint8 b,  Uint8 a);

    void Awake() override;
    void Start() override;
    void Update(float delta) override;
    void Print();
    void Render(EngineContext * context);
    void Destroy();

};


#endif