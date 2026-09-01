#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <iostream>


float window_w = 800;
float window_h = 600;


class NPC {
  public:
    float size;
	SDL_FRect * rect;
	int horizontal;
	int vertical;
	double speed;
	NPC(float size, int horizontal, int vertical, double speed, SDL_FRect * rect) { // Constructor with parameters
      this->size = size;
      this->horizontal = horizontal;
      this->vertical = vertical;
	  this->speed = speed;
	  this->rect = rect;
    }

	void move(SDL_Time time_elapsed_frame){
		(*rect).x += horizontal * speed * time_elapsed_frame;
		(*rect).y += vertical * speed * time_elapsed_frame;
	}
	void border_collision(float window_w, float window_h){

		if((*rect).x < 0){
				(*rect).x = 0;
				horizontal = 1;
		}
		if((*rect).x + (*rect).w > window_w){
			(*rect).x = window_w - (*rect).w;
			horizontal = -1;
		}
		if((*rect).y < 0){
			(*rect).y = 0;
			vertical = 1;
		}
		if((*rect).y + (*rect).h > window_h){
			(*rect).y = window_h - (*rect).h;
			vertical = -1;
		}
	}
};

class Player {
	public:
		SDL_FRect * rect;
		int horizontal;
		int vertical;
		double speed;
		float player_size;
		Player(float player_size, int horizontal, int vertical, double speed, SDL_FRect * rect) { // Constructor with parameters
			this->player_size = player_size;
			this->horizontal = horizontal;
			this->vertical = vertical;
			this->speed = speed;
			this->rect = rect;
			(*rect).w = player_size;
			(*rect).h = player_size;
			(*rect).x = window_w / 2 - player_size / 2;
			(*rect).y = window_h / 2 - player_size / 2;
		}
		void move(SDL_Time time_elapsed_frame){
			(*rect).x += horizontal * speed * time_elapsed_frame;
			(*rect).y += vertical * speed * time_elapsed_frame;
		}
		void border_collision(){
			if((*rect).x < 0){
					(*rect).x = 0;
			}
			if((*rect).x + (*rect).w > window_w){
				(*rect).x = window_w - (*rect).w;
			}
			if((*rect).y < 0){
				(*rect).y = 0;
			}
			if((*rect).y + (*rect).h > window_h){
				(*rect).y = window_h - (*rect).h;
			}
		}
};

void move(int horizontal, int vertical, double speed, SDL_Time time_elapsed_frame , SDL_FRect *player_rect)
{
	(*player_rect).x += horizontal * speed * time_elapsed_frame;
	(*player_rect).y += vertical * speed * time_elapsed_frame;
}

void border_collision(SDL_FRect *player_rect, float window_w, float window_h){

	if((*player_rect).x < 0){
			(*player_rect).x = 0;
	}
	if((*player_rect).x + (*player_rect).w > window_w){
		(*player_rect).x = window_w - (*player_rect).w;
	}
	if((*player_rect).y < 0){
		(*player_rect).y = 0;
	}
	if((*player_rect).y + (*player_rect).h > window_h){
		(*player_rect).y = window_h - (*player_rect).h;
	}

}

void NPC_border_collision(NPC npc, float window_w, float window_h){

	if((*npc.rect).x < 0){
			(*npc.rect).x = 0;
	}
	if((*npc.rect).x + (*npc.rect).w > window_w){
		(*npc.rect).x = window_w - (*npc.rect).w;
	}
	if((*npc.rect).y < 0){
		(*npc.rect).y = 0;
	}
	if((*npc.rect).y + (*npc.rect).h > window_h){
		(*npc.rect).y = window_h - (*npc.rect).h;
	}

}

int main(int argc, char* argv[])
{

	int target_framerate_ms = 1000 / 60;       // 16 milliseconds
	int target_framerate_ns = 1000000000 / 60; // 16666666 nanoseconds

	SDL_Window* window = SDL_CreateWindow("E00 - introduction", window_w, window_h, 0);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);


	// increase the zoom to make debug text more legible
	// (ie, on the class projector, we will usually use 2)
	{
		float zoom = 2;
		window_w /= zoom;
		window_h /= zoom;
		SDL_SetRenderScale(renderer, zoom, zoom);
	}

	

	bool quit = false;

	SDL_Time walltime_frame_beg;
	SDL_Time walltime_work_end;
	SDL_Time walltime_frame_end = 0;
	SDL_Time time_elapsed_frame;
	SDL_Time time_elapsed_work;

	SDL_Time time_elapsed_sleep;
	SDL_Time time_elapsed_busywait;

	int delay_type = 0;
	

	SDL_FRect player_rect;
	SDL_FRect player_rect_2;

	Player player_1(40.0f, 0, 0, 0.0000005, &player_rect);
	
	Player player_2(40.0f, 0, 0, 0.0000005, &player_rect_2);

	SDL_FRect NPC_rect;

	NPC npc(20, 1, 1, 0.0000001, &NPC_rect);

	NPC_rect.w = npc.size;
	NPC_rect.h = npc.size;
	NPC_rect.x = window_w / 2 - npc.size / 2;
	NPC_rect.y = window_h / 2 - npc.size / 2;

	bool btn_pressed_up = false;

	SDL_GetCurrentTime(&walltime_frame_beg);
	while(!quit)
	{
		//move(horizontal, vertical, speed, player_rect);
		
		//std::cout << horizontal;
		// input
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_EVENT_QUIT:
					quit = true;
					break;
				case SDL_EVENT_KEY_UP:
					switch (event.key.key)
						{
						case SDLK_W:
							//std::cout << "hello";
							if(player_1.vertical < 0)
								player_1.vertical = 0;
							break;
						case SDLK_A:
							//cout << "LEFT";
							if(player_1.horizontal < 0)
								player_1.horizontal = 0;
							break;
						case SDLK_S:
							//cout << "DOWN";
							if(player_1.vertical > 0)
								player_1.vertical = 0;
							break;
						case SDLK_D:
							//cout << "RIGHT";
							if(player_1.horizontal > 0)
								player_1.horizontal = 0;
							break;
						case SDLK_UP:
							//std::cout << "hello";
							if(player_2.vertical < 0)
								player_2.vertical = 0;
							break;
						case SDLK_LEFT:
							//cout << "LEFT";
							if(player_2.horizontal < 0)
								player_2.horizontal = 0;
							break;
						case SDLK_DOWN:
							//cout << "DOWN";
							if(player_2.vertical > 0)
								player_2.vertical = 0;
							break;
						case SDLK_RIGHT:
							//cout << "RIGHT";
							if(player_2.horizontal > 0)
								player_2.horizontal = 0;
							break;
						}
					break;
				case SDL_EVENT_KEY_DOWN:
					if(event.key.key >= SDLK_1 && event.key.key < SDLK_6){
						delay_type = event.key.key - SDLK_1;
						break;
					}
					else{
						switch (event.key.key)
						{
							case SDLK_D:
								//cout << "RIGHT";
								player_1.horizontal = 1;
								break;
							case SDLK_A:
								//cout << "LEFT";
								player_1.horizontal = -1;
								break;
							case SDLK_W:
								player_1.vertical = -1;
								break;
							case SDLK_S:
								//cout << "DOWN";
								player_1.vertical = 1;
								break;
							case SDLK_RIGHT:
								//cout << "RIGHT";
								player_2.horizontal = 1;
								break;
							case SDLK_LEFT:
								//cout << "LEFT";
								player_2.horizontal = -1;
								break;
							case SDLK_UP:
								player_2.vertical = -1;
								break;
							case SDLK_DOWN:
								//cout << "DOWN";
								player_2.vertical = 1;
								break;
						}
					}
					break;

			}
		}
		// clear screen
		// NOTE: `0x` prefix means we are expressing the number in hexadecimal (base 16)
		//       `0b` is another useful prefix, expresses the number in binary
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(renderer);
		
		SDL_SetRenderDrawColor(renderer, 0x3C, 0x63, 0xFF, 0XFF);
		SDL_RenderFillRect(renderer, &player_rect);

		SDL_SetRenderDrawColor(renderer, 0x3C, 0x63, 0x00, 0XFF);
		SDL_RenderFillRect(renderer, &player_rect_2);

		SDL_SetRenderDrawColor(renderer, 0x63, 0x00, 0x00, 0XFF);
		SDL_RenderFillRect(renderer, npc.rect);


		SDL_GetCurrentTime(&walltime_work_end);
		//SDL_Log("%lu, %lu\n", walltime_work_end, walltime_frame_beg);
		time_elapsed_work = walltime_work_end - walltime_frame_beg;

		if(target_framerate_ns > time_elapsed_work)
		{
			switch(delay_type)
			{
				case 0:
				{
					// busy wait - very precise, but costly
					walltime_frame_end = walltime_work_end;
					while(walltime_frame_end - walltime_frame_beg < target_framerate_ns)
						SDL_GetCurrentTime(&walltime_frame_end);

					time_elapsed_busywait = walltime_frame_end - walltime_work_end;
					time_elapsed_sleep = 0;
					break;
				}
				case 1:
				{
					// simple delay - too imprecise
					// NOTE: `SDL_Delay` gets milliseconds, but our timer gives us nanoseconds! We need to covert it manually
					SDL_Delay((target_framerate_ns - time_elapsed_work) / 1000000);
					SDL_GetCurrentTime(&walltime_frame_end);

					time_elapsed_busywait = 0;
					time_elapsed_sleep = walltime_frame_end - walltime_frame_beg;
					break;
				}
				case 2:
				{
					// delay ns - also too imprecise
					SDL_DelayNS(target_framerate_ns - time_elapsed_work);
					SDL_GetCurrentTime(&walltime_frame_end);

					time_elapsed_busywait = 0;
					time_elapsed_sleep = walltime_frame_end - walltime_frame_beg;
					break;
				}
				case 3:
				{
					// delay precise
					SDL_DelayPrecise(target_framerate_ns - time_elapsed_work);
					SDL_GetCurrentTime(&walltime_frame_end);

					time_elapsed_busywait = 0;
					time_elapsed_sleep = walltime_frame_end - walltime_frame_beg;
					break;
				}
				case 4:
				{
					// custom delay - we use the sleeping delay with an arbitrary margin, then we busywait what's left
					const Uint64 sleep_safety_margin = 1000000; // ie, 1ms
					SDL_Time walltime_sleep_end;
					
					SDL_DelayNS(target_framerate_ns - time_elapsed_work - sleep_safety_margin);
					SDL_GetCurrentTime(&walltime_sleep_end);
					walltime_frame_end = walltime_sleep_end;

					while(walltime_frame_end - walltime_frame_beg < target_framerate_ns)
						SDL_GetCurrentTime(&walltime_frame_end);

					time_elapsed_busywait = walltime_frame_end - walltime_sleep_end;
					time_elapsed_sleep = walltime_sleep_end - walltime_work_end;
					break;
				}
			}
		}

		time_elapsed_frame = walltime_frame_end - walltime_frame_beg;

		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderDebugTextFormat(renderer, 10.0f, 10.0f, "elapsed (frame): %9.6f ms", (float)time_elapsed_frame/(float)1000000);
		SDL_RenderDebugTextFormat(renderer, 10.0f, 20.0f, "elapsed(work   : %9.6f ms", (float)time_elapsed_work/(float)1000000);
		SDL_RenderDebugTextFormat(renderer, 10.0f, 30.0f, "delay type: %d (change with 1-5)", delay_type + 1);

		SDL_RenderDebugTextFormat(renderer, 10.0f, 50.0f, "time spent sleeping   : %9.6f ms", (float)time_elapsed_sleep/(float)1000000);
		SDL_RenderDebugTextFormat(renderer, 10.0f, 60.0f, "time spent busywaiting: %9.6f ms", (float)time_elapsed_busywait/(float)1000000);


		player_1.move(time_elapsed_frame);
		player_2.move(time_elapsed_frame);

		npc.move(time_elapsed_frame);

		npc.border_collision(window_w, window_h);
		player_1.border_collision();
		player_2.border_collision();

		

		// render
		SDL_RenderPresent(renderer);
		
		//walltime_frame_beg = walltime_frame_end;

		// NOTE: while taking the time two different times is no ideal, in the current setup we have a problem:
		//       our `time_elapsed_frame` doesn't take into account the time it takes to render the debug view, AND
		//       to "present" the graphics. Usually that is not a big deal, but if if the untimed stuff takes too long
		//       we end up in a death spiral! Our options are:
		//       - take the time again after everything has been done (which means the time logged will be slightly lower)
		//       - show the elapsed time from the previous frame
		//       we will go with the first option here
		// walltime_frame_beg = walltime_frame_end;
		SDL_GetCurrentTime(&walltime_frame_beg);
	}

	// NOTE: we created a bunch of resources (window, renderer). Should we explicitely destroy them here?
	//       it's actually not a trivial question!
	
	return 0;
};
