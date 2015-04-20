//56b37c6f-792a-480f-b962-9a0db8c32aa4
//58ed427c-0a4f-405f-9de0-37417179023c
/*
Save settings upon exit
Restore settings upon start, except as defined by settings

Start ->
Back = Exit
    Up = BPM Mode
Select =

tried the sample tap ap here, but it sucks hardcore
http://developer.getpebble.com/guides/pebble-apps/sensors/accelerometer/


*/
#include "pebble.h"

static Window *main_window;
static Layer *graphics_layer;
bool pause=false;
uint8_t offset=0;

int16_t z[256];
int16_t graph[256];


int32_t abs32(int32_t x) {return (x ^ (x >> 31)) - (x >> 31);}
int16_t abs16(int16_t x) {return (x ^ (x >> 15)) - (x >> 15);}
// ------------------------------------------------------------------------ //
//  Timer Functions
// ------------------------------------------------------------------------ //
#define UPDATE_MS 50 // Refresh rate in milliseconds
static void timer_callback(void *data) {
  layer_mark_dirty(graphics_layer);                    // Schedule redraw of screen
  app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
}

void accel_data_handler(AccelData *data, uint32_t num_samples) {
if(!pause)
  for(uint32_t i=0; i<num_samples; i++) {
    //uint64_t timestamp
    //if(data[i].did_vibrate)
    //  data[i].timestamp
    z[offset]=data[i].z>>3;
    offset++;
  }
}

// timestamp
// not vibrate
// call function when tap is detected

// ------------------------------------------------------------------------ //
//  Button Functions
// ------------------------------------------------------------------------ //
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  vibes_long_pulse();
}
void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  vibes_short_pulse();
}
void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  pause=!pause;
}
  
void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}
  
// ------------------------------------------------------------------------ //
//  Drawing Functions
// ------------------------------------------------------------------------ //
int16_t Q1=0;

void draw_textbox(GContext *ctx, int16_t x, int16_t y, int16_t number) {
  static char text[40];  //Buffer to hold text
  graphics_context_set_text_color(ctx, GColorWhite);   // Text Color
  graphics_context_set_fill_color(ctx, GColorBlack);   // BG Color
  graphics_context_set_stroke_color(ctx, GColorWhite); // Line Color
  int16_t w;
  if(number<0) {
    if(number>-10) w=20;
    else if(number>-100) w=28;
    else if(number>-1000) w=36;
    else w=44;
  } else {
    if(number<10) w=12;
    else if(number<100) w=20;
    else if(number<1000) w=28;
    else w=36;
  }
  
  GRect textframe = GRect(x, y, w, 13);  // Text Box Position and Size: x, y, w, h
  graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
  snprintf(text, sizeof(text), "%d", number);  // What text to draw
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(x, y-3, w, 17), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text
}

static void graphics_layer_update(Layer *me, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite); // Line Color
  
  for(uint8_t i=0; i<255; i++)
    graph[i] = z[(i+offset+(256-144))%256];  // cleanup graph
  
  // calculate average line
  int32_t avg=0;
  for(int16_t i=0; i<256; i++)
    avg+=graph[i];
  avg/=256;
  graphics_draw_line(ctx, GPoint(0, 100), GPoint(144,100));
  
  // graph
  for(int16_t i=1; i<144; i++)
    graphics_draw_line(ctx, GPoint(i-1, (graph[i-1]-avg)/1+100), GPoint(i, (graph[i]-avg)/1+100));
  
  // timestamp initial
  // then calculate length
  // then after done, wait till next
  // only backlog as long as needed
  // timestamp everything else
  bool tapping; tapping=false;
  int16_t gap;
  for(int16_t i=0; i<143; i++) {
//     graphics_draw_line(ctx, GPoint(i, (graph[i]-avg)/2+100), GPoint(i+1, (graph[i+1]-avg)/2+100));
    if(tapping) {
      if(abs16(graph[i]-graph[i+1]) < 12) {  // subtle vibrations, but tap might still be going
        gap++;
        if(gap>5) {      // too many subtle vibrations, tap done
          Q1=i-Q1-gap; // Q1 now = point where tap ended - point were tap started = length of vibrations
          draw_textbox(ctx, i, 26, Q1);
          gap=0;
          tapping=false;
        }
      } else { // still major vibrations, tap still going on
        gap=0;
      }
    } else {
      if(abs16(graph[i]-graph[i+1]) >= 20) {   // tap begins! sensitivity=20
        Q1=abs16(graph[i]-graph[i+1]);
        draw_textbox(ctx, i, 6, Q1);
        Q1=i;  // Q1 = point where tap started
        tapping=true;
        gap=0;
      }
    }

    if(tapping)
      graphics_draw_line(ctx, GPoint(i, 0), GPoint(i,4));
  }
  
  draw_textbox(ctx, 0, 150, offset);
  
}

// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  graphics_layer = layer_create(layer_get_frame(window_layer));
  layer_set_update_proc(graphics_layer, graphics_layer_update);
  layer_add_child(window_layer, graphics_layer);
  timer_callback(NULL);
}

static void main_window_unload(Window *window) {
  layer_destroy(graphics_layer);
}

static void init(void) {
  // Set up and push main window
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(main_window, click_config_provider);
  window_set_background_color(main_window, GColorBlack);
  window_set_fullscreen(main_window, true);

  //Set up other stuff
  srand(time(NULL));  // Seed randomizer
  accel_data_service_subscribe(5, accel_data_handler);  // We will be using the accelerometer: 1 sample_per_update
  accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);  // This seems to break it ... maybe it doesnt...
  
  //Begin
  window_stack_push(main_window, true /* Animated */); // Display window.  Callback will be called once layer is dirtied then written
}
  
static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
