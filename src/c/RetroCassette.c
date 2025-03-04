#include <pebble.h>
#include <string.h>
  
#define READY 1
#define ERA 2
#define TRUE 1
#define FALSE 0

// Persistent storage key
#define SETTINGS_KEY 1

// Define our settings struct
typedef struct ClaySettings {
  char era[25];
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

static Window *s_main_window;
static TextLayer *s_time_layer;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;

// I'm making this global so that I can always get the value
bool date;
int tick_counter;


// Save the settings to persistent storage
static void default_settings() {

 // snprintf(settings.era, sizeof(settings.era), "%s", "nine");
  APP_LOG(APP_LOG_LEVEL_ERROR, "default settings.era: %s", settings.era);
}

// Save the settings to persistent storage
static void save_settings() {
  
  APP_LOG(APP_LOG_LEVEL_ERROR, "save_settings settings.era: %s", settings.era);
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// get the saved settings from persistent storage
static void get_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}
static void send_settings_update_items(){
  // Begin dictionary
  DictionaryIterator *iter;

  // Start the sync to the js
  AppMessageResult result = app_message_outbox_begin(&iter);

  if(result == APP_MSG_OK) {

    // This is to pull the settings info from cache and push it to the index.js 
    dict_write_cstring(iter, MESSAGE_KEY_ERA, settings.era);
   
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
static void update_time() {

  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
 
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
 
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);

}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //////////////BASIC LAYER////////////////////
  // Create the canvas Layer
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOADING);
  
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  s_bitmap_layer = bitmap_layer_create(GRect(0,0, bounds.size.w, bounds.size.h));
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
   
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);

  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer));

  //////////////TIME LAYER////////////////////
  // Create the TextLayer with specific bounds 
  // s_time_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(0, -20),PBL_IF_ROUND_ELSE(125, 125), bounds.size.w, bounds.size.h));
  s_time_layer = text_layer_create(GRect(0 ,110, bounds.size.w, bounds.size.h));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
   
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  //Destroy the BG
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {

  update_time();
 
}

// BEGIN background shenanigans
static void update_background_bmp(DictionaryIterator *iterator) {
  
  // Read tuples for data
  Tuple *era_tuple = dict_find(iterator, ERA);

  if (era_tuple && strlen(era_tuple->value->cstring) > 0 ){
    APP_LOG(APP_LOG_LEVEL_ERROR, "update_background_bmp era_tuple: %s", era_tuple->value->cstring);
    snprintf(settings.era, sizeof(settings.era), "%s", era_tuple->value->cstring);
  }
 
  // We're done looking at the settings returned. let's save it for future use.
  //HEL TO DO, does this get over written when nothing is returned for those values?
  save_settings();

  APP_LOG(APP_LOG_LEVEL_ERROR, "update_background_bmp settings.era: %s", settings.era);
  APP_LOG(APP_LOG_LEVEL_ERROR, "update_background_bmp settings.era strlen: %d", strlen(settings.era));
  if (strlen(settings.era)>0){
    if ((strstr(settings.era,"nine")) != NULL){  
      s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_NINE_COLOR), gbitmap_create_with_resource(RESOURCE_ID_NINE_BW));
    }  
    if ((strstr(settings.era,"eight")) != NULL){  
      s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_EIGHT_COLOR), gbitmap_create_with_resource(RESOURCE_ID_EIGHT_BW));
    }  
    if ((strstr(settings.era,"seven")) != NULL){  
      s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_SEVEN_COLOR), gbitmap_create_with_resource(RESOURCE_ID_SEVEN_BW));
    }  
    if ((strstr(settings.era,"six")) != NULL){  
      s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_SIX_COLOR), gbitmap_create_with_resource(RESOURCE_ID_SIX_BW));
    }  
  } else {
    s_bitmap = PBL_IF_COLOR_ELSE(gbitmap_create_with_resource(RESOURCE_ID_NINE_COLOR), gbitmap_create_with_resource(RESOURCE_ID_NINE_BW));
  }
  
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);

}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  // was this just a ready signal or a "GIVE ME WEATHER I"M HUNGRY NOW!" sign
  Tuple *ready_tuple = dict_find(iterator, READY);

  if (ready_tuple) {
    
    // This is just a ready signal, We'll go back to the JS program
    // and give it the API information

    send_settings_update_items();
  } else {
    // otherwise, this is just a syle update for you
    update_background_bmp(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback Message dropped! Reason: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iter,
                                      AppMessageResult reason, void *context) {
  // The message just sent failed to be delivered
  APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback Message send failed. Reason: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback Outbox send success!");
}
// END Style shenanigans

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
  window_stack_push(s_main_window, true);
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

  date = FALSE;

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