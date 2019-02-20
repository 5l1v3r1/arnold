/* 
 *  Arnold emulator (c) Copyright, Kevin Thacker 1995-2001
 *  
 *  This file is part of the Arnold emulator source code distribution.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// table to map KeySym values to CPC Key values
int	KeySymToCPCKey[SDLK_LAST];

// Joystick handles
SDL_Joystick *joystick1, *joystick2;

// This is the area around the center of an analog Joystick where all movement
// is assumed to be jitter
#define JOYDEAD 3200

extern void quit(void);		// FIXME

// State is True for Key Pressed, False for Key Release.
// theEvent holds the keyboard event.
void	HandleKey(SDL_KeyboardEvent *theEvent)
{
	//int State = ( theEvent->type == SDL_KEYDOWN ) ? TRUE : FALSE;
	int key_upper_code;
	//int keycode;
	CPC_KEY_ID	theKeyPressed;

	// get KeySym
	SDL_keysym	*keysym = &theEvent->keysym;
	SDLKey	keycode = keysym->sym;

	/* Handle Function keys to control emulator */
	if (keycode == SDLK_F1 && theEvent->type == SDL_KEYDOWN ) {
		CPC_Reset();
	} else if (keycode == SDLK_F2 && theEvent->type == SDL_KEYDOWN ) {	
		//DisplayMode ^=0x0ff;
		fullscreen = !fullscreen;
		if (fullscreen) {
			sdl_SetDisplayFullscreen(screen->w,screen->h,
				screen->format->BitsPerPixel);
		} else {
			sdl_SetDisplayWindowed(screen->w,screen->h,
				screen->format->BitsPerPixel);
		}
	} else if (keycode == SDLK_F4 && theEvent->type == SDL_KEYDOWN ) {
		quit();
	/* Handle CPC keys */
	} else {
		//printf("Keycode: <%04x> <%04x> <%04x> <%04x>\n",
		//	keysym->scancode, keysym->sym, keysym->mod, keysym->unicode );

		if ( keycode <= SDLK_LAST ) {
			theKeyPressed = KeySymToCPCKey[keycode];
			if (theKeyPressed == CPC_KEY_NULL)
				printf("Null key: <%04x>\n", keysym->sym);
		} else {
			theKeyPressed = CPC_KEY_NULL;
			printf("Unknown key: <%04x>\n", keysym->sym);
		}

		// set or release key depending on state	
		if ( theEvent->type == SDL_KEYDOWN ) {	
			// set key
			CPC_SetKey(theKeyPressed);
		} else {
			// release key
			CPC_ClearKey(theKeyPressed);
		}
	}
}

void	HandleJoy(SDL_JoyAxisEvent *event) {
	if( event->axis == 0) {
		/* Left-right movement */
		if ( ( event->value < -JOYDEAD ) ) {
			CPC_SetKey(CPC_KEY_JOY_LEFT);
			CPC_ClearKey(CPC_KEY_JOY_RIGHT);
		} else if ( ( event->value > JOYDEAD ) ) {
			CPC_ClearKey(CPC_KEY_JOY_LEFT);
			CPC_SetKey(CPC_KEY_JOY_RIGHT);
		} else {
			CPC_ClearKey(CPC_KEY_JOY_LEFT);
			CPC_ClearKey(CPC_KEY_JOY_RIGHT);
		}
	}
	if( event->axis == 1) {
		/* Up-Down movement */
		if ( ( event->value < -JOYDEAD ) ) {
			CPC_SetKey(CPC_KEY_JOY_UP);
			CPC_ClearKey(CPC_KEY_JOY_DOWN);
		} else if ( ( event->value > JOYDEAD ) ) {
			CPC_ClearKey(CPC_KEY_JOY_UP);
			CPC_SetKey(CPC_KEY_JOY_DOWN);
		} else {
			CPC_ClearKey(CPC_KEY_JOY_UP);
			CPC_ClearKey(CPC_KEY_JOY_DOWN);
		}
	}
}

BOOL sdl_ProcessSystemEvents()
{
	SDL_Event	event;
	while(SDL_PollEvent(&event)) {
		switch (event.type)
		{
			case SDL_QUIT:
				printf("Oh! SDL_Quit\n");
				return TRUE;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				HandleKey((SDL_KeyboardEvent *) &event);
			}
			break;

			case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
				HandleJoy((SDL_JoyAxisEvent *) &event);
			break;

			case SDL_JOYBUTTONDOWN:  /* Handle Joystick Buttons */
				if ( event.jbutton.button == 0 ) {
					CPC_SetKey(CPC_KEY_JOY_FIRE1);
				} else if ( event.jbutton.button == 2 ) {
					CPC_SetKey(CPC_KEY_JOY_FIRE2);
				}
				break;

			case SDL_JOYBUTTONUP:  /* Handle Joystick Buttons */
				if ( event.jbutton.button == 0 ) {
					CPC_ClearKey(CPC_KEY_JOY_FIRE1);
				} else if ( event.jbutton.button == 2 ) {
					CPC_ClearKey(CPC_KEY_JOY_FIRE2);
				}
				break;


			default:
				break;
		}
	}

	return FALSE;
}

void	sdl_InitialiseJoysticks()
{
	int numJoys = 0;

	numJoys = SDL_NumJoysticks();
	fprintf(stderr, "Found %i Joysticks\n", numJoys);
	if (numJoys > 0) {
		SDL_JoystickEventState(SDL_ENABLE);
		joystick1 = SDL_JoystickOpen(0);
		if (numJoys > 1) {
			joystick2 = SDL_JoystickOpen(1);
		}
	}
}

void	sdl_EnableJoysticks(BOOL state)
{
	SDL_JoystickEventState((state == TRUE) ? SDL_ENABLE : SDL_DISABLE);
}

// forward declarations
void	sdl_InitialiseKeyboardMapping_qwertz();
void	sdl_InitialiseKeyboardMapping_azerty();

void	sdl_InitialiseKeyboardMapping(int layout)
{
	int	 i;

	printf("sdl_InitialiseKeyboardMapping(%i)\n",layout);
	//printf("SDLK_LAST: %i 0x%04x\n", SDLK_LAST, SDLK_LAST);
	for (i=0; i<SDLK_LAST; i++)
	{
		KeySymToCPCKey[i] = CPC_KEY_NULL;
	}
	
	/* International key mappings */
	KeySymToCPCKey[SDLK_0] = CPC_KEY_ZERO;
	KeySymToCPCKey[SDLK_1] = CPC_KEY_1;
	KeySymToCPCKey[SDLK_2] = CPC_KEY_2;
	KeySymToCPCKey[SDLK_3] = CPC_KEY_3;
	KeySymToCPCKey[SDLK_4] = CPC_KEY_4;
	KeySymToCPCKey[SDLK_5] = CPC_KEY_5;
	KeySymToCPCKey[SDLK_6] = CPC_KEY_6;
	KeySymToCPCKey[SDLK_7] = CPC_KEY_7;
	KeySymToCPCKey[SDLK_8] = CPC_KEY_8;
	KeySymToCPCKey[SDLK_9] = CPC_KEY_9;
	KeySymToCPCKey[SDLK_a] = CPC_KEY_A;
	KeySymToCPCKey[SDLK_b] = CPC_KEY_B;
	KeySymToCPCKey[SDLK_c] = CPC_KEY_C;
	KeySymToCPCKey[SDLK_d] = CPC_KEY_D;
	KeySymToCPCKey[SDLK_e] = CPC_KEY_E;
	KeySymToCPCKey[SDLK_f] = CPC_KEY_F;
	KeySymToCPCKey[SDLK_g] = CPC_KEY_G;
	KeySymToCPCKey[SDLK_h] = CPC_KEY_H;
	KeySymToCPCKey[SDLK_i] = CPC_KEY_I;
	KeySymToCPCKey[SDLK_j] = CPC_KEY_J;
	KeySymToCPCKey[SDLK_k] = CPC_KEY_K;
	KeySymToCPCKey[SDLK_l] = CPC_KEY_L;
	KeySymToCPCKey[SDLK_m] = CPC_KEY_M;
	KeySymToCPCKey[SDLK_n] = CPC_KEY_N;
	KeySymToCPCKey[SDLK_o] = CPC_KEY_O;
	KeySymToCPCKey[SDLK_p] = CPC_KEY_P;
	KeySymToCPCKey[SDLK_q] = CPC_KEY_Q;
	KeySymToCPCKey[SDLK_r] = CPC_KEY_R;
	KeySymToCPCKey[SDLK_s] = CPC_KEY_S;
	KeySymToCPCKey[SDLK_t] = CPC_KEY_T;
	KeySymToCPCKey[SDLK_u] = CPC_KEY_U;
	KeySymToCPCKey[SDLK_v] = CPC_KEY_V;
	KeySymToCPCKey[SDLK_w] = CPC_KEY_W;
	KeySymToCPCKey[SDLK_x] = CPC_KEY_X;
	KeySymToCPCKey[SDLK_y] = CPC_KEY_Y;
	KeySymToCPCKey[SDLK_z] = CPC_KEY_Z;
	KeySymToCPCKey[SDLK_SPACE] = CPC_KEY_SPACE;
	KeySymToCPCKey[SDLK_COMMA] = CPC_KEY_COMMA;
	KeySymToCPCKey[SDLK_PERIOD] = CPC_KEY_DOT;
	KeySymToCPCKey[SDLK_SEMICOLON] = CPC_KEY_COLON;
	KeySymToCPCKey[SDLK_MINUS] = CPC_KEY_MINUS;
	KeySymToCPCKey[SDLK_EQUALS] = CPC_KEY_HAT;
	KeySymToCPCKey[SDLK_LEFTBRACKET] = CPC_KEY_AT;
	KeySymToCPCKey[SDLK_RIGHTBRACKET] =CPC_KEY_OPEN_SQUARE_BRACKET;

	KeySymToCPCKey[SDLK_TAB] = CPC_KEY_TAB;
	KeySymToCPCKey[SDLK_RETURN] = CPC_KEY_RETURN;
	KeySymToCPCKey[SDLK_BACKSPACE] = CPC_KEY_DEL;
	KeySymToCPCKey[SDLK_ESCAPE] = CPC_KEY_ESC;

	//KeySymToCPCKey[SDLK_Equals & 0x0ff)] = CPC_KEY_CLR;

	KeySymToCPCKey[SDLK_UP] = CPC_KEY_CURSOR_UP;
	KeySymToCPCKey[SDLK_DOWN] = CPC_KEY_CURSOR_DOWN;
	KeySymToCPCKey[SDLK_LEFT] = CPC_KEY_CURSOR_LEFT;
	KeySymToCPCKey[SDLK_RIGHT] = CPC_KEY_CURSOR_RIGHT;
	
	KeySymToCPCKey[SDLK_KP0] = CPC_KEY_F0;
	KeySymToCPCKey[SDLK_KP1] = CPC_KEY_F1;
	KeySymToCPCKey[SDLK_KP2] = CPC_KEY_F2;
	KeySymToCPCKey[SDLK_KP3] = CPC_KEY_F3;
	KeySymToCPCKey[SDLK_KP4] = CPC_KEY_F4;
	KeySymToCPCKey[SDLK_KP5] = CPC_KEY_F5;
	KeySymToCPCKey[SDLK_KP6] = CPC_KEY_F6;
	KeySymToCPCKey[SDLK_KP7] = CPC_KEY_F7;
	KeySymToCPCKey[SDLK_KP8] = CPC_KEY_F8;
	KeySymToCPCKey[SDLK_KP9] = CPC_KEY_F9;

	KeySymToCPCKey[SDLK_KP_PERIOD] = CPC_KEY_FDOT;

	KeySymToCPCKey[SDLK_LSHIFT] = CPC_KEY_SHIFT;
	KeySymToCPCKey[SDLK_RSHIFT] = CPC_KEY_SHIFT;
	KeySymToCPCKey[SDLK_LCTRL] = CPC_KEY_CONTROL;
	KeySymToCPCKey[SDLK_RCTRL] = CPC_KEY_CONTROL;
	KeySymToCPCKey[SDLK_CAPSLOCK] = CPC_KEY_CAPS_LOCK;
	
	KeySymToCPCKey[SDLK_KP_ENTER] = CPC_KEY_SMALL_ENTER;

	KeySymToCPCKey[SDLK_DELETE] = CPC_KEY_JOY_LEFT;
	KeySymToCPCKey[SDLK_END] = CPC_KEY_JOY_DOWN;
	KeySymToCPCKey[SDLK_PAGEDOWN] = CPC_KEY_JOY_RIGHT;
	KeySymToCPCKey[SDLK_INSERT] = CPC_KEY_JOY_FIRE1;
	KeySymToCPCKey[SDLK_HOME] = CPC_KEY_JOY_UP;
	KeySymToCPCKey[SDLK_PAGEUP] = CPC_KEY_JOY_FIRE2;	

	KeySymToCPCKey[0x0134] = CPC_KEY_COPY;			/* Alt */
	KeySymToCPCKey[0x0137] = CPC_KEY_COPY;			/* Compose */

	switch (layout) {
		case QWERTZ:
			sdl_InitialiseKeyboardMapping_qwertz();
			break;
		case AZERTY:
			sdl_InitialiseKeyboardMapping_azerty();
			break;
	}
}

void	sdl_InitialiseKeyboardMapping_qwertz()
{
	/* German key mappings */
	KeySymToCPCKey[0x00fc] =CPC_KEY_AT;			/* ue */
	KeySymToCPCKey[0x002b] =CPC_KEY_OPEN_SQUARE_BRACKET;	/* Plus */
	KeySymToCPCKey[0x00f6] =CPC_KEY_COLON;			/* oe */
	KeySymToCPCKey[0x00e4] =CPC_KEY_SEMICOLON;		/* ae */
	KeySymToCPCKey[0x0023] =CPC_KEY_CLOSE_SQUARE_BRACKET;	/* Hash */
	KeySymToCPCKey[0x00df] =CPC_KEY_MINUS;			/* sz */
	KeySymToCPCKey[0x00b4] =CPC_KEY_HAT;			/* Accent */
	KeySymToCPCKey[0x005e] =CPC_KEY_CLR;			/* Hat */
	KeySymToCPCKey[0x003c] =CPC_KEY_FORWARD_SLASH;		/* Less */

	/* The next one might break US keyboards?!? */
	KeySymToCPCKey[SDLK_MINUS] = CPC_KEY_BACKSLASH;
}

void	sdl_InitialiseKeyboardMapping_azerty()
{
	// Ajout Ramlaid
	KeySymToCPCKey[SDLK_LALT] = CPC_KEY_COPY;
	
	KeySymToCPCKey[SDLK_AMPERSAND]  = CPC_KEY_1;
	KeySymToCPCKey[SDLK_WORLD_73]   = CPC_KEY_2;
	KeySymToCPCKey[SDLK_QUOTEDBL]   = CPC_KEY_3;
	KeySymToCPCKey[SDLK_QUOTE]      = CPC_KEY_4;
	KeySymToCPCKey[SDLK_LEFTPAREN]  = CPC_KEY_5;
	KeySymToCPCKey[SDLK_MINUS]      = CPC_KEY_6;
	KeySymToCPCKey[SDLK_WORLD_72]   = CPC_KEY_7;
	KeySymToCPCKey[SDLK_UNDERSCORE] = CPC_KEY_8;
	KeySymToCPCKey[SDLK_WORLD_71]   = CPC_KEY_9;
	KeySymToCPCKey[SDLK_WORLD_64]   = CPC_KEY_ZERO;
	
	KeySymToCPCKey[SDLK_RIGHTPAREN] = CPC_KEY_MINUS;
	KeySymToCPCKey[SDLK_EQUALS]     = CPC_KEY_HAT;
	KeySymToCPCKey[SDLK_CARET]      = CPC_KEY_AT;
	KeySymToCPCKey[SDLK_DOLLAR]     = CPC_KEY_OPEN_SQUARE_BRACKET;
	KeySymToCPCKey[SDLK_WORLD_89]   = CPC_KEY_SEMICOLON;
	KeySymToCPCKey[SDLK_ASTERISK]   = CPC_KEY_CLOSE_SQUARE_BRACKET;
	KeySymToCPCKey[SDLK_COMMA]      = CPC_KEY_COMMA;
	KeySymToCPCKey[SDLK_SEMICOLON]  = CPC_KEY_DOT;
	KeySymToCPCKey[SDLK_COLON]      = CPC_KEY_COLON;
	KeySymToCPCKey[SDLK_EXCLAIM]    = CPC_KEY_BACKSLASH;
	KeySymToCPCKey[SDLK_LESS]       = CPC_KEY_FORWARD_SLASH;
}				
