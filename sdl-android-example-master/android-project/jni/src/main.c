// Hot Rocks
#define APPNAME "Hotrocks"

#include "main.h"
#include "SDL_main.h"

Vector min_seperate(Vector p1, int s1, Vector p2, int s2);
void move_ship2(ship *sh,asteroid *field, bullet *bulls, enemy *en);
void collide2(Vector *v1, Vector *p1, int s1, Vector *v2, Vector *p2, int s2);
int keyboard_events(Player *p1, Player *p2, int mode);
int pause_game();
void player_wins(int n);
void init_powerup(float x, float y);
void collect_powerup(enemy *en, Player *p, int i);
int EventFilter(void *, SDL_Event * event);

SDL_Window* mywindow;				// the usual window
int dual_bullets;
int no_of_enemy = 0;
int no_of_asteroids = 0;
int no_of_powerups = 0;
int	SCREEN_WIDTH = 1024;
int	SCREEN_HEIGHT = 576;
int NO_OF_EXPLOSIONS = 4;		// Must be divisor of MAX_PARTICLES
int MAX_PARTICLES = 128;		// Must be multiple of NO_OF_EXPLOSIONS
int render = 1;
float time_delta = 0;			// the time elapsed since last frame was rendered
GLuint tex;
Camera camera;
explosion* explode = NULL;		// pointer to array sized to Number of simultaneous explosions
particle* parts = NULL;			// pointer to array for particle engine
powerup powerups[MAX_POWERUPS]; // currently 3
int cam = 1; 					// 0 = fixed, 1 = lag, 2 = traditional

int init_game(){

	if (init_sdl() != 0){
		printf("initialisation error\n");
		close_sdl();
		return 1;
	}
	init_sound();
	// load graphics
	tex = load_texture("stars.png");			// background image
	powertex = load_texture("power.png");	
	explode = malloc(NO_OF_EXPLOSIONS * sizeof(explosion));	// allocate memory for arrays	
	parts = malloc(MAX_PARTICLES * sizeof(particle));		// which depend on detail setting
	return 0;
}

int main(int argc, char *argv[]){

	srand(time(NULL));	
	
	int mode = 0;
	Player p[2];
	Player *p1, *p2;
	p1 = &p[0];
	p2 = &p[1];
	if (init_game()){
		__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "init failure");
		goto end;
	}
	set_game_mode:
	mode = main_menu(tex);
	if (mode == 99)
		goto end;

//	SDL_SetEventFilter(EventFilter, nullptr);
	static asteroid field[MAX_ASTEROIDS];							// init some more stufff
	static enemy en[MAX_ENEMY];
	
	int quit = 0;					// user wants to quit flag
	int status = 0;
	int level = 1;					// starting level
	int endtimer = 0;				// end of level or respawn timer
	int eton = 0;					// end timer running flag
	
	init_level(&field[0], &p[0], &en[0], level, mode);
	init_particles();

	Uint32 startclock = 0;
	Uint32 deltaclock = 0;
	Uint32 oldclock = SDL_GetTicks();
	Uint32 time_delta = 0;
	
//********************************** MAIN LOOP *****************************************//

	while (!quit){

		// FRAMES PER SECOND
		startclock = SDL_GetTicks();				// current time
		deltaclock = startclock - oldclock;			// frame time = new time - previous
		oldclock = startclock;						// 
		
		time_delta += deltaclock;					// time elapsed in render state

		

		// ------------------------------- GAME LOGIC -------------------------------------

		// Player has no lives left, so start end timer for 3 secs then end game
		if (p1->lives <= 0){

			if (mode == 0){								// Player 1 dead in 1 player game
				
				if (eton == 0){
					eton = 1;
					endtimer = SDL_GetTicks();
				}
				if (SDL_GetTicks() - endtimer > 3000){		
				
					high_scores(p1->score, 0, tex);
					eton = 0;
					goto set_game_mode;
				}
			}
			else if (mode == 1 && p2->lives == 0){		// Player 1+2 dead in 2 player game
				if (eton == 0){
					eton = 1;
					endtimer = SDL_GetTicks();
				}
				if (SDL_GetTicks() - endtimer > 3000){			
					high_scores(p1->score, p2->score, tex);
					eton = 0;
					goto set_game_mode;
				}
			}
			else if (mode == 2){						// Player 1 dead in 2 player duel
				if (eton == 0){
					eton = 1;
					endtimer = SDL_GetTicks();
				}
				if (SDL_GetTicks() - endtimer > 3000){	
					player_wins(2);
					goto set_game_mode;
				}
			}
		}

		if (mode == 2 && p2->lives <= 0){				// Player 2 dead in 2 player duel
			if (eton == 0){
					eton = 1;
					endtimer = SDL_GetTicks();
			}
			if (SDL_GetTicks() - endtimer > 3000){		
			
				player_wins(1);
				eton = 0;
				goto set_game_mode;
			}
		}

		// end of level
		if (no_of_asteroids < 1 && mode != 2 && no_of_enemy < 1){
			if (eton == 0){
				eton = 1;
				endtimer = SDL_GetTicks();
			}
			if (SDL_GetTicks() - endtimer > 3000){
				++level;
				eton = 0;
				init_particles();
				init_level(&field[0], &p[0], &en[0], level, mode);
				oldclock = SDL_GetTicks();
			}
		}
// -----------------------------------PHYSICS-----------------------------

		while (time_delta >= DELTA_P){				// update physics in fixed timesteps

			status = keyboard_events(p1, p2, mode);
		
			if (status == 2){

				pause_game();
				oldclock = SDL_GetTicks();
			}
			if (status == 99)
				quit = 1;
			
			//printf("delta %d\n", time_delta);
			time_delta -= DELTA_P;					// subtract one time period from accumulator
			
			// moving camera
			if (cam !=2 && mode == 0){
				camera.x -= p1->sh.velocity.x;
				camera.y -= p1->sh.velocity.y;
			}
			
			// 1 PLAYER MODE LOGIC

			if (p1->lives > 0){
				collision_detect(&field[0], p1, en);
				move_bullets(&p1->bulls[0]);
				
				if (p1->sh.shield == 1){
					if (SDL_GetTicks() - p1->sh.shield_timer >= 100){
						p1->sh.shield_power -= 1;
						p1->sh.shield_timer = SDL_GetTicks();
						if (p1->sh.shield_power < 1.0){
							p1->sh.shield_power = 0;
							p1->sh.shield = 0;
						}
					}
				}
		
				// is player fixed at screen center?	
				if (cam > 0)
					move_ship(&p1->sh);
				else
					move_ship2(&p1->sh, &field[0], &p1->bulls[0], en);	
			}
	
			// 2 PLAYER MODE LOGIC

			if (mode > 0 && p2->lives > 0){
				
				move_ship(&p2->sh);
				move_bullets(&p2->bulls[0]);
				
				if (mode == 1){
					collision_detect(&field[0], p2, en);
				}
				
				if (mode == 2){
					dual_collisions(p1, p2);
				}
				
				if (p2->sh.shield == 1){
					if (SDL_GetTicks() - p2->sh.shield_timer >= 100){
						p2->sh.shield_power -= 1;
						p2->sh.shield_timer = SDL_GetTicks();
						if (p2->sh.shield_power < 1)
							p2->sh.shield = 0;
					}
				}
			}	
	
			// not dual mode
			if (mode != 2){
				move_asteroid(&field[0]);			
			}
	
			// ENEMY PHYSICS
			if (no_of_enemy > 0){
						
				for (int i = 0; i < no_of_enemy; ++i){
					move_bullets(en[i].bulls);
				}
	
				move_enemies(en, field, p, mode);
			}
		}	// END OF PHYSICS

// ------------------------------------RENDER----------------------------------------------		
		// DRAWING CODE 1 PLAYER

		glMatrixMode(GL_MODELVIEW);
		glClear (GL_COLOR_BUFFER_BIT);
	
		// DRAW POWERUPS
		if (no_of_powerups > 0){
			for (int i = 0; i < no_of_powerups; ++i){
				draw_powerup(&powerups[i]);
			}
		}
		// DRAW ASTEROIDS
		draw_asteroids(field);
		// DRAW SHIP
		if(p1->lives > 0){
			draw_ship(&p1->sh, 0);	
			// DRAW SHIELD	
			if (p1->sh.shield == 1)
				draw_shield(&p1->sh);
			// DRAW BULLETS
			draw_bullets(p1->bulls);
		}
		// DRAW PARTICLES
		draw_particles();
	
		// DRAWING CODE 2 PLAYER

		if (mode > 0 && p2->lives > 0){ 
			draw_ship(&p2->sh, 1);
			if (p2->sh.shield == 1)
				draw_shield(&p2->sh);
			draw_bullets(p2->bulls);
		}

		// DRAW ENEMIES
		
		if (no_of_enemy > 0){
					
			for (int i = 0; i < no_of_enemy; ++i){
				
				draw_ship(&en[i].sh, 2);
				draw_bullets(en[i].bulls);
				
			}	
		}

		// DRAW EXPLOSIONS

		for (int i = 0; i < NO_OF_EXPLOSIONS; ++i){
			if (explode[i].age > 0.1){
				explode[i].age -= 0.75;
				draw_explosion(&explode[i]);
			}
		}
		draw_background(tex);
		draw_scorebar(p, mode);
		SDL_GL_SwapWindow( mywindow );

		SDL_Delay(10);

		
	}

	close_sdl();
	end:
	return 0;
}	

void init_level(asteroid *field, Player *p, enemy *en, int level, int mode){

	no_of_enemy = 0;
	no_of_powerups = 0;
	cam = 1;
	int x;
	int no_of_players;
	if (mode > 0)
		no_of_players = 2;
	else
		no_of_players = 1;

	if (mode == 2)
		no_of_asteroids = 0;
	else
		no_of_asteroids = 2 + level;
	// init players
	for (x = 0; x < no_of_players; ++x){
		if(level == 1){
			p[x].score = 0;
			p[x].lives = 5;
		}	
		init_player(&p[x], x, no_of_players);
	}

	if (level%3 == 0){
		cam = 1;
		no_of_asteroids = 0;
		//p->sh.fire = create_bullet;
		for(int i = 0; i < level; ++i){
			create_enemies(en, 1);
			if (i == 9)
				break;
		}
	}

	// init asteroids
	for (int i = 0; i < no_of_asteroids; ++i){
		init_asteroid(&field[i], 3 * (1 + (i % 4)));
		do{
			field[i].position.x = (SCREEN_WIDTH / no_of_asteroids) * (i + 0.5);
			field[i].position.y = ((float)rand()/(float)RAND_MAX) * SCREEN_HEIGHT;
		}while (field[i].position.y < SCREEN_HEIGHT/2 + 100 && field[i].position.y > SCREEN_HEIGHT/2 - 100);
	}

	level_transition(level);
}

void init_player(Player *p, int id, int num){

	if (p->lives == 0)			// if dead return
		return;
		
	p->sh.shield_power = 99;
	p->sh.shield_timer = 1;
	p->sh.shield = 1;
	p->sh.velocity.x = 0;
	p->sh.velocity.y = 0;
	p->sh.gun = 0;

	if (cam == 0){
		p->sh.fire = create_bullet;
	}
	else{
		p->sh.fire = create_bullet_rel;
	}

	for (int i = 0; i < MAX_BULLETS; ++i)
		p->bulls[i].position.x = -99999;

	if (num == 2){
		if (id == 0){
			p->sh.position.x = SCREEN_WIDTH/2 - 100;
			p->sh.position.y = SCREEN_HEIGHT/2;
			p->sh.angle = -PI;
		}
		if (id == 1){
			p->sh.position.x = SCREEN_WIDTH/2 + 100;
			p->sh.position.y = SCREEN_HEIGHT/2;
			p->sh.angle = 0;
		}
	}		
	else {
		p->sh.position.x = SCREEN_WIDTH/2;
		p->sh.position.y = SCREEN_HEIGHT/2;
		p->sh.angle = -PI/2;
	}

	for (int i = 0; i < 12; ++i){
		p->sh.thrust[i].velocity.x = 0;
		p->sh.thrust[i].velocity.y = 0;
		p->sh.thrust[i].position.x = 0;
		p->sh.thrust[i].position.y = 0;
		p->sh.thrust[i].state = 0;
		p->sh.thrust[i].age = 4.5;
	}
}

void init_asteroid(asteroid *dave, int size){

	float speed;
	float direction;
	
	dave->angle = ((float)rand()/(float)RAND_MAX) * 2.0 * PI;	
	dave->angle_inc = (((float)rand()/(float)RAND_MAX)-0.5)/20.0;
	dave->has_collided = 0;
	dave->size = size;
	
	// Generate random starting speed vector.
	speed = ((float)rand()/(float)RAND_MAX) * 3;
	direction = ((float)rand()/(float)RAND_MAX) * 2.0 * PI;
	dave->velocity.x = speed * cos(direction);
	dave->velocity.y = speed * sin(direction);
}

void init_particles(){

	for (int i = 0; i < MAX_PARTICLES; ++i){
		parts[i].position.x = 0;
		parts[i].position.y = 0;
		float speed;
		float direction;
		parts[i].state = 0;
		parts[i].age = ((float)rand()/(float)RAND_MAX);
		speed = ((float)rand()/(float)RAND_MAX) * 8;
		direction = ((float)rand()/(float)RAND_MAX) * 2.0 * PI;
		parts[i].velocity.x = speed * cos(direction);
		parts[i].velocity.y = speed * sin(direction);
	}

	for (int i = 0; i < NO_OF_EXPLOSIONS; ++i){
		explode[i].x = 0;
		explode[i].y = 0;
		explode[i].age = 0;
	}
}

void set_parts(int x, int y){

	// simple particle engine that handles NO_OF_EXPLOSIONS simultaneous events
	
	static int id = 0;
	int chunk = MAX_PARTICLES/NO_OF_EXPLOSIONS;		// number of particles per explosion

	id = (id + 1)%NO_OF_EXPLOSIONS;					// Get the next available chunk of particles
	
	explode[id].x = x;								// and set the origin and lifespan
	explode[id].y = y;
	explode[id].age = 4.5;

	for (int i = chunk * id; i < chunk * (id + 1); ++i){
		parts[i].position.x = x;
		parts[i].position.y = y;
		parts[i].state = 1;
		parts[i].age = ((float)rand()/(float)RAND_MAX);
	}
		
}		

void init_sound(){

	int audio_rate = 22050;
	//Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
	int audio_channels = 2;
	int audio_buffers = 1024;
   
    SDL_Init(SDL_INIT_AUDIO);

	/* open up our audio device. Load the sounds */
     
	if(Mix_OpenAudio(audio_rate, MIX_DEFAULT_FORMAT, audio_channels, audio_buffers)) {
    	printf("Unable to open audio!\n");
    	exit(1);
  		}
  	Mix_AllocateChannels(64);
  	Mix_ReserveChannels(1); 			// reserve channel zero for shield
  	
	sound[CRASH].effect = Mix_LoadWAV( "crash.wav" );
	sound[THRUST].effect = Mix_LoadWAV( "thrust.wav" );
	sound[FIRE].effect = Mix_LoadWAV( "fire.wav" );
	sound[HIT].effect = Mix_LoadWAV( "hit.wav" );
	sound[SHIELD].effect = Mix_LoadWAV( "shield.wav" );
	sound[IMPACT].effect = Mix_LoadWAV( "impact.wav" );
	sound[SELECTS].effect = Mix_LoadWAV( "select.wav" );
	sound[BOUNCE].effect = Mix_LoadWAV( "bounce.wav" );
	sound[HYPER].effect = Mix_LoadWAV( "hyper.wav" );
	sound[ENTRY].effect = Mix_LoadWAV( "entry.wav" );
	sound[POWER].effect = Mix_LoadWAV( "power.wav" );
}

void init_thruster(ship *sh){

	float posx = sh->position.x - 12*cos(sh->angle);
	float posy = sh->position.y - 12*sin(sh->angle);
	float speed;
	float direction;

	for (int i = 0; i < 12; ++i){
		sh->thrust[i].position.x = posx;
		sh->thrust[i].position.y = posy;
		sh->thrust[i].state = 1;
		sh->thrust[i].age = ((float)rand()/(float)RAND_MAX);
		speed = ((float)rand()/(float)RAND_MAX) * 8;
		direction = ((float)rand()/(float)RAND_MAX)/2 - 0.25;
		direction = sh->angle + PI + direction;
		sh->thrust[i].velocity.x = speed * cos(direction);
		sh->thrust[i].velocity.y = speed * sin(direction);
	}
}
// bullets that move relative to ships velocity
void create_bullet_rel(bullet *bulls, ship *sh){
	
	int speed = 8;
	float shx, shy, vx, vy, m, dot;
	float dual_offset = -0.2;
	
	if (sh->gun == 1){

		for (int i = 0; i < MAX_BULLETS; ++i){		// scan through bullets until we find one that is free

			if (bulls[i].position.x == -99999){

				shx = speed * cos(sh->angle + dual_offset);
				shy = speed * sin(sh->angle + dual_offset);
				bulls[i].position.x = sh->position.x + 3 * shx;
				bulls[i].position.y = sh->position.y + 3 * shy;

				shx = speed * cos(sh->angle);
				shy = speed * sin(sh->angle);
				m = sqrt(shx*shx + shy*shy);
				vx = shx/m;
				vy = shy/m;
				dot = vx * sh->velocity.x + vy * sh->velocity.y;
				vx = vx * dot;
				vy = vy * dot;
				bulls[i].velocity.x = shx + sh->velocity.x;
				bulls[i].velocity.y = shy + sh->velocity.y;

				if (dual_offset > 0){
					play_sound(FIRE, 0);
					return;
				}
				dual_offset = 0.2;
			}
		}
	}

	for (int i = 0; i < MAX_BULLETS; ++i){		// scan through bullets until we find one that is free
		
		if (bulls[i].position.x == -99999){
			play_sound(FIRE, 0);
			shx = speed * cos(sh->angle);
			shy = speed * sin(sh->angle);
			bulls[i].position.x = sh->position.x + 3 * shx;
			bulls[i].position.y = sh->position.y + 3 * shy;
			m = sqrt(shx*shx + shy*shy);
			vx = shx/m;
			vy = shy/m;
			dot = vx * sh->velocity.x + vy * sh->velocity.y;
			vx = vx * dot;
			vy = vy * dot; 
			bulls[i].velocity.x = shx + sh->velocity.x;
			bulls[i].velocity.y = shy + sh->velocity.y;
			
			return;
		}
	}
	return;
}
// bullets that move relative to screen		
void create_bullet(bullet *bulls, ship *sh){

	
	int speed = 8;
	float shx, shy;
	
	for (int i = 0; i < MAX_BULLETS; ++i){
		
		if (bulls[i].position.x == -99999){
			play_sound(FIRE, 0);
			shx = speed * cos(sh->angle);
			shy = speed * sin(sh->angle);
			bulls[i].position.x = sh->position.x + 3 * shx;
			bulls[i].position.y = sh->position.y + 3 * shy;
			bulls[i].velocity.x = shx;
			bulls[i].velocity.y = shy;
			
			return;
		}
	}
	return;
}

void init_powerup(float x, float y){

	if (no_of_powerups == MAX_POWERUPS)
		return;

	powerups[no_of_powerups].position.x = x;
	powerups[no_of_powerups].position.y = y;
	powerups[no_of_powerups].age = 0;

	float rn = ((float)rand()/(float)RAND_MAX);
	if (rn > 0.75)
		powerups[no_of_powerups].type = 3;
	else if (rn > 0.5)
		powerups[no_of_powerups].type = 2;
	else if (rn > 0.25)
		powerups[no_of_powerups].type = 1;
	else
		powerups[no_of_powerups].type = 3;

	no_of_powerups += 1;
}

void move_asteroid(asteroid *field){

	// For each asteroid in the field update its coordinates.
	for (int i = 0; i < no_of_asteroids; ++i){

		field[i].angle = field[i].angle + field[i].angle_inc * DELTA;

		// check for boundry conditions
		
		if (field[i].position.x > SCREEN_WIDTH + field[i].size * 5){
			//~ field[i].position.x = 1 - field[i].size * 5;
			field[i].position.x = -field[i].size * 5;
		}
		if (field[i].position.x < 0 - field[i].size * 5){
			//~ field[i].position.x = SCREEN_WIDTH + field[i].size * 5;
			field[i].position.x = SCREEN_WIDTH + field[i].size * 5;
		}
		if (field[i].position.y > SCREEN_HEIGHT + field[i].size * 5){
			//~ field[i].position.y = 1 - field[i].size * 5;
			field[i].position.y = - field[i].size * 5;
		}
		if (field[i].position.y < 0 - field[i].size * 5){
			//~ field[i].position.y = SCREEN_HEIGHT + field[i].size * 5;
			field[i].position.y = SCREEN_HEIGHT + field[i].size * 5;
		}

		// update position
		field[i].position.x = field[i].position.x + field[i].velocity.x * DELTA;
		field[i].position.y = field[i].position.y + field[i].velocity.y * DELTA;
	}	

}	// end move asteroid
// standard way to move ship
void move_ship(ship *sh){
	
	if (sh->position.x > SCREEN_WIDTH ){
		sh->position.x = 0 ;
	}
	if (sh->position.x < 0 ){
		sh->position.x = SCREEN_WIDTH ;
	}
	if (sh->position.y > SCREEN_HEIGHT ){
		sh->position.y = 0 ;
	}
	if (sh->position.y < 0 ){
		sh->position.y = SCREEN_HEIGHT ;
	}
	sh->position.x = sh->position.x + sh->velocity.x * DELTA;	// new position = old + current speed * delta time since last moved
	sh->position.y = sh->position.y + sh->velocity.y * DELTA;
	
}
// ship remains fixed and all other objects are moved relative
void move_ship2(ship *sh,asteroid *field, bullet *bulls, enemy *en){

	for (int i = 0; i < no_of_asteroids; ++i){
		field[i].position.x += -sh->velocity.x * DELTA;
		field[i].position.y += -sh->velocity.y * DELTA;
	}
	for (int i = 0; i < MAX_BULLETS; ++i){
		if (bulls[i].position.x != -99999){
			bulls[i].position.x += bulls[i].velocity.x;
			bulls[i].position.y += bulls[i].velocity.y;
		}
	}
	for (int i = 0; i < no_of_enemy; ++i){
		en[i].sh.position.x += -sh->velocity.x * DELTA;
		en[i].sh.position.y += -sh->velocity.y * DELTA;
		for (int j = 0; j < MAX_BULLETS; ++j){
			if (bulls[j].position.x != -99999){
				bulls[j].position.x += -sh->velocity.x * DELTA;
				bulls[j].position.y += -sh->velocity.y * DELTA;
			}
		}
	}
}

void move_bullets(bullet *bulls){

	for (int i = 0; i < MAX_BULLETS; ++i){
		if (bulls[i].position.x < 0 || bulls[i].position.x > SCREEN_WIDTH)
			bulls[i].position.x = -99999;

		else if (bulls[i].position.y < 0 || bulls[i].position.y > SCREEN_HEIGHT)
			bulls[i].position.x = -99999; // sentinel value

		else{
			bulls[i].position.x += bulls[i].velocity.x * DELTA;
			bulls[i].position.y += bulls[i].velocity.y * DELTA;
		}

	}
}

void dual_collisions(Player *p1, Player *p2){

	float xdiff, ydiff;
	float distance2;
	
	// distance between the ships

	xdiff =  p1->sh.position.x - p2->sh.position.x;
	ydiff =  p1->sh.position.y - p2->sh.position.y;
	distance2 = xdiff * xdiff + ydiff * ydiff;
	
	if (distance2 < 550){
		if (p1->sh.shield == 1 && p2->sh.shield == 1){	// bounce players
			collide2(&p1->sh.velocity, &p1->sh.position, 1, &p2->sh.velocity, &p2->sh.position, 1);
			play_sound(BOUNCE, 0);
			}
		if (p2->sh.shield == 0){	// p2 dies
			play_sound(IMPACT, 0);				
			--p2->lives;
			set_parts(p2->sh.position.x, p2->sh.position.y);
			init_player(p2, 1, 2);
			//init_player(p1, 0, 2);
		}
		if (p1->sh.shield == 0){	// p1 dies
			play_sound(IMPACT, 0);				
			--p1->lives;
			set_parts(p1->sh.position.x, p1->sh.position.y);
			init_player(p1, 0, 2);
			//init_player(p2, 1, 2);
		}
	}

	// player bullet collisions
	

	// compare p1 bullets to p2
	for (int j = 0; j < MAX_BULLETS; ++j){
		if (p1->bulls[j].position.x != -99999){
			xdiff = p1->bulls[j].position.x - p2->sh.position.x;
			ydiff = p1->bulls[j].position.y - p2->sh.position.y;
			distance2 = xdiff * xdiff + ydiff * ydiff;

			if (distance2 < 120){
				p1->bulls[j].position.x = -99999;
				if (p2->sh.shield != 1){
					play_sound(HIT, 0);
					set_parts(p2->sh.position.x, p2->sh.position.y);	
					--p2->lives;
					init_player(p2, 1, 2);
				}
				else collide2(&p1->bulls[j].velocity, &p1->bulls[j].position, 1, &p2->sh.velocity, &p2->sh.position, 40);
			}
		}
	}	
	// compare p2 bullets to p1
	for (int j = 0; j < MAX_BULLETS; ++j){
		if (p2->bulls[j].position.x != -99999){
			xdiff = p2->bulls[j].position.x - p1->sh.position.x;
			ydiff = p2->bulls[j].position.y - p1->sh.position.y;
			distance2 = xdiff * xdiff + ydiff * ydiff;

			if (distance2 < 120){
				p2->bulls[j].position.x = -99999;
				if (p1->sh.shield != 1){
					play_sound(HIT, 0);
					set_parts(p1->sh.position.x, p1->sh.position.y);	
					--p1->lives;
					init_player(p1, 0, 2);
				}
				else collide2(&p2->bulls[j].velocity, &p2->bulls[j].position, 1, &p1->sh.velocity, &p1->sh.position, 40);
			}
		}
	}	
}

void collision_detect(asteroid *field, Player *p, enemy *en){
	
	float xdiff, ydiff;
	float distance2 = 0, overlap = 0, xpos = 0, ypos = 0, size = 0;
	
	for (int i = 0; i < no_of_asteroids - 1; ++i){		// each asteroid
		xpos = field[i].position.x;
		ypos = field[i].position.y;
		size = field[i].size * 5;
		for (int j = i + 1; j < no_of_asteroids; ++j){	// compare to each successive asteroid
			xdiff =  xpos - field[j].position.x;
			ydiff =  ypos - field[j].position.y;
			distance2 = xdiff * xdiff + ydiff * ydiff;		// sqrt not needed
			overlap = distance2 - size * size - field[j].size * field[j].size * 25 - 10*size*field[j].size;
			if (overlap <= 0){
				collide2(&field[i].velocity, &field[i].position, field[i].size, &field[j].velocity, &field[j].position, field[j].size);	
				play_sound(CRASH, 0);
			}
		}
	}
			
	for (int i = 0; i < no_of_asteroids; ++i){			// player - asteroid
		// compare ship
		xdiff =  p->sh.position.x - field[i].position.x;
		ydiff =  p->sh.position.y - field[i].position.y;
		distance2 = xdiff * xdiff + ydiff * ydiff;
		overlap = distance2 - field[i].size * field[i].size * 24 - 144 - 2 * field[i].size * 5 * 10;
		if (overlap <= 910){
			if (p->sh.shield == 1){
				collide2(&field[i].velocity, &field[i].position, field[i].size, &p->sh.velocity, &p->sh.position, 1);
				play_sound(BOUNCE, 0);
			}
			else if (overlap <= 0){
				play_sound(IMPACT, 0);
				set_parts(p->sh.position.x, p->sh.position.y);			
				--p->lives;
				init_player(p, 1, 1);
			
			}
		}
		
		for (int j = 0; j < MAX_BULLETS; ++j){			// bullet asteroid
			// compare bullet
			int hit = 0;
			if (p->bulls[j].position.x != -99999){
				xdiff = p->bulls[j].position.x - field[i].position.x;
				ydiff = p->bulls[j].position.y - field[i].position.y;
				distance2 = xdiff * xdiff + ydiff * ydiff;
				overlap = distance2 - field[i].size * field[i].size * 24;
				if (overlap <= 0){
					split_asteroid(&field[0], i);
					if (((float)rand()/(float)RAND_MAX) > 0.8){
					
						create_enemies(en, 1);
					}
					p->score = p->score + field[i].size;
					play_sound(HIT, 0);
					p->bulls[j].position.x = -99999;
					hit = 1;
				}
				
			}
			if (hit == 1)
				break;
		}
	}

	for (int i = 0; i < no_of_powerups; ++i){			// player - powerup

		xdiff =  p->sh.position.x - powerups[i].position.x;
		ydiff =  p->sh.position.y - powerups[i].position.y;
		distance2 = xdiff * xdiff + ydiff * ydiff;

		if (distance2 <= 256){
			collect_powerup(en, p, i);
		}
	}

	if (no_of_enemy == 0)
		return;
	
	for (int i = 0; i < no_of_enemy; ++i){
		for (int j = 0; j < MAX_BULLETS; ++j){			// enemy bullet player
			// compare bullet
			if (en[i].bulls[j].position.x != -99999){	// if bullet is existing in game
				xdiff = en[i].bulls[j].position.x - p->sh.position.x;
				ydiff = en[i].bulls[j].position.y - p->sh.position.y;
				distance2 = xdiff * xdiff + ydiff * ydiff;
				//overlap = distance2 - 100;
				if (distance2 < 120){
					en[i].bulls[j].position.x = -99999;
					if (p->sh.shield != 1){
						play_sound(HIT, 0);
						set_parts(p->sh.position.x, p->sh.position.y);	
						--p->lives;
						init_player(p, 1, 1);
					}
				}
			}
		}
	}

	for (int j = 0; j < MAX_BULLETS; ++j){			// player bullet enemy
		if (p->bulls[j].position.x != -99999){	
			for ( int k = 0; k < no_of_enemy; ++k){
				xdiff = p->bulls[j].position.x - en[k].sh.position.x;
				ydiff = p->bulls[j].position.y - en[k].sh.position.y;
				distance2 = xdiff * xdiff + ydiff * ydiff;
				if (distance2 < 140){
					kill_enemy(en, k);
					p->score += 25;
					p->bulls[j].position.x = -99999;
				}
			}
		}	
	}
	
	for (int i = 0; i < no_of_enemy; ++i){			// player - enemy
		
		xdiff =  p->sh.position.x - en[i].sh.position.x;
		ydiff =  p->sh.position.y - en[i].sh.position.y;
		distance2 = xdiff * xdiff + ydiff * ydiff;
		
		if (distance2 <= 200){
			if (p->sh.shield == 1){
				kill_enemy(en, i);
				p->score += 25;
			}
			else {
				play_sound(IMPACT, 0);
				kill_enemy(en, i);			
				--p->lives;
				init_player(p, 1, 1);
			
			}
		}
	}
} 	// end collision_detect

// Take 2 position Vectors and two velocity vectors
void collide2(Vector *v1, Vector *p1, int s1, Vector *v2, Vector *p2, int s2){

	Vector colnormal;
	Vector colplane;
	Vector diff;
	
	float mag, overlap = 0;

	while (overlap < 1){
		diff.x = p2->x - p1->x;
		diff.y = p2->y - p1->y;
		mag = diff.x*diff.x + diff.y*diff.y;		// sqrt not needed
		overlap = mag - s1 * s1 * 25 - s2 * s2 * 25 - 10 * s1 * s2;

		if (p1->x < p2->x){
			--p1->x;
			++p2->x;
		}
		else{
			++p1->x;
			--p2->x;
		}
		if (p1->y < p2->y){
			--p1->y;
			++p2->y;
		}
		else{
			++p1->y;
			--p2->y;
		}
	}	
	
	diff.x = p2->x - p1->x;
	diff.y = p2->y - p1->y;
	
	mag = sqrt(diff.x*diff.x + diff.y*diff.y);
	// get normalised collision plane between objects
	colnormal.x = diff.x/mag;
	colnormal.y = diff.y/mag;
	// normal vector = swapping the two components of the vector and multiplying the first by -1.
	colplane.x = -1.0 * colnormal.y;
	colplane.y = colnormal.x;
	
	float n_veli = v1->x * colnormal.x + v1->y * colnormal.y;
	float c_veli = v1->x * colplane.x + v1->y * colplane.y;

	float n_velj = v2->x * colnormal.x + v2->y * colnormal.y;
	float c_velj = v2->x * colplane.x + v2->y * colplane.y;

	// Calculate the scaler velocities of each object after the collision.

	float new1 = (n_veli * (s1 - s2) + 2 * s2 * n_velj) / (s1 + s2);
	float new2 = (n_velj * (s2 - s1) + 2 * s1 * n_veli) / (s1 + s2);
	
	// Convert the scalers to vectors by multiplying by the normalised plane vectors.
	v1->x = colnormal.x * new1 + colplane.x * c_veli;
	v1->y = colnormal.y * new1 + colplane.y * c_veli;
	
	v2->x = colnormal.x * new2 + colplane.x * c_velj;
	v2->y = colnormal.y * new2 + colplane.y * c_velj;

}

void split_asteroid(asteroid *field, int a){

	set_parts(field[a].position.x, field[a].position.y);
	if (((float)rand()/(float)RAND_MAX) > 0.8)
		init_powerup(field[a].position.x, field[a].position.y);

	float a_speed, a_direction, dx, dy;
	float b_speed, b_direction;
	
	if (field[a].size == 3){
		//for (int i = no_of_asteroids; i > a; --i){ 
	//	field[a] = field[no_of_asteroids-1]; // delete the asteroid
	// use memmove when src and dest can overlap!!!!!
		memmove(&field[a], &field[no_of_asteroids-1], sizeof(asteroid));
		no_of_asteroids -= 1;
		return;
	}

	int n = no_of_asteroids++;
	float oldx = field[a].position.x;
	float oldy = field[a].position.y;

	a_direction = atan2(field[a].velocity.y, field[a].velocity.x) + PI/4;
	a_speed = sqrt(field[a].velocity.x * field[a].velocity.x + field[a].velocity.y * field[a].velocity.y);
	b_direction = atan2(field[a].velocity.y, field[a].velocity.x) - PI/4;
	b_speed = sqrt(field[a].velocity.x * field[a].velocity.x + field[a].velocity.y * field[a].velocity.y);

	// first new asteroid
	field[a].size -= 3;
	
	field[a].velocity.x = a_speed * cos(a_direction);
	field[a].velocity.y = a_speed * sin(a_direction);
		
	dx = field[a].velocity.x / a_speed;
	dy = field[a].velocity.y / a_speed;
	
	field[a].position.x = oldx + dx * field[a].size * 7 + 2 * dx;
	field[a].position.y = oldy + dy * field[a].size * 7 + 2 * dy;

	// second new asteroid
	field[n].size = field[a].size;

	field[n].velocity.x = b_speed * cos(b_direction);
	field[n].velocity.y = b_speed * sin(b_direction);
		
	dx = field[n].velocity.x / b_speed;
	dy = field[n].velocity.y / b_speed;
	
	field[n].position.x = oldx + dx * field[n].size * 7 + 2 * dx;
	field[n].position.y = oldy + dy * field[n].size * 7 + 2 * dy;
	
}

Vector min_seperate(Vector p1, int s1, Vector p2, int s2){

	Vector res;
	float angle, m;
	 
	res.x = p2.x - p1.x;
	res.y = p2.y - p1.y;
	angle = atan2(res.y, res.x);
	
	m = sqrt(res.x*res.x + res.y*res.y);
	m = fabs(m - s1 - s2);

	res.x = m * cos(angle);
	res.y = m * sin(angle);

	return (res);
}
	
void play_sound(int index, int loop){

//	if (!Mix_Playing(index))

	if(index == SHIELD)
		Mix_PlayChannel(0, sound[index].effect, loop);
	else if (index != SHIELD)
		Mix_PlayChannel(-1, sound[index].effect, loop);

	//printf("Mix_PlayChannel: %s\n",Mix_GetError());
}

int pause_game(){

	texture t;
	SDL_Event e;
	int q = 0;

	if (render == 1){

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		Mix_PauseMusic();
		Mix_HaltChannel(-1);
		render_text(&t, "PAUSED", 1, 88);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, t.t);
		glColor4f( 1.0, 0.55, 0.1, 1.0 );

		GLfloat tex[] = {0,0, 1,0, 1,1, 0,1};
		GLfloat box[] = {SCREEN_WIDTH/2 - t.w/2, SCREEN_HEIGHT/2 - t.h,
						 SCREEN_WIDTH/2 + t.w/2, SCREEN_HEIGHT/2 - t.h,
						 SCREEN_WIDTH/2 + t.w/2, SCREEN_HEIGHT/2,
						 SCREEN_WIDTH/2 - t.w/2, SCREEN_HEIGHT/2};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0,box);
		glTexCoordPointer(2, GL_FLOAT, 0, tex);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);
		SDL_GL_SwapWindow(mywindow);

	}
		
	while (!q){
		//if (SDL_PollEvent(&e)){ 		/* execution suspends here while waiting on an event */
		while (SDL_PollEvent(&e) != 0){
			switch (e.type){

				case SDL_QUIT:
					q = 1;
					return 99;
					break;
				case SDL_APP_WILLENTERBACKGROUND:		// unload stars ?
					render = 0;
					Mix_PauseMusic();
					Mix_HaltChannel(-1);
					break;
				case SDL_APP_DIDENTERBACKGROUND:		// unload stars ?
					render = 0;
					Mix_PauseMusic();
					Mix_HaltChannel(-1);
					break;
				case SDL_APP_WILLENTERFOREGROUND:

					break;
				case SDL_APP_DIDENTERFOREGROUND:

					Mix_ResumeMusic();
					render = 1;
					q = 1;
					break;

				case SDL_FINGERDOWN:
					render = 1;
					q = 1;
					break;
				default:
					break;
			}

		}

		SDL_Delay(500);

	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &t.t);
	Mix_ResumeMusic();
}
/*
int EventFilter(void *, SDL_Event * event)
{
   switch (event->type)
   {
      case SDL_APP_WILLENTERBACKGROUND:

         paused = true;
         break;

      case SDL_APP_DIDENTERFOREGROUND:

         paused = false;
         break;
   }
   return 1;
}
*/
// returns 0 = normal; 1 = quit; 2 = game was paused.
int keyboard_events(Player *p1, Player *p2, int mode){

	SDL_Event e;
	static Uint32 timer = 0;
	static Uint32 accum = 0;
	static Uint32 old_timer = 0;
	static float motion = 0;
	static float x = 0;
	static float y = 0;
	static float sensitivity = 0.005;
	//SDL_TouchID tid = SDL_GetTouchDevice(index);  //

	while (SDL_PollEvent(&e) != 0){	

		if (p1->lives > 0){
			if (e.type == SDL_APP_WILLENTERBACKGROUND){		// unload stars ?
				Mix_PauseMusic();
				Mix_HaltChannel(-1);
				render = 0;
				return 2;
			}

			if (e.type == SDL_APP_DIDENTERFOREGROUND){

				Mix_ResumeMusic();
				render = 1;

			}
			if (e.type == SDL_QUIT)
				return 99;

			if (e.type == SDL_FINGERDOWN){
				// screen touched, start timer
				motion = 0;
				timer = SDL_GetTicks();
				old_timer = timer;
			}
			if (e.type == SDL_FINGERUP){
				// switch of shield
				p1->sh.shield = 0;
				Mix_HaltChannel(0);
				// if short touch, fire bullet
				if (timer - old_timer <= 150 && !p1->sh.shield){
					//create_bullet_rel(&p1->bulls[0], &p1->sh);
					p1->sh.fire(&p1->bulls[0], &p1->sh);
				}
				// reset static variables
				accum = 0;
				x = 0;
				y = 0;
				motion = 0;
				old_timer = 0;
			}

			if (old_timer != 0){
				timer = SDL_GetTicks();
				if (timer - old_timer > 150 && !p1->sh.shield && p1->sh.shield_power > 0 && motion < sensitivity){
					p1->sh.shield = 1;
					p1->sh.shield_timer = SDL_GetTicks();
					play_sound(SHIELD, -1);
				}
			}
//			if (SDL_GetNumTouchFingers(tid) > 1 && !p1->sh.shield && p1->sh.shield_timer > 0){
//				p1->sh.shield = 1;
//				p1->sh.shield_timer = SDL_GetTicks();
//				play_sound(SHIELD, -1);
//			}


			if (e.type == SDL_FINGERMOTION){
				// calculate the distance moved squared
				motion += (e.tfinger.dx*e.tfinger.dx) + (e.tfinger.dy*e.tfinger.dy);

				if (motion > sensitivity){
					p1->sh.angle = atan2(e.tfinger.dy, e.tfinger.dx);
					play_sound(THRUST, 0);
					init_thruster(&p1->sh);
					p1->sh.velocity.x += 9 * e.tfinger.dx;
					p1->sh.velocity.y += 9 * e.tfinger.dy;
					//p1->sh.velocity.x += 0.25 * cos(p1->sh.angle) * DELTA;
					//p1->sh.velocity.y += 0.25 * sin(p1->sh.angle) * DELTA;
					motion = 0;
				}



			}	// end mouse motion
		}	// end lives > 1
	
	
		// Repeating events player 1
		// currentKeyStates points to an array of values, 1=pressed, 0=unpressed

	

		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
		
//		if (currentKeyStates[SDL_SCANCODE_AC_HOME]){
//			return 1;
//		}
		if (currentKeyStates[SDL_SCANCODE_AC_BACK]){
			return 2;
		}
	}
	return 0;
}

void player_wins(int n){

	texture t;
	SDL_Event e;
	int q = 0;
	char winner[15];

	play_sound(HYPER, 0);
	sprintf(winner, "PLAYER %d WINS", n);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	render_text(&t, winner, 1, 88);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
	glBindTexture(GL_TEXTURE_2D, t.t);
	glColor4f( 0.1, 0.55, 1, 1.0 );

	GLfloat tex[] = {0,0, 1,0, 1,1, 0,1};
	GLfloat box[] = {SCREEN_WIDTH/2 - t.w/2, SCREEN_HEIGHT/2 - t.h,
					 SCREEN_WIDTH/2 + t.w/2, SCREEN_HEIGHT/2 - t.h,
					 SCREEN_WIDTH/2 + t.w/2, SCREEN_HEIGHT/2,
					 SCREEN_WIDTH/2 - t.w/2, SCREEN_HEIGHT/2};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
	glVertexPointer(2, GL_FLOAT, 0,box);
	glTexCoordPointer(2, GL_FLOAT, 0, tex);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);
	
	while (!q){
	SDL_PollEvent(&e);
	
		if (e.type == SDL_KEYDOWN)
			if (e.key.keysym.sym == layout[P1FIRE] || e.key.keysym.sym == layout[P2FIRE])
				q = 1;

		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &t.t);
}

void collect_powerup(enemy *en, Player *p, int id){

	// 0 = bomb, 1 = dual bullets, 2 = kill enemy, 3 = life
	int type = powerups[id].type;
	play_sound(POWER, 0);

	if (type == 0){
		// somehow explode all in certain radius
	}
	else if (type == 1){
		p->sh.gun = 1;
	}
	else if (type == 2){
		for (int i = 0; i < no_of_enemy; ++i){
			set_parts(en[i].sh.position.x, en[i].sh.position.y);
		}

		play_sound(HIT, 0);
		no_of_enemy = 0;
	}
	else if (type == 3){
		p->lives += 1;
	}

	memmove(&powerups[id], &powerups[no_of_powerups - 1], sizeof(powerup));
	no_of_powerups -= 1;
}
