#include <pebble.h>
#include <string.h>
  
#define READY 1
#define ERA 2
#define SHOWWEEKDAYSTYLE 3

// Persistent storage key
#define SETTINGS_KEY 4

// Define our settings struct
typedef struct ClaySettings {
  char era[25];
  char show_weekday_style[25];
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_ABSide_layer;
static GBitmap *s_bitmap;
static GBitmap *t_bitmap;
static BitmapLayer *s_bitmap_layer;
static BitmapLayer *t_bitmap_layer;
static Layer *s_window_layer;

// I'm making this global so that I can always get the value
int tick_counter;

// This is different from the setting from the user, as if it's random, we need to know the current displayed era
char era[25];

int background_round_y_pos = -9;
int background_square_y_pos = -10;

int time_round_y_pos = 125;
int time_square_y_pos = 110;

int date_round_y_pos = 10;
int date_square_y_pos = 0;

// Is the screen obstructed?
static bool s_screen_is_obstructed;

struct tm *tick_time;

// Save the settings to persistent storage
static void default_settings() {
  snprintf(settings.era, sizeof(settings.era), "%s", "random");
  snprintf(era, sizeof(era), "%s", settings.era);
  snprintf(settings.show_weekday_style, sizeof(settings.show_weekday_style), "%s", "FALSE");
}

// Save the settings to persistent storage
static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void send_settings_update_items(){
  // Begin dictionary
  DictionaryIterator *iter;

  // Start the sync to the js
  AppMessageResult result = app_message_outbox_begin(&iter);

  if(result == APP_MSG_OK) {

    // This is to pull the settings info from cache and push it to the index.js 
    dict_write_cstring(iter, MESSAGE_KEY_ERA, settings.era);
    dict_write_cstring(iter, MESSAGE_KEY_SHOWWEEKDAYSTYLE, settings.show_weekday_style);
    
    // Send this message
    result = app_message_outbox_send();

    // Check the resultW
    if(result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
    }

  } else {
    // The outbox cannot be used right now
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
  }
}

// get the saved settings from persistent storage
static void get_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void update_time() {

  // Get a tm structure
  time_t temp = time(NULL);
  tick_time = localtime(&temp);
 
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
 
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);

  // Display this date to the other TextLayer
  static char d_buffer[80];
  if ((strlen(settings.show_weekday_style)>0) && ((strstr(settings.show_weekday_style,"TRUE")) != NULL)){
    strftime(d_buffer, sizeof(d_buffer), "%A %d", tick_time);
  } else {
    strftime(d_buffer, sizeof(d_buffer), "%B %d", tick_time);
  }
  text_layer_set_text(s_date_layer, d_buffer);
}

static int era_tape_square_position(){
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "era_tape_square_position era: %s", era);
  if ((strstr(era,"six")) != NULL || (strstr(era,"seven")) != NULL ){
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "era_tape_square_position six or seven");
      return background_square_y_pos - 8;
  }
  
  if ((strstr(era,"eight")) != NULL){
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "era_tape_square_position eight");
    return background_square_y_pos - 4;
  }
  
  if ((strstr(era,"nine")) != NULL){
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "era_tape_square_position nine");
    return background_square_y_pos - 3;
  }

  return 1000;
}

static int era_tape_round_position(){
 
  if ((strstr(era,"six")) != NULL || (strstr(era,"seven")) != NULL){
    return background_round_y_pos -8;
  }

  if ((strstr(era,"eight")) != NULL){
    return background_round_y_pos - 4;
  }
  
  if ((strstr(era,"nine")) != NULL){
    return background_round_y_pos - 3;
  }

  return 1000;
}

// Event fires once, before the obstruction appears or disappears
static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area, void *context) {
  if (s_screen_is_obstructed) {
    // Obstructed
  } else {
    // Unobstructed
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,
    "Available screen area: width: %d, height: %d",
    final_unobstructed_screen_area.size.w,
    final_unobstructed_screen_area.size.h);
}

static void adjust_positions(){

  // Move the text layers
  GRect time_text_frame = layer_get_frame(text_layer_get_layer(s_time_layer));
  layer_set_frame(text_layer_get_layer(s_time_layer), time_text_frame);

  GRect date_text_frame = layer_get_frame(text_layer_get_layer(s_date_layer));
  layer_set_frame(text_layer_get_layer(s_date_layer), date_text_frame);

  GRect s_bitmap_frame = layer_get_frame(bitmap_layer_get_layer(s_bitmap_layer));
  GRect t_bitmap_frame = layer_get_frame(bitmap_layer_get_layer(t_bitmap_layer));

  int local_tape_pos = era_tape_square_position();
  
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "adjust_positions: %d", local_tape_pos);

  if (!s_screen_is_obstructed) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Not Obstructed");
    s_bitmap_frame.origin.y = PBL_IF_ROUND_ELSE(background_round_y_pos, background_square_y_pos);
    t_bitmap_frame.origin.y = PBL_IF_ROUND_ELSE(era_tape_round_position(), era_tape_square_position());
    time_text_frame.origin.y = PBL_IF_ROUND_ELSE(time_round_y_pos, time_square_y_pos);
    date_text_frame.origin.y = PBL_IF_ROUND_ELSE(date_round_y_pos, date_square_y_pos);

  } else {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Obstructed");
    
    // Size of Obstruction:
    GRect fullscreen = layer_get_bounds(s_window_layer);
    GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
    int16_t obstruction_height = fullscreen.size.h - unobstructed_bounds.size.h;

    s_bitmap_frame.origin.y =  PBL_IF_ROUND_ELSE(background_round_y_pos - obstruction_height, background_square_y_pos - obstruction_height);
    t_bitmap_frame.origin.y = PBL_IF_ROUND_ELSE(era_tape_round_position() - obstruction_height, era_tape_square_position() - obstruction_height );
    time_text_frame.origin.y = PBL_IF_ROUND_ELSE(time_round_y_pos - obstruction_height, time_square_y_pos - obstruction_height);
    date_text_frame.origin.y = PBL_IF_ROUND_ELSE(date_round_y_pos - obstruction_height, date_square_y_pos - obstruction_height);
  }
  layer_set_frame(bitmap_layer_get_layer(s_bitmap_layer), s_bitmap_frame);
  layer_set_frame(bitmap_layer_get_layer(t_bitmap_layer), t_bitmap_frame);
  layer_set_frame(text_layer_get_layer(s_time_layer), time_text_frame);
  layer_set_frame(text_layer_get_layer(s_date_layer), date_text_frame);
}

// Event fires frequently, while obstruction is appearing or disappearing
static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  adjust_positions();
}

// Event fires once, after obstruction appears or disappears
static void prv_unobstructed_did_change(void *context) {
  // Keep track if the screen is obstructed or not
  s_screen_is_obstructed = !s_screen_is_obstructed;

  if(s_screen_is_obstructed) {
    // text_layer_set_text(s_text_layer, "Obstructed!");
  } else {
    // text_layer_set_text(s_text_layer, "Unobstructed!");
  }
}

static void main_window_unload(Window *window) {
  
  // Unsubscribe from the unobstructed area service
  unobstructed_area_service_unsubscribe();

  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  //Destroy the BG
  gbitmap_destroy(s_bitmap);
  gbitmap_destroy(t_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
  bitmap_layer_destroy(t_bitmap_layer);
}

static int get_tape_depth(){

  APP_LOG(APP_LOG_LEVEL_DEBUG, "get_tape_depth hr: %d",  tick_time->tm_hour);
 
  switch(tick_time->tm_hour){
    case 1:
    case 2:
    case 13:
    case 14:
      return 1;
    case 3:
    case 4:
    case 15:
    case 16:
      return 2;
    case 5:
    case 6:
    case 17:
    case 18:
      return 3;
    case 7:
    case 8:
    case 19:
    case 20:
      return 4;
    case 9:
    case 10:
    case 21:
    case 22:
      return 5;
    case 11:
    case 12:
    case 23:
    case 24:
      return 6;
    default:
      return 3;
  }
}

static void get_tape_era(){
  int tape_depth = get_tape_depth();
  
  if ((strstr(era,"six")) != NULL){
      if (tape_depth == 1){
          t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_01_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_01_SIX_BW));
          return;
      }
      if (tape_depth == 2){
        t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_02_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_02_SIX_BW));
        return;
      }
      if (tape_depth == 3){
        t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_03_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_03_SIX_BW));
        return;
      }
      if (tape_depth == 4){
        t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_04_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_04_SIX_BW));
        return;
      }
      if (tape_depth == 5){
        t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_05_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_05_SIX_BW));
        return;
      }
      if (tape_depth == 6){
        t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_06_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_06_SIX_BW));
        return;
      }
  }

  if ((strstr(era,"seven")) != NULL){
    if (tape_depth == 1){
        t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_01_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_01_SEVEN_BW));
        return;
    }
    if (tape_depth == 2){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_02_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_02_SEVEN_BW));
      return;
    }
    if (tape_depth == 3){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_03_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_03_SEVEN_BW));
      return;
    }
    if (tape_depth == 4){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_04_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_04_SEVEN_BW));
      return;
    }
    if (tape_depth == 5){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_05_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_05_SEVEN_BW));
      return;
    }
    if (tape_depth == 6){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_06_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_06_SEVEN_BW));
      return;
    }
  }

  if ((strstr(era,"eight")) != NULL){
    if (tape_depth == 1){
        t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_01_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_01_EIGHT_BW));
        return;
    }
    if (tape_depth == 2){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_02_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_02_EIGHT_BW));
      return;
    }
    if (tape_depth == 3){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_03_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_03_EIGHT_BW));
      return;
    }
    if (tape_depth == 4){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_04_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_04_EIGHT_BW));
      return;
    }
    if (tape_depth == 5){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_05_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_05_EIGHT_BW));
      return;
    }
    if (tape_depth == 6){
      t_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_TAPE_06_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_TAPE_06_EIGHT_BW));
      return;
    }
  }

  if ((strstr(era,"nine")) != NULL){
    if (tape_depth == 1){
      t_bitmap = (gbitmap_create_with_resource(RESOURCE_ID_TAPE_01_NINE_BW));
      return;
    }
    if (tape_depth == 2){
      t_bitmap = (gbitmap_create_with_resource(RESOURCE_ID_TAPE_02_NINE_BW));
      return;
    }
    if (tape_depth == 3){
      t_bitmap = (gbitmap_create_with_resource(RESOURCE_ID_TAPE_03_NINE_BW));
      return;
    }
    if (tape_depth == 4){
      t_bitmap = (gbitmap_create_with_resource(RESOURCE_ID_TAPE_04_NINE_BW));
      return;
    }
    if (tape_depth == 5){
      t_bitmap = (gbitmap_create_with_resource(RESOURCE_ID_TAPE_05_NINE_BW));
      return;
    }
    if (tape_depth == 6){
      t_bitmap = (gbitmap_create_with_resource(RESOURCE_ID_TAPE_06_NINE_BW));
      return;
    }
  }
}

static void get_random_background(){

  switch (rand() % (9 - 6 + 1) + 6) {
    case 9:
      snprintf(era, sizeof(era), "%s", "nine");
      break;
    case 8:
      snprintf(era, sizeof(era), "%s", "eight");
      break;
    case 7:
      snprintf(era, sizeof(era), "%s", "seven");
      break;
    case 6:
      snprintf(era, sizeof(era), "%s", "six");
      break;
    default:
      snprintf(era, sizeof(era), "%s", "nine");
      break;
  }

}

static void set_background(){

  gbitmap_destroy(s_bitmap);
  gbitmap_destroy(t_bitmap);
  s_bitmap = NULL;
  t_bitmap = NULL;

  if ((strstr(settings.era,"random")) != NULL){ 
    get_random_background();
  } else {
    snprintf(era, sizeof(settings.era), "%s", settings.era);
  }
  
  if ((strstr(era,"nine")) != NULL){
    s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_NINE_COLOR), gbitmap_create_with_resource(RESOURCE_ID_NINE_BW));
  }  
  if ((strstr(era,"eight")) != NULL){  
    s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_EIGHT_BW));
  }  
  if ((strstr(era,"seven")) != NULL){  
    s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_SEVEN_BW));
  }  
  if ((strstr(era,"six")) != NULL){  
    s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_SIX_BW));
  }  

  get_tape_era();

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "set_background era: %s", era);

  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_bitmap(t_bitmap_layer, t_bitmap);
  
  adjust_positions();
}

static void tick_handler(struct tm *local_tick_time, TimeUnits units_changed) {

  update_time();
 // tick_time = local_tick_time;

  // Update the background every 30min if random was selected
  if ((tick_time->tm_min % 30 == 0)) {
    set_background();
  }
}

// BEGIN background shenanigans
static void get_style_setting(DictionaryIterator *iterator) {

  // Read tuples for data
  Tuple *era_tuple = dict_find(iterator, ERA);
  Tuple *show_weekday_tuple = dict_find(iterator, SHOWWEEKDAYSTYLE);

  if (era_tuple && strlen(era_tuple->value->cstring) > 0 ){
    snprintf(settings.era, sizeof(settings.era), "%s", era_tuple->value->cstring);
  }

  if (show_weekday_tuple && strstr(show_weekday_tuple->value->cstring, "") != NULL ){
    if ((strstr(show_weekday_tuple->value->cstring, "1") !=NULL) || (strstr(show_weekday_tuple->value->cstring, "TRUE") !=NULL)){
      snprintf(settings.show_weekday_style, sizeof(settings.show_weekday_style), "%s", "TRUE");
    } else {
      snprintf(settings.show_weekday_style, sizeof(settings.show_weekday_style), "%s", "FALSE");
    }
  }
  
  // We're done looking at the settings returned. let's save it for future use.
  save_settings();

  set_background();
  update_time();
}
 
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  // was this just a ready signal or a "GIVE ME INFO!" sign
 Tuple *ready_tuple = dict_find(iterator, READY);

  if (ready_tuple) {
    
    // This is just a ready signal, We'll go back to the JS program
    // and give it the settings information
    send_settings_update_items();
    
    set_background();
  } else {
    // otherwise, this is just a syle update for you
    get_style_setting(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback Message dropped! Reason: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  // The message just sent failed to be delivered
  APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback Message send failed. Reason: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback Outbox send success!");
}
// END Style shenanigans

static void main_window_load(Window *window) {
  // Get information about the Window

  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  //Destroy the BG
  gbitmap_destroy(s_bitmap);
  gbitmap_destroy(t_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
  bitmap_layer_destroy(t_bitmap_layer);

  // In case it's already obstructed
  s_window_layer = window_get_root_layer(window);
  GRect fullscreen = layer_get_bounds(s_window_layer);
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(s_window_layer);
  int16_t obstruction_height = fullscreen.size.h - unobstructed_bounds.size.h;


  // Determine if the screen is obstructed when the app starts
  s_screen_is_obstructed = grect_equal(&fullscreen, &unobstructed_bounds);
  
  if (!s_screen_is_obstructed) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Not Obstructed");
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Obstructed");
  }

  //////////////BASIC LAYER////////////////////
  // Create the canvas Layer
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOADING);
  t_bitmap = gbitmap_create_with_resource(RESOURCE_ID_TAPE_LOADING);
  
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_bitmap(t_bitmap_layer, t_bitmap);
  
  s_bitmap_layer = bitmap_layer_create(GRect(0, PBL_IF_ROUND_ELSE(background_round_y_pos - obstruction_height, background_square_y_pos - obstruction_height), fullscreen.size.w, fullscreen.size.h));
  t_bitmap_layer = bitmap_layer_create(GRect(0, PBL_IF_ROUND_ELSE(0, 0), fullscreen.size.w, fullscreen.size.h));
      
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  bitmap_layer_set_compositing_mode(t_bitmap_layer, GCompOpSet);
   
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_bitmap(t_bitmap_layer, t_bitmap);

  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(t_bitmap_layer));

  //////////////TIME LAYER////////////////////
  // Create the TextLayer with specific bounds 
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(time_round_y_pos - obstruction_height, time_square_y_pos - obstruction_height), fullscreen.size.w, fullscreen.size.h));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, PBL_IF_ROUND_ELSE(fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS), fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49)));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));
   
  //////////////DATE LAYER////////////////////
  // Create the TextLayer with specific bounds 
  s_date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(date_round_y_pos - obstruction_height, date_square_y_pos - obstruction_height), fullscreen.size.w, fullscreen.size.h));

  // Improve the layout for humans
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_font(s_date_layer, PBL_IF_ROUND_ELSE(fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21)));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
  
  // if (s_screen_is_obstructed){
  //   layer_set_hidden(text_layer_get_layer(s_date_layer), true);
  // } else {
  //   layer_set_hidden(text_layer_get_layer(s_date_layer), false);
  // }

  // Subscribe to the unobstructed area events
  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change,
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);

  // Make sure the time is displayed from the start
  update_time();

  // update bg
  set_background();
}

static void init() {

  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
 
  // Get any saved storage
  get_settings();
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, 1);
  window_set_background_color(s_main_window, GColorWhite);

   // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // According to Pebble Doc, it is best practice to register the AppMessage callbacks
  // before opening it. So the registration is under INIT()
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);

}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}