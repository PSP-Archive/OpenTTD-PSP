/* $Id: window.h 9849 2007-05-15 21:24:18Z rubidium $ */

#ifndef WINDOW_H
#define WINDOW_H

#include "macros.h"
#include "string.h"
#include "order.h"

typedef struct WindowEvent WindowEvent;

typedef void WindowProc(Window *w, WindowEvent *e);

/* How the resize system works:
    First, you need to add a WWT_RESIZEBOX to the widgets, and you need
     to add the flag WDF_RESIZABLE to the window. Now the window is ready
     to resize itself.
    As you may have noticed, all widgets have a RESIZE_XXX in their line.
     This lines controls how the widgets behave on resize. RESIZE_NONE means
     it doesn't do anything. Any other option let's one of the borders
     move with the changed width/height. So if a widget has
     RESIZE_RIGHT, and the window is made 5 pixels wider by the user,
     the right of the window will also be made 5 pixels wider.
    Now, what if you want to clamp a widget to the bottom? Give it the flag
     RESIZE_TB. This is RESIZE_TOP + RESIZE_BOTTOM. Now if the window gets
     5 pixels bigger, both the top and bottom gets 5 bigger, so the whole
     widgets moves downwards without resizing, and appears to be clamped
     to the bottom. Nice aint it?
   You should know one more thing about this system. Most windows can't
    handle an increase of 1 pixel. So there is a step function, which
    let the windowsize only be changed by X pixels. You configure this
    after making the window, like this:
      w->resize.step_height = 10;
    Now the window will only change in height in steps of 10.
   You can also give a minimum width and height. The default value is
    the default height/width of the window itself. You can change this
    AFTER window-creation, with:
     w->resize.width or w->resize.height.
   That was all.. good luck, and enjoy :) -- TrueLight */

enum ResizeFlags {
	RESIZE_NONE   = 0,

	RESIZE_LEFT   = 1,
	RESIZE_RIGHT  = 2,
	RESIZE_TOP    = 4,
	RESIZE_BOTTOM = 8,

	RESIZE_LR     = RESIZE_LEFT  | RESIZE_RIGHT,
	RESIZE_RB     = RESIZE_RIGHT | RESIZE_BOTTOM,
	RESIZE_TB     = RESIZE_TOP   | RESIZE_BOTTOM,
	RESIZE_LRB    = RESIZE_LEFT  | RESIZE_RIGHT  | RESIZE_BOTTOM,
	RESIZE_LRTB   = RESIZE_LEFT  | RESIZE_RIGHT  | RESIZE_TOP | RESIZE_BOTTOM,
	RESIZE_RTB    = RESIZE_RIGHT | RESIZE_TOP    | RESIZE_BOTTOM,

	/* The following flags are used by the system to specify what is disabled, hidden, or clicked
	 * They are used in the same place as the above RESIZE_x flags, Widget visual_flags.
	 * These states are used in exceptions. If nothing is specified, they will indicate
	 * Enabled, visible or unclicked widgets*/
	WIDG_DISABLED = 4,  // widget is greyed out, not available
	WIDG_HIDDEN   = 5,  // widget is made invisible
	WIDG_LOWERED  = 6,  // widget is paint lowered, a pressed button in fact
} ResizeFlag;

/* used to indicate the end of widgets' list for vararg functions */
enum {
	WIDGET_LIST_END = -1,
};

typedef struct Widget {
	byte type;                        ///< Widget type, see @WindowWidgetTypes
	byte display_flags;               ///< Resize direction, alignment, etc. during resizing, see @ResizeFlags
	byte color;                       ///< Widget colour, see docs/ottd-colourtext-palette.png
	uint16 left, right, top, bottom;  ///< The position offsets inside the window
	uint16 data;                      ///< The String/Image or special code (list-matrixes) of a widget
	StringID tooltips;                ///< Tooltips that are shown when rightclicking on a widget
} Widget;

typedef enum FrameFlags {
	FR_TRANSPARENT  = 0x01,  ///< Makes the background transparent if set
	FR_BORDERONLY   = 0x10,  ///< Draw border only, no background
	FR_LOWERED      = 0x20,  ///< If set the frame is lowered and the background color brighter (ie. buttons when pressed)
	FR_DARKENED     = 0x40,  ///< If set the background is darker, allows for lowered frames with normal background color when used with FR_LOWERED (ie. dropdown boxes)
} FrameFlags;

void DrawFrameRect(int left, int top, int right, int bottom, int color, FrameFlags flags);

enum WindowEventCodes {
	WE_CLICK               =  0,
	WE_PAINT               =  1,
	WE_MOUSELOOP           =  2,
	WE_TICK                =  3,
	WE_4                   =  4,
	WE_TIMEOUT             =  5,
	WE_PLACE_OBJ           =  6,
	WE_ABORT_PLACE_OBJ     =  7,
	WE_DESTROY             =  8,
	WE_ON_EDIT_TEXT        =  9,
	WE_POPUPMENU_SELECT    = 10,
	WE_POPUPMENU_OVER      = 11,
	WE_DRAGDROP            = 12,
	WE_PLACE_DRAG          = 13,
	WE_PLACE_MOUSEUP       = 14,
	WE_PLACE_PRESIZE       = 15,
	WE_DROPDOWN_SELECT     = 16,
	WE_RCLICK              = 17,
	WE_KEYPRESS            = 18,
	WE_CREATE              = 19,
	WE_MOUSEOVER           = 20,
	WE_ON_EDIT_TEXT_CANCEL = 21,
	WE_RESIZE              = 22,
	WE_MESSAGE             = 23,
	WE_SCROLL              = 24,
	WE_MOUSEWHEEL          = 25,
	WE_INVALIDATE_DATA     = 26,
};

struct WindowEvent {
	byte event;
	union {
		struct{
			Point pt;
			int widget;
		} click;

		struct {
			Point pt;
			TileIndex tile;
			TileIndex starttile;
			int userdata;
		} place;

		struct {
			Point pt;
			int widget;
		} dragdrop;

		struct {
			Point size;
			Point diff;
		} sizing;

		struct {
			char *str;
		} edittext;

		struct {
			Point pt;
		} popupmenu;

		struct {
			int button;
			int index;
		} dropdown;

		struct {
			Point pt;
			int widget;
		} mouseover;

		struct {
			bool cont;     // continue the search? (default true)
			uint16 key;    // 16-bit Unicode value of the key
			uint16 keycode;// untranslated key (including shift-state)
		} keypress;

		struct {
			uint msg;      // message to be sent
			uint wparam;   // additional message-specific information
			uint lparam;   // additional message-specific information
		} message;

		struct {
			Point delta;   // delta position against position of last call
		} scroll;

		struct {
			int wheel;     // how much was 'wheel'd'
		} wheel;
	} we;
};

enum WindowKeyCodes {
	WKC_SHIFT = 0x8000,
	WKC_CTRL  = 0x4000,
	WKC_ALT   = 0x2000,
	WKC_META  = 0x1000,

	// Special ones
	WKC_NONE        =  0,
	WKC_ESC         =  1,
	WKC_BACKSPACE   =  2,
	WKC_INSERT      =  3,
	WKC_DELETE      =  4,

	WKC_PAGEUP      =  5,
	WKC_PAGEDOWN    =  6,
	WKC_END         =  7,
	WKC_HOME        =  8,

	// Arrow keys
	WKC_LEFT        =  9,
	WKC_UP          = 10,
	WKC_RIGHT       = 11,
	WKC_DOWN        = 12,

	// Return & tab
	WKC_RETURN      = 13,
	WKC_TAB         = 14,

	// Numerical keyboard
	WKC_NUM_0       = 16,
	WKC_NUM_1       = 17,
	WKC_NUM_2       = 18,
	WKC_NUM_3       = 19,
	WKC_NUM_4       = 20,
	WKC_NUM_5       = 21,
	WKC_NUM_6       = 22,
	WKC_NUM_7       = 23,
	WKC_NUM_8       = 24,
	WKC_NUM_9       = 25,
	WKC_NUM_DIV     = 26,
	WKC_NUM_MUL     = 27,
	WKC_NUM_MINUS   = 28,
	WKC_NUM_PLUS    = 29,
	WKC_NUM_ENTER   = 30,
	WKC_NUM_DECIMAL = 31,

	// Space
	WKC_SPACE       = 32,

	// Function keys
	WKC_F1          = 33,
	WKC_F2          = 34,
	WKC_F3          = 35,
	WKC_F4          = 36,
	WKC_F5          = 37,
	WKC_F6          = 38,
	WKC_F7          = 39,
	WKC_F8          = 40,
	WKC_F9          = 41,
	WKC_F10         = 42,
	WKC_F11         = 43,
	WKC_F12         = 44,

	// backquote is the key left of "1"
	// we only store this key here, no matter what character is really mapped to it
	// on a particular keyboard. (US keyboard: ` and ~ ; German keyboard: ^ and �)
	WKC_BACKQUOTE   = 45,
	WKC_PAUSE       = 46,

	// 0-9 are mapped to 48-57
	// A-Z are mapped to 65-90
	// a-z are mapped to 97-122
};

typedef struct WindowDesc {
	int16 left, top, width, height;
	WindowClass cls;
	WindowClass parent_cls;
	uint32 flags;
	const Widget *widgets;
	WindowProc *proc;
} WindowDesc;

enum {
	WDF_STD_TOOLTIPS    =  1, /* use standard routine when displaying tooltips */
	WDF_DEF_WIDGET      =  2, /* default widget control for some widgets in the on click event */
	WDF_STD_BTN         =  4, /* default handling for close and drag widgets (widget no 0 and 1) */

	WDF_UNCLICK_BUTTONS = 16, /* Unclick buttons when the window event times out */
	WDF_STICKY_BUTTON   = 32, /* Set window to sticky mode; they are not closed unless closed with 'X' (widget 2) */
	WDF_RESIZABLE       = 64, /* A window can be resized */
};

/* can be used as x or y coordinates to cause a specific placement */
enum {
	WDP_AUTO      = -1, ///< Find a place automatically
	WDP_CENTER    = -2, ///< Center the window (left/right or top/bottom)
	WDP_ALIGN_TBR = -3, ///< Align the right side of the window with the right side of the main toolbar
	WDP_ALIGN_TBL = -4, ///< Align the left side of the window with the left side of the main toolbar
};

typedef struct Textbuf {
	char *buf;                  /* buffer in which text is saved */
	uint16 maxlength, maxwidth; /* the maximum size of the buffer. Maxwidth specifies screensize in pixels, maxlength is in bytes */
	uint16 length, width;       /* the current size of the string. Width specifies screensize in pixels, length is in bytes */
	bool caret;                 /* is the caret ("_") visible or not */
	uint16 caretpos;            /* the current position of the caret in the buffer, in bytes */
	uint16 caretxoffs;          /* the current position of the caret in pixels */
} Textbuf;

#define WP(ptr,str) (*(str*)(ptr)->custom)
/* You cannot 100% reliably calculate the biggest custom struct as
 * the number of pointers in it and alignment will have a huge impact.
 * 96 is the largest window-size for 64-bit machines currently */
#define WINDOW_CUSTOM_SIZE 96

typedef struct Scrollbar {
	uint16 count, cap, pos;
} Scrollbar;

typedef struct ResizeInfo {
	uint width; /* Minimum width and height */
	uint height;

	uint step_width; /* In how big steps the width and height go */
	uint step_height;
} ResizeInfo;

typedef struct WindowMessage {
		int msg;
		int wparam;
		int lparam;
} WindowMessage;

struct Window {
	uint16 flags4;
	WindowClass window_class;
	WindowNumber window_number;

	int left, top;
	int width, height;

	Scrollbar hscroll, vscroll, vscroll2;
	ResizeInfo resize;

	byte caption_color;

	WindowProc *wndproc;
	ViewPort *viewport;
	const Widget *original_widget;
	Widget *widget;
	uint widget_count;
	uint32 desc_flags;

	WindowMessage message;
	byte custom[WINDOW_CUSTOM_SIZE];
};

typedef struct querystr_d {
	StringID caption;
	WindowClass wnd_class;
	WindowNumber wnd_num;
	Textbuf text;
	const char *orig;
	CharSetFilter afilter;
} querystr_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(querystr_d));

typedef struct chatquerystr_d {
	StringID caption;
	WindowClass wnd_class;
	WindowNumber wnd_num;
	Textbuf text;
	const char *orig;
	CharSetFilter afilter;
	int dest;
} chatquerystr_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(chatquerystr_d));

typedef struct query_d {
	StringID caption;
	StringID message;
	WindowClass wnd_class;
	WindowNumber wnd_num;
	void (*ok_cancel_callback)(bool ok_clicked);
	bool calledback;
} query_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(query_d));

typedef struct {
	byte item_count;      /* follow_vehicle */
	byte sel_index;       /* scrollpos_x */
	byte main_button;     /* scrollpos_y */
	byte action_id;
	StringID string_id;   /* unk30 */
	uint16 checked_items; /* unk32 */
	byte disabled_items;
} menu_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(menu_d));

typedef struct {
	int16 data_1, data_2, data_3;
	int16 data_4, data_5;
	bool close;
	byte byte_1;
} def_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(def_d));

typedef struct {
	void *data;
} void_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(void_d));

typedef struct {
	uint16 base;
	uint16 count;
} tree_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(tree_d));

typedef struct {
	StringID string_id;
	byte paramcount;
	uint32 params[5];
} tooltips_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(tooltips_d));

typedef struct {
	byte vehicle_type;
	union {
		byte railtype;
		byte acc_planes; // AIRCRAFT_ONLY, ALL, HELICOPTERS_ONLY
	} filter;
	byte sel_index;  // deprecated value, used for 'unified' ship and road
	bool descending_sort_order;
	byte sort_criteria;
	EngineID sel_engine;
	EngineID rename_engine;
	EngineList eng_list;
} buildvehicle_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(buildvehicle_d));

typedef struct {
	byte vehicletype;
	byte sel_index[2];
	EngineID sel_engine[2];
	uint16 count[2];
	bool wagon_btnstate; // true means engine is selected
} replaceveh_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(replaceveh_d));

typedef struct {
	VehicleID sel;
	byte type;
	bool generate_list;
	uint16 engine_list_length;
	uint16 wagon_list_length;
	uint16 engine_count;
	uint16 wagon_count;
	Vehicle **vehicle_list;
	Vehicle **wagon_list;
} depot_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(depot_d));

typedef struct {
	int sel;
} order_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(order_d));

typedef struct {
	byte tab;
} traindetails_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(traindetails_d));

typedef struct {
	int32 scroll_x;
	int32 scroll_y;
	int32 subscroll;
} smallmap_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(smallmap_d));

typedef struct {
	uint32 face;
	byte gender;
} facesel_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(facesel_d));

typedef struct {
	int sel;
	struct RefitOption *cargo;
	struct RefitList *list;
	uint length;
	VehicleOrderID order;
} refit_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(refit_d));

typedef struct {
	VehicleID follow_vehicle;
	int32 scrollpos_x;
	int32 scrollpos_y;
} vp_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(vp_d));

// vp2_d is the same as vp_d, except for the data_# values..
typedef struct {
	VehicleID follow_vehicle;
	int32 scrollpos_x;
	int32 scrollpos_y;
	byte data_1;
	byte data_2;
	byte data_3;
} vp2_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(vp2_d));

typedef struct {
	uint16 follow_vehicle;
	int32 scrollpos_x;
	int32 scrollpos_y;
	NewsItem *ni;
} news_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(news_d));

typedef struct {
	uint32 background_img;
	int8 rank;
} highscore_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(highscore_d));

typedef struct {
	int height;
	uint16 counter;
} scroller_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(scroller_d));

typedef enum SortListFlags {
	VL_DESC    = 0x01,  // sort descending or ascending
	VL_RESORT  = 0x02,  // instruct the code to resort the list in the next loop
	VL_REBUILD = 0x04   // create sort-listing to use for qsort and friends
} SortListFlags;

typedef struct Listing {
	bool order;    // Ascending/descending
	byte criteria; // Sorting criteria
} Listing;

typedef struct list_d {
	uint16 list_length;  // length of the list being sorted
	byte sort_type;      // what criteria to sort on
	SortListFlags flags; // used to control sorting/resorting/etc.
	uint16 resort_timer; // resort list after a given amount of ticks if set
} list_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(list_d));

typedef struct message_d {
	int msg;
	int wparam;
	int lparam;
} message_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(message_d));

typedef struct dropdown_d {
	uint32 disabled_state;
	uint32 hidden_state;
	WindowClass parent_wnd_class;
	WindowNumber parent_wnd_num;
	byte parent_button;
	byte num_items;
	byte selected_index;
	const StringID *items;
	byte click_delay;
	bool drag_mode;
} dropdown_d;
assert_compile(WINDOW_CUSTOM_SIZE >= sizeof(dropdown_d));


/****************** THESE ARE NOT WIDGET TYPES!!!!! *******************/
enum WindowWidgetBehaviours {
	WWB_PUSHBUTTON  = 1 << 5,

	WWB_MASK        = 0xE0,
};


enum WindowWidgetTypes {
	WWT_EMPTY,

	WWT_PANEL,      /* simple depressed panel */
	WWT_INSET,      /* pressed (inset) panel, most commonly used as combo box _text_ area */
	WWT_IMGBTN,     /* button with image */
	WWT_IMGBTN_2,   /* button with diff image when clicked */

	WWT_TEXTBTN,    /* button with text */
	WWT_TEXTBTN_2,  /* button with diff text when clicked */
	WWT_LABEL,      /* centered label */
	WWT_MATRIX,
	WWT_SCROLLBAR,
	WWT_FRAME,      /* frame */
	WWT_CAPTION,

	WWT_HSCROLLBAR,
	WWT_STICKYBOX,
	WWT_SCROLL2BAR, /* 2nd vertical scrollbar*/
	WWT_RESIZEBOX,
	WWT_CLOSEBOX,
	WWT_LAST,       /* Last Item. use WIDGETS_END to fill up padding!! */

	WWT_MASK = 0x1F,

	WWT_PUSHBTN     = WWT_PANEL   | WWB_PUSHBUTTON,
	WWT_PUSHTXTBTN  = WWT_TEXTBTN | WWB_PUSHBUTTON,
	WWT_PUSHIMGBTN  = WWT_IMGBTN  | WWB_PUSHBUTTON,
};

#define WIDGETS_END WWT_LAST,   RESIZE_NONE,     0,     0,     0,     0,     0, 0, STR_NULL

enum WindowFlags {
	WF_TIMEOUT_SHL       = 0,
	WF_TIMEOUT_MASK      = 7,
	WF_DRAGGING          = 1 <<  3,
	WF_SCROLL_UP         = 1 <<  4,
	WF_SCROLL_DOWN       = 1 <<  5,
	WF_SCROLL_MIDDLE     = 1 <<  6,
	WF_HSCROLL           = 1 <<  7,
	WF_SIZING            = 1 <<  8,
	WF_STICKY            = 1 <<  9,

	WF_DISABLE_VP_SCROLL = 1 << 10,

	WF_WHITE_BORDER_ONE  = 1 << 11,
	WF_WHITE_BORDER_MASK = 3 << 11,
	WF_SCROLL2           = 1 << 13,
};

/* window.c */
void CallWindowEventNP(Window *w, int event);
void CallWindowTickEvent(void);
void SetWindowDirty(const Window *w);
void SendWindowMessage(WindowClass wnd_class, WindowNumber wnd_num, uint msg, uint wparam, uint lparam);
void SendWindowMessageClass(WindowClass wnd_class, uint msg, uint wparam, uint lparam);

Window *FindWindowById(WindowClass cls, WindowNumber number);
void DeleteWindow(Window *w);
void DeletePlayerWindows(PlayerID pi);
void ChangeWindowOwner(PlayerID old_player, PlayerID new_player);
Window *BringWindowToFrontById(WindowClass cls, WindowNumber number);
Window *FindWindowFromPt(int x, int y);

bool IsWindowOfPrototype(const Window *w, const Widget *widget);
void AssignWidgetToWindow(Window *w, const Widget *widget);
Window *AllocateWindow(
							int x,
							int y,
							int width,
							int height,
							WindowProc *proc,
							WindowClass cls,
							const Widget *widget);

Window *AllocateWindowDesc(const WindowDesc *desc);
Window *AllocateWindowDescFront(const WindowDesc *desc, int window_number);

void DrawWindowViewport(const Window *w);
void ResizeWindow(Window *w, int x, int y);

/**
 * Sets the enabled/disabled status of a widget.
 * By default, widgets are enabled.
 * On certain conditions, they have to be disabled.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 * @param disab_stat : status to use ie: disabled = true, enabled = false
 */
static inline void SetWindowWidgetDisabledState(Window *w, byte widget_index, bool disab_stat)
{
	assert(widget_index < w->widget_count);
	SB(w->widget[widget_index].display_flags, WIDG_DISABLED, 1, !!disab_stat);
}

/**
 * Sets a widget to disabled.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 */
static inline void DisableWindowWidget(Window *w, byte widget_index)
{
	SetWindowWidgetDisabledState(w, widget_index, true);
}

/**
 * Sets a widget to Enabled.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 */
static inline void EnableWindowWidget(Window *w, byte widget_index)
{
	SetWindowWidgetDisabledState(w, widget_index, false);
}

/**
 * Gets the enabled/disabled status of a widget.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 * @return status of the widget ie: disabled = true, enabled = false
 */
static inline bool IsWindowWidgetDisabled(const Window *w, byte widget_index)
{
	assert(widget_index < w->widget_count);
	return HASBIT(w->widget[widget_index].display_flags, WIDG_DISABLED);
}

/**
 * Sets the hidden/shown status of a widget.
 * By default, widgets are visible.
 * On certain conditions, they have to be hidden.
 * @param w Window on which the widget is located
 * @param widget_index index of this widget in the window
 * @param hidden_stat status to use ie. hidden = true, visible = false
 */
static inline void SetWindowWidgetHiddenState(Window *w, byte widget_index, bool hidden_stat)
{
	assert(widget_index < w->widget_count);
	SB(w->widget[widget_index].display_flags, WIDG_HIDDEN, 1, !!hidden_stat);
}

/**
 * Sets a widget hidden.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 */
static inline void HideWindowWidget(Window *w, byte widget_index)
{
	SetWindowWidgetHiddenState(w, widget_index, true);
}

/**
 * Sets a widget visible.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 */
static inline void ShowWindowWidget(Window *w, byte widget_index)
{
	SetWindowWidgetHiddenState(w, widget_index, false);
}

/**
 * Gets the visibility of a widget.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 * @return status of the widget ie: hidden = true, visible = false
 */
static inline bool IsWindowWidgetHidden(const Window *w, byte widget_index)
{
	assert(widget_index < w->widget_count);
	return HASBIT(w->widget[widget_index].display_flags, WIDG_HIDDEN);
}

/**
 * Sets the lowered/raised status of a widget.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 * @param hidden_stat : status to use ie: lowered = true, raised = false
 */
static inline void SetWindowWidgetLoweredState(Window *w, byte widget_index, bool lowered_stat)
{
	assert(widget_index < w->widget_count);
	SB(w->widget[widget_index].display_flags, WIDG_LOWERED, 1, !!lowered_stat);
}

/**
 * Invert the lowered/raised  status of a widget.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 */
static inline void ToggleWidgetLoweredState(Window *w, byte widget_index)
{
	assert(widget_index < w->widget_count);
	TOGGLEBIT(w->widget[widget_index].display_flags, WIDG_LOWERED);
}

/**
 * Marks a widget as lowered.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 */
static inline void LowerWindowWidget(Window *w, byte widget_index)
{
	SetWindowWidgetLoweredState(w, widget_index, true);
}

/**
 * Marks a widget as raised.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 */
static inline void RaiseWindowWidget(Window *w, byte widget_index)
{
	SetWindowWidgetLoweredState(w, widget_index, false);
}

/**
 * Gets the lowered state of a widget.
 * @param w : Window on which the widget is located
 * @param widget_index : index of this widget in the window
 * @return status of the widget ie: lowered = true, raised= false
 */
static inline bool IsWindowWidgetLowered(const Window *w, byte widget_index)
{
	assert(widget_index < w->widget_count);
	return HASBIT(w->widget[widget_index].display_flags, WIDG_LOWERED);
}

void InitWindowSystem(void);
void UnInitWindowSystem(void);
void ResetWindowSystem(void);
int GetMenuItemIndex(const Window *w, int x, int y);
void InputLoop(void);
void HandleKeypress(uint32 key);
void HandleMouseEvents(void);
void UpdateWindows(void);
void InvalidateWidget(const Window *w, byte widget_index);
void InvalidateThisWindowData(Window *w);
void InvalidateWindowData(WindowClass cls, WindowNumber number);
void RaiseWindowButtons(Window *w);
void RelocateAllWindows(int neww, int newh);
int PositionMainToolbar(Window *w);
void CDECL SetWindowWidgetsDisabledState(Window *w, bool disab_stat, int widgets, ...);
void CDECL SetWindowWidgetsHiddenState(Window *w, bool hidden_stat, int widgets, ...);
void CDECL SetWindowWidgetsLoweredState(Window *w, bool lowered_stat, int widgets, ...);

/* misc_gui.c*/
void GuiShowTooltipsWithArgs(StringID str, uint paramcount, const uint params[]);
static inline void GuiShowTooltips(StringID str)
{
	GuiShowTooltipsWithArgs(str, 0, NULL);
}

/* widget.c */
int GetWidgetFromPos(const Window *w, int x, int y);
void DrawWindowWidgets(const Window *w);
void ShowDropDownMenu(Window *w, const StringID *strings, int selected, int button, uint32 disabled_mask, uint32 hidden_mask);

void HandleButtonClick(Window *w, byte widget);

Window *GetCallbackWnd(void);
void DeleteNonVitalWindows(void);
void DeleteAllNonVitalWindows(void);
void HideVitalWindows(void);
void ShowVitalWindows(void);
Window **FindWindowZPosition(const Window *w);

/* window.c */
extern Window *_z_windows[];
extern Window **_last_z_window;
#define FOR_ALL_WINDOWS(wz) for (wz = _z_windows; wz != _last_z_window; wz++)

VARDEF Point _cursorpos_drag_start;

VARDEF bool _left_button_down;
VARDEF bool _left_button_clicked;

VARDEF bool _right_button_down;
VARDEF bool _right_button_clicked;

VARDEF int _scrollbar_start_pos;
VARDEF int _scrollbar_size;
VARDEF byte _scroller_click_timeout;

VARDEF bool _scrolling_scrollbar;
VARDEF bool _scrolling_viewport;
VARDEF bool _popup_menu_active;

VARDEF byte _special_mouse_mode;
enum SpecialMouseMode {
	WSM_NONE     = 0,
	WSM_DRAGDROP = 1,
	WSM_SIZING   = 2,
	WSM_PRESIZE  = 3,
};

void ScrollbarClickHandler(Window *w, const Widget *wi, int x, int y);

#endif /* WINDOW_H */
