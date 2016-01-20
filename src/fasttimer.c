/*************************************
 * circtimer.c
 *
 * This is the main source file
 * for the circtimer app exclusivly 
 * for the Pebble Time Round
 *************************************/

#include <pebble.h>

#define PERSIST_KEY_WAKEUP_ID 42 

typedef struct {
  int minutes;
	int seconds;
	int state;
	time_t timestamp_to_wakeup;
} Time;

static Window *s_main_window;
static TextLayer *s_output_layer;
static TextLayer *s_output_layer_sec;
static TextLayer *s_help_layer;

static AppTimer *s_blink_timer;

static Time s_current_timer;
static char s_time_buffer[10];
static char s_time_buffer_sec[3];

static WakeupId s_wakeup_id;

/* WINDOW HANDLERS */
static void main_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
	
	// Create banner layer
	BitmapLayer *bitmap_layer = bitmap_layer_create(GRect(0, 0, window_bounds.size.w, 70));
	bitmap_layer_set_background_color(bitmap_layer, GColorDarkCandyAppleRed);
	bitmap_layer_set_bitmap(bitmap_layer, gbitmap_create_with_resource(RESOURCE_ID_ALARM_CLOCK));
	layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
	
	// Create help text
  s_help_layer = text_layer_create(GRect(0, 104, window_bounds.size.w, 50));
	text_layer_set_background_color(s_help_layer, GColorClear);
	text_layer_set_text_color(s_help_layer, GColorDarkCandyAppleRed);
	text_layer_set_font(s_help_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(s_help_layer, GTextAlignmentCenter);
	text_layer_set_text(s_help_layer, "press select to start");
	layer_add_child(window_layer, text_layer_get_layer(s_help_layer));
	
  // Create timer text
	s_output_layer = text_layer_create(GRect(0, 65, window_bounds.size.w, 50));
	text_layer_set_background_color(s_output_layer, GColorClear);
	text_layer_set_text_color(s_output_layer, GColorDarkCandyAppleRed);
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
	text_layer_set_text(s_output_layer, "00:00");
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
	
	// Create timer sec text
	s_output_layer_sec = text_layer_create(GRect(PBL_IF_ROUND_ELSE(142, 120), 83, 40, 50));
	text_layer_set_background_color(s_output_layer_sec, GColorClear);
	text_layer_set_text_color(s_output_layer_sec, GColorDarkCandyAppleRed);
  text_layer_set_font(s_output_layer_sec, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LECO_NUMBERS_18)));
  text_layer_set_text_alignment(s_output_layer_sec, GTextAlignmentLeft);
	text_layer_set_text(s_output_layer_sec, "00");
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer_sec));
	
}
static void main_window_unload(Window *window) {
	text_layer_destroy(s_output_layer);
	text_layer_destroy(s_output_layer_sec);
	text_layer_destroy(s_help_layer);
}
/* WINDOW HANDLERS */

/* TIMER HANDLERS */
void second_timer_callback(void *data) {
	if (s_current_timer.state == 1) {
		
		// set help text
		text_layer_set_text(s_help_layer, "running...");
		
		// reduce timer by one second
		s_current_timer.seconds--;
		snprintf(s_time_buffer_sec, sizeof(s_time_buffer_sec), "%02d", (int)(s_current_timer.seconds % 60));
		text_layer_set_text(s_output_layer_sec, s_time_buffer_sec);
		
		// register next execution	-> return each minute
		if (s_current_timer.seconds == 0) {
			// end of timer reached
			s_current_timer.state = 2;
			text_layer_set_text(s_help_layer, "finished");
		} else {
			// refresh if not finished
			s_current_timer.state = 1;
			s_blink_timer = app_timer_register(1000, (AppTimerCallback) second_timer_callback, NULL);
		}
	} else if (s_current_timer.state == 2) {
		// vibe as long as no button is pressed
		vibes_double_pulse();
		s_blink_timer = app_timer_register(1000, (AppTimerCallback) second_timer_callback, NULL);
	}
	
	// blink delimeter
	if ((s_time_buffer[2] == ':') && (s_current_timer.seconds > 0)) {
		snprintf(s_time_buffer, sizeof(s_time_buffer), "%02d.%02d", (int)(s_current_timer.seconds / 3600), (int)(s_current_timer.seconds / 60 % 60));
		text_layer_set_text(s_output_layer, s_time_buffer);
	} else {
		snprintf(s_time_buffer, sizeof(s_time_buffer), "%02d:%02d", (int)(s_current_timer.seconds / 3600), (int)(s_current_timer.seconds / 60 % 60));
		text_layer_set_text(s_output_layer, s_time_buffer);
	}

	APP_LOG(APP_LOG_LEVEL_DEBUG, "%d sec: %02d:%02d:%02d", s_current_timer.seconds, (int)(s_current_timer.seconds / 3600), (int)(s_current_timer.seconds / 60 % 60), (int)(s_current_timer.seconds % 60));
}
/* TIMER HANDLERS */

/* WAKEUP HANDLERS */
static void check_wakeup() {
	s_wakeup_id = persist_read_int(PERSIST_KEY_WAKEUP_ID);
	if (s_wakeup_id > 0) {
		wakeup_query(s_wakeup_id, &s_current_timer.timestamp_to_wakeup);
		s_current_timer.seconds = s_current_timer.timestamp_to_wakeup - time(NULL);
		
		// refresh screen after wakeup
		s_current_timer.state = 1;
		s_blink_timer = app_timer_register(1, (AppTimerCallback) second_timer_callback, NULL);
	}
}
static void wakeup_handler(WakeupId id, int32_t reason) {
	persist_delete(PERSIST_KEY_WAKEUP_ID);
	
	// app woken app by timing event
	s_current_timer.state = 2;
	text_layer_set_text(s_help_layer, "finished");
	vibes_double_pulse();
	
	// activate blink timer for start finishing procedure
	s_blink_timer = app_timer_register(1, (AppTimerCallback) second_timer_callback, NULL);
}
/* WAKEUP HANDLERS */

/* CLICK HANDLERS */	
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	//increase the timer
	if (s_current_timer.state == 0) {	
			s_current_timer.seconds = s_current_timer.seconds + 1*60;
		
			// Compose string of all data
		snprintf(s_time_buffer_sec, sizeof(s_time_buffer_sec), "%02d", (int)(s_current_timer.seconds % 60));
		snprintf(s_time_buffer, sizeof(s_time_buffer), "%02d:%02d", (int)(s_current_timer.seconds / 3600), (int)(s_current_timer.seconds / 60 % 60));
		text_layer_set_text(s_output_layer_sec, s_time_buffer_sec);
		text_layer_set_text(s_output_layer, s_time_buffer);
	}
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	// initiate the timer -> return each minute
	if ((s_current_timer.state == 0) && (s_current_timer.seconds > 0)) {
		// set timers for logic
		s_blink_timer = app_timer_register(1000, (AppTimerCallback) second_timer_callback, NULL);
	
		s_current_timer.state = 1;
		
		// set timestamp to wakeup
		s_current_timer.timestamp_to_wakeup = time(NULL) + s_current_timer.seconds;
		s_wakeup_id = wakeup_schedule(s_current_timer.timestamp_to_wakeup, s_current_timer.state, true);
		persist_write_int(PERSIST_KEY_WAKEUP_ID, s_wakeup_id);
		
	} else if (s_current_timer.state == 2) {
		// stop vibrating
		vibes_cancel();
		s_current_timer.state = 0;
		s_current_timer.seconds = 0;
		s_current_timer.minutes = 0;
		text_layer_set_text(s_help_layer, "press select to start");
	}
	
}
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	// decrease the timer
	if (s_current_timer.state == 0) {
		if (s_current_timer.seconds > 1*60) {
			s_current_timer.seconds = s_current_timer.seconds - 1*60;
		} else {
			s_current_timer.seconds = 0;
		}
			
		// Compose string of all data
		snprintf(s_time_buffer_sec, sizeof(s_time_buffer_sec), "%02d", (int)(s_current_timer.seconds % 60));
		snprintf(s_time_buffer, sizeof(s_time_buffer), "%02d:%02d", (int)(s_current_timer.seconds / 3600), (int)(s_current_timer.seconds / 60 % 60));
		text_layer_set_text(s_output_layer_sec, s_time_buffer_sec);
		text_layer_set_text(s_output_layer, s_time_buffer);
	}
}
static void up_longclick_handler(ClickRecognizerRef recognizer, void *context) {
	if (s_current_timer.state == 0) {
		s_current_timer.seconds = s_current_timer.seconds + 15*60;
		
		// Compose string of all data
		snprintf(s_time_buffer_sec, sizeof(s_time_buffer_sec), "%02d", (int)(s_current_timer.seconds % 60));
		snprintf(s_time_buffer, sizeof(s_time_buffer), "%02d:%02d", (int)(s_current_timer.seconds / 3600), (int)(s_current_timer.seconds / 60 % 60));
		text_layer_set_text(s_output_layer_sec, s_time_buffer_sec);
		text_layer_set_text(s_output_layer, s_time_buffer);
	}
}
static void select_longclick_handler (ClickRecognizerRef recognizer, void *context) {
	// Reset Timer on long press
	if ((s_current_timer.state == 0) || (s_current_timer.state == 1)) {
		s_current_timer.minutes = 0;
		s_current_timer.seconds = 0;
		s_current_timer.state = 0;

		// Compose string of all data
		snprintf(s_time_buffer, sizeof(s_time_buffer), "%02i:%02i", (int)(s_current_timer.minutes / 60), s_current_timer.minutes % 60);
		text_layer_set_text(s_output_layer, s_time_buffer);
		
		snprintf(s_time_buffer_sec, sizeof(s_time_buffer_sec), "%02d", (int)(s_current_timer.seconds % 60));
		text_layer_set_text(s_output_layer_sec, s_time_buffer_sec);
		
		text_layer_set_text(s_help_layer, "press select to start");
		
		//delete the persistance key
		persist_delete(PERSIST_KEY_WAKEUP_ID);
		wakeup_cancel_all();
	}
}
static void down_longclick_handler (ClickRecognizerRef recognizer, void *context) {
	if (s_current_timer.state == 0) {
		if (s_current_timer.seconds > 15*60) {
			s_current_timer.seconds = s_current_timer.seconds - 15*60;
		} else {
			s_current_timer.seconds = 0;
		}
		
		// Compose string of all data
		snprintf(s_time_buffer_sec, sizeof(s_time_buffer_sec), "%02d", (int)(s_current_timer.seconds % 60));
		snprintf(s_time_buffer, sizeof(s_time_buffer), "%02d:%02d", (int)(s_current_timer.seconds / 3600), (int)(s_current_timer.seconds / 60 % 60));
		text_layer_set_text(s_output_layer_sec, s_time_buffer_sec);
		text_layer_set_text(s_output_layer, s_time_buffer);
	}
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
	/* NORMAL OPERATION *************
	 * up: 			increase time by one minute
	 * down:		decrease time by one minute
	 * select:	start timer with the set time
	 * NORMAL OPERATION *************/
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);

	/* SPECIAL OPERATION *************
	 * up: 			increase time by 15 minutes
	 * down:		decrease time by 15 minutes
	 * select:	reset timer to 00:00 even in operation
	 * SPECIAL OPERATION *************/
	window_long_click_subscribe(BUTTON_ID_UP, 800, up_longclick_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_SELECT, 1200, select_longclick_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_DOWN, 800, down_longclick_handler, NULL);
}
/* CLICK HANDLERS */


static void init() {
  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });

	// Set window parameters
	window_set_background_color(s_main_window, GColorWhite);
  window_stack_push(s_main_window, true);
		
	// Activate Click Handler
	window_set_click_config_provider(s_main_window, click_config_provider);
	
	// Reset Time Struct
	s_current_timer.seconds = 0;
	s_current_timer.minutes = 0;
	s_current_timer.timestamp_to_wakeup = 0 + time(NULL);
	s_current_timer.state = 0;	// 0 = stop, 1 = running, 2 = vibrating
	
	// Subscribe to Wakeup API
  wakeup_service_subscribe(wakeup_handler);

  // Was this a wakeup launch?
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
		WakeupId id = 0;
		int32_t reason = 0;
		
    // Get details and handle the wakeup
    wakeup_get_launch_event(&id, &reason);
    wakeup_handler(id, reason);
  } else {
    // Check whether a wakeup will occur soon
    check_wakeup();
  }
}
static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);
}
int main(void) {
  init();
  app_event_loop();
  deinit();
}