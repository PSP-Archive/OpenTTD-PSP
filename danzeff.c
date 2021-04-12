#include "danzeff.h"


#define false 0
#define true 1

/*bool*/ int holding = false;     //user is holding a button
/*bool*/ int dirty = true;        //keyboard needs redrawing
/*bool*/ int shifted = false;     //user is holding shift
int mode = 0;             //charset selected. (0 - letters or 1 - numbers)
/*bool*/ int initialized = false; //keyboard is initialized

//Position on the 3-3 grid the user has selected (range 0-2)
int selected_x = 1;
int selected_y = 1;

//Variable describing where each of the images is
#define guiStringsSize 12 /* size of guistrings array */
#define PICS_BASEDIR "./graphics/"
char *guiStrings[] = 
{
	PICS_BASEDIR "keys.png", PICS_BASEDIR "keys_t.png", PICS_BASEDIR "keys_s.png",
	PICS_BASEDIR "keys_c.png", PICS_BASEDIR "keys_c_t.png", PICS_BASEDIR "keys_s_c.png",
	PICS_BASEDIR "nums.png", PICS_BASEDIR "nums_t.png", PICS_BASEDIR "nums_s.png",
	PICS_BASEDIR "nums_c.png", PICS_BASEDIR "nums_c_t.png", PICS_BASEDIR "nums_s_c.png"
};

//amount of modes (non shifted), each of these should have a corresponding shifted mode.
#define MODE_COUNT 2
//this is the layout of the keyboard
char modeChar[MODE_COUNT*2][3][3][5] = 
{
	{	//standard letters
		{ ",abc",  ".def","!ghi" },
		{ "-jkl","\010m n", "?opq" },
		{ "(rst",  ":uvw",")xyz" }
	},
	
	{	//capital letters
		{ "^ABC",  "@DEF","*GHI" },
		{ "_JKL","\010M N", "\"OPQ" },
		{ "=RST",  ";UVW","/XYZ" }
	},
	
	{	//numbers
		{ "\0\0\0001","\0\0\0002","\0\0\0003" },
		{ "\0\0\0004",  "\010\0 5","\0\0\0006" },
		{ "\0\0\0007","\0\0\0008", "\0\00009" }
	},

	{	//special characters
		{ "'(.)",  "\"<'>","-[_]" },
		{ "!{?}","\010\0 \0", "+\\=/" },
		{ ":@;#",  "~$`%","*^|&" }
	}
};

/*bool*/ int danzeff_isinitialized()
{
	return initialized;
}

/*bool*/ int danzeff_dirty()
{
	return dirty;
}

/** Attempts to read a character from the controller
* If no character is pressed then we return 0
* Other special values: 1 = move left, 2 = move right, 3 = select, 4 = start
* Every other value should be a standard ascii value.
* An unsigned int is returned so in the future we can support unicode input
*/
unsigned int danzeff_readInput(SceCtrlData pspctrl)
{
	//Work out where the analog stick is selecting
	int x = 1;
	int y = 1;
	if (pspctrl.Lx < 85)     x -= 1;
	else if (pspctrl.Lx > 170) x += 1;
	
	if (pspctrl.Ly < 85)     y -= 1;
	else if (pspctrl.Ly > 170) y += 1;
	
	if (selected_x != x || selected_y != y) //If they've moved, update dirty
	{
		dirty = true;
		selected_x = x;
		selected_y = y;
	}
	//if they are changing shift then that makes it dirty too
	if ((!shifted && (pspctrl.Buttons & PSP_CTRL_RTRIGGER)) || (shifted && !(pspctrl.Buttons & PSP_CTRL_RTRIGGER)))
		dirty = true;
	
	unsigned int pressed = 0; //character they have entered, 0 as that means 'nothing'
	shifted = (pspctrl.Buttons & PSP_CTRL_RTRIGGER)?true:false;
	
	if (!holding)
	{
		if (pspctrl.Buttons& (PSP_CTRL_CROSS|PSP_CTRL_CIRCLE|PSP_CTRL_TRIANGLE|PSP_CTRL_SQUARE)) //pressing a char select button
		{
			int innerChoice = 0;
			if      (pspctrl.Buttons & PSP_CTRL_TRIANGLE)
				innerChoice = 0;
			else if (pspctrl.Buttons & PSP_CTRL_SQUARE)
				innerChoice = 1;
			else if (pspctrl.Buttons & PSP_CTRL_CROSS)
				innerChoice = 2;
			else //if (pspctrl.Buttons & PSP_CTRL_CIRCLE)
				innerChoice = 3;
			
			//Now grab the value out of the array
			pressed = modeChar[ mode*2 + shifted][y][x][innerChoice];
		}
		else if (pspctrl.Buttons& PSP_CTRL_LTRIGGER) //toggle mode
		{
			dirty = true;
			mode++;
			mode %= MODE_COUNT;
		}
		else if (pspctrl.Buttons& PSP_CTRL_DOWN)
		{
			pressed = '\n';
		}
		else if (pspctrl.Buttons& PSP_CTRL_UP)
		{
			pressed = 131080; //backspace
		}
		else if (pspctrl.Buttons& PSP_CTRL_LEFT)
		{
			pressed = 589824; //LEFT
		}
		else if (pspctrl.Buttons& PSP_CTRL_RIGHT)
		{
			pressed = 720896; //RIGHT
		}
		else if (pspctrl.Buttons& PSP_CTRL_SELECT)
		{
			pressed = DANZEFF_SELECT; //SELECT
		}
		else if (pspctrl.Buttons& PSP_CTRL_START)
		{
			pressed = DANZEFF_START; //START
		}
	}

	holding = pspctrl.Buttons & ~PSP_CTRL_RTRIGGER; //RTRIGGER doesn't set holding
	
	return pressed;
}

///-----------------------------------------------------------
///These are specific to the implementation, they should have the same behaviour across implementations.
///-----------------------------------------------------------


///This is the original SDL implementation
#ifdef DANZEFF_SDL

SDL_Surface* keyBits[guiStringsSize];
int keyBitsSize = 0;
int moved_x = 0, moved_y = 0; // location that we are moved to

///variable needed for rendering in SDL, the screen surface to draw to, and a function to set it!
SDL_Surface* danzeff_screen;
SDL_Rect danzeff_screen_rect;
void danzef_set_screen(SDL_Surface* screen)
{
	danzeff_screen = screen;
	danzeff_screen_rect.x = 0;
	danzeff_screen_rect.y = 0;
	danzeff_screen_rect.h = screen->h;
	danzeff_screen_rect.w = screen->w;
}


///Internal function to draw a surface internally offset
//Render the given surface at the current screen position offset by screenX, screenY
//the surface will be internally offset by offsetX,offsetY. And the size of it to be drawn will be intWidth,intHeight
void surface_draw_offset(SDL_Surface* pixels, int screenX, int screenY, int offsetX, int offsetY, int intWidth, int intHeight)
{
	//move the draw position
	danzeff_screen_rect.x = moved_x + screenX;
	danzeff_screen_rect.y = moved_y + screenY;

	//Set up the rectangle
	SDL_Rect pixels_rect;
	pixels_rect.x = offsetX;
	pixels_rect.y = offsetY;
	pixels_rect.w = intWidth;
	pixels_rect.h = intHeight;
	
	SDL_BlitSurface(pixels, &pixels_rect, danzeff_screen, &danzeff_screen_rect);
}

///Draw a surface at the current moved_x, moved_y
void surface_draw(SDL_Surface* pixels)
{
	surface_draw_offset(pixels, 0, 0, 0, 0, pixels->w, pixels->h);
}

/* load all the guibits that make up the OSK */
void danzeff_load()
{
	if (initialized) return;
	
	int a;
	for (a = 0; a < guiStringsSize; a++)
	{
		keyBits[a] = IMG_Load(guiStrings[a]);
		if (keyBits[a] == NULL)
		{
			//ERROR! out of memory.
			//free all previously created surfaces and set initialized to false
			int b;
			for (b = 0; b < a; b++)
			{
				SDL_FreeSurface(keyBits[b]);
				keyBits[b] = NULL;
			}
			initialized = false;
			return;
		}
	}
	initialized = true;
}

/* remove all the guibits from memory */
void danzeff_free()
{
	if (!initialized) return;
	
	int a;
	for (a = 0; a < guiStringsSize; a++)
	{
		SDL_FreeSurface(keyBits[a]);
		keyBits[a] = NULL;
	}
	initialized = false;
}

/* draw the keyboard at the current position */
void danzeff_render()
{
	printf("Drawing Keyboard at %i,%i\n", selected_x, selected_y);
	dirty = false;
	
	///Draw the background for the selected keyboard either transparent or opaque
	///this is the whole background image, not including the special highlighted area
	//if center is selected then draw the whole thing opaque
	//if (selected_x == 1 && selected_y == 1)
		surface_draw(keyBits[6*mode + shifted*3]);
	//else
	//	surface_draw(keyBits[6*mode + shifted*3 + 1]);
	
	///Draw the current Highlighted Selector (orange bit)
	surface_draw_offset(keyBits[6*mode + shifted*3 + 2], 
	//Offset from the current draw position to render at
	selected_x*43, selected_y*43, 
	//internal offset of the image
	selected_x*64,selected_y*64,
	//size to render (always the same)
	64, 64);
}

/* move the position the keyboard is currently drawn at */
void danzeff_moveTo(const int newX, const int newY)
{
	moved_x = newX;
	moved_y = newY;
}

#endif //DANZEFF_SDL
