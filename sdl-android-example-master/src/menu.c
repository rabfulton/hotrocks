
#include "main.h"

extern SDL_Window* mywindow;				// the usual window
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern int MAX_PARTICLES;
extern int NO_OF_EXPLOSIONS;

void set_keys(GLuint tex);
void load_keys(int);		// non zero to load defaults
void save_keys();
void load_defaults();
void set_detail(GLuint tex);
void set_mouse(GLuint tex);

int main_menu(GLuint tex){

	char *s1[] = {"Start Game", "Config", "High Scores", "Quit"};
	char *sc[] = {"Game Mode", "Controls", "Video", "Back"};
	char *sv[] = {"Fullscreen", "Aspect Ratio", "Detail", "Back"};
	char *sm[] = {"1 Player", "2 Players", "Death Duel", "Back"};
	char *sp[] = {"Controls", "mouse", "Defaults", "Back"};
	char **s;
	static int mode = 0;
	s = s1;
	int colour = 0;
	int selected = 0;
	int old_selected = 0;
	int original_selected = 0;
	int quit = 0;
	SDL_Event e;
	int menu_mode = 0;
	Uint32 timer = 0;
	Uint32 old_timer = 0;
	float motion = 0;

	enum{
		MENU_MAIN,
		MENU_CONFIG,
		MENU_GAME,
		MENU_VIDEO,
		MENU_CONTROLS,
	};
	
	//load_keys(0);
	
	while (!quit){

		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);

		draw_menu_item(SCREEN_WIDTH/2, SCREEN_HEIGHT/7, "HOT ROCKS", 2, 88);

		for (int i = 0; i < 4; ++i){
			if (i == selected)
				colour = 1;
			else
				colour = 2;
				
			draw_menu_item(SCREEN_WIDTH/2 , SCREEN_HEIGHT/6 * (i+2), s[i], colour, 48);
		}

		while (SDL_PollEvent(&e) != 0){
	
			switch (e.type){

				case SDL_QUIT:
					quit = 1;
					break;
				case SDL_APP_WILLENTERBACKGROUND:		// unload stars ?
					Mix_PauseMusic();Mix_HaltChannel(-1);
					break;
				case SDL_APP_WILLENTERFOREGROUND:
					Mix_ResumeMusic();
					break;
				case SDL_FINGERDOWN:
					original_selected = selected;
					/* no break */
				case SDL_FINGERMOTION:

					for(int i = 0; i < 4; ++i){
						if (e.tfinger.y < (1.0/6.0) * (i+2) + 1.0/13.0 &&
							e.tfinger.y > (1.0/6.0) * (i+2) - 1.0/13.0){

							selected = i;
							if (old_selected != selected)
								play_sound(SELECTS, 0);
							old_selected = selected;
						}
					}

					break;
				case SDL_FINGERUP:
					if (e.tfinger.y < (1.0/6) * (selected+2) + 1.0/13 &&
						e.tfinger.y > (1.0/6) * (selected+2) - 1.0/13){

						// MAIN MENU LOGIC---------------------------------------------<
						if (selected == original_selected && menu_mode == MENU_MAIN){
							play_sound(FIRE, 0);
							if (selected == 0)
								return mode;
							else if (selected == 1){
								s = s1;
								menu_mode = MENU_MAIN;
							}
							else if (selected == 2){		// goto highscores
								high_scores(0, 0, tex);
								menu_mode = MENU_MAIN;
								s = s1;
							}
							else if (selected == 3){
								SDL_Delay(700);				// Delay to let sample finish
								quit = 1;
							}
							selected = 0;					// start at top of new menu
						}
					}// END MAIN MENU LOGIC---------------------------------------------<

					// CONFIG MENU LOGIC---------------------------------------------<
					if (selected == original_selected && menu_mode == MENU_CONFIG){
						if (selected == 0){
							s = sm;
							menu_mode = MENU_GAME;
						}
						else if (selected == 1){
							s = sp;
							menu_mode = MENU_CONTROLS;
						}
						else if (selected == 2){
							s = sv;
							menu_mode = MENU_VIDEO;
						}
						else if (selected == 3){
							menu_mode = MENU_MAIN;
							s = s1;
						}
						selected = 0;
					}// END CONFIG MENU LOGIC---------------------------------------------<

					// VIDEO MENU LOGIC---------------------------------------------<
					if (selected == original_selected && menu_mode == MENU_VIDEO){
						if (selected == 0)
							;											// Toggle fullscreen
						else if (selected == 1)							// select aspect ratio
							;
						else if (selected == 2){						// set detail defailts
							set_detail(tex);
						}
						else if (selected == 3){
							menu_mode = MENU_CONFIG;
							s = sc;
						}
						selected = 0;
					}// END VIDEO MENU LOGIC---------------------------------------------<

					// GAME TYPE MENU LOGIC---------------------------------------------<
					if (selected == original_selected && menu_mode == MENU_GAME){
						if (selected == 0){
							mode = 0;					//set 1player game
							//menu_mode = MENU_MAIN;
							s = sm;
						}
						else if (selected == 1){
							mode = 1;					//set two players
							//menu_mode = MENU_MAIN;
							s = sm;
						}
						else if (selected == 2){
							mode = 2;					//set death duel mode
							//menu_mode = MENU_MAIN;
							s = sm;
						}
						else if (selected == 3){
							menu_mode = MENU_CONFIG;
							s = sc;
						}
						selected = 0;
					}// END GAME TYPE MENU LOGIC---------------------------------------------<

					// CONTROLS MENU LOGIC---------------------------------------------<
					if (selected == original_selected && menu_mode == MENU_CONTROLS){
						if (selected == 0){
							set_keys(tex);				// Players controls
						}
						else if (selected == 1){
							set_mouse(tex);				// Mouse enable
						}

						else if (selected == 2){
							load_defaults();			// Load defaults
							menu_mode = MENU_MAIN;
							s = s1;
						}
						else if (selected == 3){
							menu_mode = MENU_CONFIG;
							s = sc;
						}
						selected = 0;
					}// END CONTROLS MENU LOGIC---------------------------------------------<
					if (selected == original_selected)
						play_sound(FIRE, 0);
					break; // END CASE FINGERUP

				default:
					break;

			}


		}

		selected = (selected+8)%4;
		SDL_GL_SwapWindow( mywindow );
		SDL_Delay(50);
	}
	
	if (quit == 1){
		close_sdl();
		return 99;
	}
	return (mode);
}

void draw_menu_item(int x, int y, char text[], int colour, int size){

	texture t;

	if (text[0] == '\0')		// check for zero length string
		return;
	
	render_text(&t, text, colour, size);
	//glTexParameterf(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE);
	//glTexParameterf(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE);
	GLfloat vertex[8] = {	x-t.w/2, y-t.h/2,
							x+t.w/2, y-t.h/2,
							x+t.w/2, y+t.h/2,
							x-t.w/2, y+t.h/2 };
							

	GLfloat texver[8] = {0, 0, 1, 0, 1, 1, 0, 1};
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glMatrixMode(GL_MODELVIEW);	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, t.t);

	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glTexCoordPointer(2, GL_FLOAT, 0, texver);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDeleteTextures(1, &t.t);
	glDisable(GL_TEXTURE_2D);
	
}

void set_keys(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	int readkey = 0;
	SDL_Event e;
	char * label[] = {
	    "P1 Rotate Left",
	    "P1 Rotate Right",
	    "P1 Thrust",
	    "P1 Fire",
	    "P1 Shield",
	    "P2 Rotate Left",
	    "P2 Rotate Right",
	    "P2 Thrust",
	    "P2 Fire",
	    "P2 Shield"
	};
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(SCREEN_WIDTH/2, 150, "HOTKEYS", 2, 88);
			
		while(readkey && SDL_WaitEvent(&e)){
			if(e.type == SDL_KEYDOWN){
				play_sound(FIRE, 0);
				layout[selected] = e.key.keysym.sym;
				readkey = 0;
				break;
			}
		}
                

                
		while (SDL_PollEvent(&e) != 0){		// check for key event
	
			if(e.type == SDL_KEYDOWN){
				//play_sound(FIRE, 0);
				switch(e.key.keysym.sym){
				case SDLK_RETURN: case SDLK_SPACE:
					layout[selected] = 46;
					readkey = 1;
					break;
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			break;			
			}	
		}

		for (int i = 0; i < 10; ++i){
			if (i == selected)
				colour = 1;
			else
				colour = 2;

			draw_menu_item(SCREEN_WIDTH/3 , 300 + i * 40, label[i], 2, 24);
			draw_menu_item(SCREEN_WIDTH - SCREEN_WIDTH/4, 300 + i * 40, (char*)SDL_GetKeyName(layout[i]), colour, 24); 
		}
		
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
		
	}

}

void load_keys(int a){

	int i, check = 0;
	FILE * config;							// pointer to config.dat

	if(a == 0)
		config = fopen("config.dat", "r");		// open for reading, use rb for non text
	else
		config = fopen("default.dat", "r");
	if (config == NULL){					// check file opens
		printf("Unable to open config.dat\n");
		return;
	}
	
	// try to read keyboard layout
	
	for(i = 0; i < 11; ++i){
		check = fscanf(config, "%*s %d", &layout[i]);	// read/ignore a string, eat whitespace, store int

		if (check != 1){
			printf("error reading keybinds from config.dat\n");			
			break;
		}
	}

	// try to read misc settings

	check = fscanf(config, "%*s %d", &MAX_PARTICLES);
	if (check != 1){
		printf("error reading max_particles from config.dat\n");
		MAX_PARTICLES = 480;
	}
	check = fscanf(config, "%*s %d", &NO_OF_EXPLOSIONS);
	if (check != 1){
		printf("error reading NO_OF_EXPLOSIONS from config.dat\n");
		NO_OF_EXPLOSIONS = 8;
	}
	
	fclose (config);
	save_keys();
}

void save_keys(){

	int i, check = 0;
	FILE * config;		/* pointer to highscore.dat */

	
	
	config = fopen("config.dat", "w");		/* open for writing, use wb for binary format */
	
	for(i = 0; i < 10; ++i){
		
		check = fprintf(config, "keybind= %d\n", layout[i]);

		if (check == 0){
			printf("error writing config.dat\n");			
			break;
		}
	}

	fprintf(config, "MOUSE= %d\n", layout[MOUSE_ENABLE]);
	fprintf(config, "MAX_PARTICLES= %d\n", MAX_PARTICLES);
	fprintf(config, "NO_OF_EXPLOSIONS= %d\n", NO_OF_EXPLOSIONS);
	
	fclose (config);		/* remember to close the file */
	
}

void load_defaults(){
	load_keys(1);
	save_keys();
}

void set_detail(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	SDL_Event e;
	char * label[] = {
	    "LOW",
	    "MEDIUM",
	    "HIGH"
	};
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(SCREEN_WIDTH/2, 150, "DETAILS", 2, 88);

                
		while (SDL_PollEvent(&e) != 0){		// check for key event
	
			if(e.type == SDL_KEYDOWN){
				
				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						NO_OF_EXPLOSIONS = 3;		// Must be divisor of MAX_PARTICLES
						MAX_PARTICLES = 210;		// Must be multiple of NO_OF_EXPLOSIONS
						quit = 1;
					}
					else if (selected == 1){
						NO_OF_EXPLOSIONS = 8;		// Must be divisor of MAX_PARTICLES
						MAX_PARTICLES = 480;		// Must be multiple of NO_OF_EXPLOSIONS
						quit = 1;
					}
					else if (selected == 2){
						NO_OF_EXPLOSIONS = 10;		// Must be divisor of MAX_PARTICLES
						MAX_PARTICLES = 1000;		// Must be multiple of NO_OF_EXPLOSIONS
						quit = 1;
					}
					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			break;			
			}	
		}

		for (int i = 0; i < 3; ++i){
			
			if (i == selected)
				colour = 1;
			else
				colour = 2;
			
			draw_menu_item(SCREEN_WIDTH/2, 300 + 100 * i, label[i], colour, 48); 
		}
		
		selected = (selected+3)%3;
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
		
	}

	save_keys();
}

void set_mouse(GLuint tex){

	int colour = 0;
	int selected = 0;
	int quit = 0;
	SDL_Event e;
	char* label[2] = {
	    "DISABLED",
	    "ENABLED",
	};
	
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		draw_background(tex);
		glColor4f(1,1,1,1);
		draw_menu_item(SCREEN_WIDTH/2, 150, "MR MOUSE", 2, 88);

                
		while (SDL_PollEvent(&e) != 0){		// check for key event
	
			if(e.type == SDL_KEYDOWN){
				
				switch(e.key.keysym.sym){
					
				case SDLK_RETURN: case SDLK_SPACE:
					play_sound(FIRE, 0);
					if (selected == 0){
						layout[MOUSE_ENABLE] = 0;
						quit = 1;
					}
					else if (selected == 1){
						layout[MOUSE_ENABLE] = 1;
						quit = 1;
					}
					
					break;
					
				case SDLK_DOWN:
					++selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_UP:
					--selected;
					play_sound(SELECTS, 0);
					break;
					
				case SDLK_ESCAPE:
					play_sound(FIRE, 0);
					save_keys();
					return;
				}
			break;			
			}	
		}

		for (int i = 0; i < 2; ++i){
			
			if (i == selected)
				colour = 1;
			else
				colour = 2;
			
			draw_menu_item(SCREEN_WIDTH/2, 300 + 100 * i, label[i], colour, 48); 
		}
		
		selected = (selected+2)%2;
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
		
	}

	save_keys();
}
