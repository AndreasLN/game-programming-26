#ifndef PLAYER
#define PLAYER


#include <itu_engine.hpp>
#include "monobehavior.hpp"
#include "itu_lib_render2d.hpp"
#include "sprite2D.hpp"
#include "character.hpp"


class Player : public Character {
	
	public:

        Player(GameObject &gameObject, EngineContext * context); // Constructor with parameters}
		Player(GameObject &gameObject, double speed, EngineContext * context); // Constructor with parameters}
		
        void Update(float delta);
        void Start();
        void Move(float delta);
		void BorderCollision();
        void Print();
		void Destroy();

};

#endif