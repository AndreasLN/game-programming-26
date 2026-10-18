#ifndef NPCGUARD
#define NPCGUARD


#include <itu_engine.hpp>
#include "monobehavior.hpp"
#include "itu_lib_render2d.hpp"
#include "sprite2D.hpp"
#include "character.hpp"

struct Random_Timer
{
	float min;
	float max;

	float time = 2;

	Random_Timer(float min, float max) 
	: min(min), max(max) {time = 2;}

};

enum NPCSTATE{
	MOVING,
	IDLE,
};

class NPC : public Character {
	
	public:
        Random_Timer * idle_timer;
        Random_Timer * moving_timer;
	    NPCSTATE state;


        NPC(GameObject &gameObject, EngineContext * context); // Constructor with parameters}
		NPC(GameObject &gameObject, double speed, EngineContext * context); // Constructor with parameters}
		
        void Update(float delta);
        void Start();
        void Move(float delta);
		void BorderCollision();
        void Print();
		
};

#endif