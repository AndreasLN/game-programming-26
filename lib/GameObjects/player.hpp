#ifndef PLAYER
#define PLAYER


#include <itu_engine.hpp>
#include "monobehavior.hpp"
#include "itu_lib_render2d.hpp"
#include "sprite2D.hpp"


enum PlayerDirection{
	LEFT,
	RIGHT,
	UP,
	DOWN,
	LAST
};

class Player : public MonoBehavior {
	
	public:

		Sprite2D * sprite;
		int horizontal;
		int vertical;
		double speed;
		float player_size;
		PlayerDirection direction;
		EngineContext * context;
        Player(GameObject &gameObject, EngineContext * context); // Constructor with parameters}
		Player(GameObject &gameObject, float player_size, double speed, EngineContext * context); // Constructor with parameters}
		
        void Update(float delta);
        void Start();
        void Move(float delta);
		void BorderCollision();
        void Print();
		
};

#endif