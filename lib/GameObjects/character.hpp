#ifndef CHARACTER
#define CHARACTER


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

class Character : public MonoBehavior {
	
	public:

		Sprite2D * sprite;
		double speed;
		PlayerDirection direction;
		EngineContext * context;
        Character(GameObject &gameObject, EngineContext * context); // Constructor with parameters}
		Character(GameObject &gameObject, double speed, EngineContext * context); // Constructor with parameters}
        
        void Start();
        void Print();
		
};

#endif