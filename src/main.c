//56b37c6f-792a-480f-b962-9a0db8c32aa4
//58ed427c-0a4f-405f-9de0-37417179023c
/*
Save settings upon exit
Restore settings upon start, except as defined by settings

Start ->
Back = Exit
    Up = BPM Mode
Select =




*/
#include "pebble.h"
#define UPDATE_MS 50 // Refresh rate in milliseconds

static Window *main_window;
static Layer *graphics_layer;

int32_t vdir=0, vacc=0, r=0;
int16_t offset=0;

int16_t z[144];
int16_t graph[144];
int16_t runavg[144];


int32_t abs32(int32_t x) {return (x ^ (x >> 31)) - (x >> 31);}
int16_t abs16(int16_t x) {return (x ^ (x >> 15)) - (x >> 15);}
// ------------------------------------------------------------------------ //
//  Timer Functions
// ------------------------------------------------------------------------ //
static void timer_callback(void *data) {
  layer_mark_dirty(graphics_layer);                    // Schedule redraw of screen
  app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
}

void accel_data_handler(AccelData *data, uint32_t num_samples) {
  for(uint32_t i=0; i<num_samples; i++) {
    //graph.z[offset]=abs16(data[i].z)>>3;
    z[offset]=data[i].z>>3;
    offset=(offset+1)%144;
  }
}



// ------------------------------------------------------------------------ //
//  Button Functions
// ------------------------------------------------------------------------ //
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  vibes_long_pulse();
}
void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  vibes_short_pulse();
}
void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {}
  
void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}
  
// ------------------------------------------------------------------------ //
//  Drawing Functions
// ------------------------------------------------------------------------ //
int16_t Q1=0;

static void graphics_layer_update(Layer *me, GContext *ctx) {
  GRect textframe;
  static char text[40];  //Buffer to hold text
  graphics_context_set_text_color(ctx, GColorWhite);  // Text Color
  graphics_context_set_fill_color(ctx, GColorBlack);  // BG Color
  
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for(int16_t i=0; i<144; i++) graph[i] = z[(i+offset)%144];  // cleanup graph
  
  // calculate average line
  int16_t avg=0;
  for(int16_t i=0; i<144; i++)
    avg+=graph[i];
  avg/=144;
  graphics_draw_line(ctx, GPoint(0, 100), GPoint(144,100));
  
  // graph
  for(int16_t i=1; i<144; i++)
    graphics_draw_line(ctx, GPoint(i-1, (graph[i-1]-avg)/2+100), GPoint(i, (graph[i]-avg)/2+100));

  // initial tap
//   for(int16_t i=0; i<143; i++)
//     if(abs16(graph[i]-graph[i+1]) >= 30)
//       graphics_draw_line(ctx, GPoint(i, 0), GPoint(i,4));
  
  // timestamp initial
  // then calculate length
  // then after done, wait till next
  // only backlog as long as needed
  // timestamp everything else
  Q1=0;
  bool tapping=false;
  int16_t gap, vibavg, count;
  for(int16_t i=0; i<143; i++) {
    if(tapping) {
      
      if(abs16(graph[i]-graph[i+1]) < 10) {  // subtle vibrations, but tap might still be going
        gap++;
        if(gap>3) {      // tap done
          Q1=i-Q1; // display length
          vibavg = vibavg/count;
          vibavg = (vibavg-avg)/2 + 100;
          //graphics_draw_line(ctx, GPoint(i-gap, vibavg), GPoint(i, vibavg));
          
  textframe = GRect(i, 6, 20, 20);  // Text Box Position and Size: x, y, w, h
  snprintf(text, sizeof(text), "%d", (int)Q1);  // What text to draw
  graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text

          gap=0;
          tapping=false;
        } else {
          vibavg += graph[i];
          count++;
        }
      } else { // still major vibrations going on
        gap=0;
        vibavg += graph[i];
        count++;
      }
    } else {
      if(abs16(graph[i]-graph[i+1]) >= 20){   // tap begins! sensitivity=20
        Q1=i;
        tapping=true;
        vibavg=graph[i];
        count=1;
      }
    }
    // initial tap detected
    if(tapping)
      graphics_draw_line(ctx, GPoint(i, 0), GPoint(i,4));
  }
  if(tapping) Q1 = 143-Q1;
  

//   textframe = GRect(0, 123, 30, 20);  // Text Box Position and Size: x, y, w, h
//   snprintf(text, sizeof(text), "%d", (int)Q1);  // What text to draw
//   graphics_context_set_fill_color(ctx, 0); graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
//   graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
//   graphics_context_set_text_color(ctx, 1);  // Text Color
//   graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
  
}

static void graphics_layer_update2(Layer *me, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  
  
  for(int16_t i=0; i<144; i++) graph[i] = z[(i+offset)%144];
  
  // Running Average
  int16_t avg, count;
  #define runavgwidth 16
  for(int16_t i=0; i<144; i++) {
    avg=0; count=0;
    for(int16_t j=-runavgwidth; j<runavgwidth; j++)
      if(j+i>=0 && j+i<=143) {
        avg += graph[i+j];
        count++;
      }
    runavg[i] = avg/count;
  
    
    { //remove outliers
    avg=0; count=0;
    for(int16_t j=-runavgwidth; j<runavgwidth; j++)
      if(j+i>=0 && j+i<=143)
        if(abs16(graph[i+j]-runavg[i])<30) {
          avg += graph[i+j];
          count++;
        }
    if(count>0)
      runavg[i] = avg/count;
    else
      runavg[i] = graph[i];
    }
    
  }
  
    
  for(int16_t i=0; i<144; i++) graph[i] = ((graph[i]-runavg[i]));
  
  //Normalize graph
//   int32_t normalize=0;
//   for(int16_t i=144-32; i<144; i++)
//     normalize = normalize + (graph[i]);
//   normalize >>= 5;

//   int32_t normalize2=0, count=0;
//   for(int16_t i=144-32; i<144; i++)
//     if(abs16(graph[i]-normalize)<30) {
//       normalize2 = normalize2 + (graph[i]);
//       count++;
//   }
//   if(count>0)
//     normalize2 /= count;
//   else
//     normalize2 = graph[143];
  
//   normalize=normalize2;
  
//   for(int16_t i=0; i<144; i++)
//     graph[i] = ((graph[i]-normalize)>>2);

  
  { // draw graph
    for(int16_t i=1; i<144; i++)  graphics_draw_line(ctx, GPoint(i-1, graph[i-1]<(20-84) ? 20 : graph[i-1]+84), GPoint(i, graph[i]<(20-84) ? 20 : graph[i]+84));
    graphics_draw_line(ctx, GPoint(0, 84), GPoint(143, 84));  // draw normalize line
    //graphics_draw_line(ctx, GPoint(144-32, 8), GPoint(144, 8));   // top line
  }
  

  for(int16_t i=0; i<143; i++)
    if(abs16(graph[i])>10)
      graphics_draw_line(ctx, GPoint(i, 0), GPoint(i,4));

  int16_t total;
  for(int16_t i=7; i<144; i++) {
    total = 0;
    for(int16_t j=-7; j<=0; j++) {
      total+=abs16(graph[i+j]);
    }
    graphics_draw_line(ctx, GPoint(i, 168), GPoint(i, 168-(total/8)));
  }
  
  // once big jump happens or 
    
  // tap detect line
//   for(int16_t i=0; i<143; i++)
//     if(abs16(z[(i+offset)%144]-z[(i+offset+1)%144]) >= 30)
//       graphics_draw_line(ctx, GPoint(i, 0), GPoint(i,4));

//   uint8_t tap=0;
//   #define diff   4
//   #define tapmax 3
//   for(int16_t i=diff; i<(143-diff); i++) {
//     tap = 0;
//     for(int16_t j=-diff; j<diff; j++)
//       if(abs16(z[(i+offset+j)%144]-z[(i+offset+j+1)%144]) > 30) tap++;
//     if(tap>=tapmax)
//       graphics_draw_line(ctx, GPoint(i, 6), GPoint(i,7));  
//   }
    
  
  
  //for(int16_t i=0; i<144; i++) graph[i] = z[(i+offset)%144] - normalize;
  
//   int16_t quietwidth=0, loudwidth=0, tempavg;
//   //int16_t currentmode=0;  // quiet
//   for(int16_t i=0; i<144; i++) {
//     //tempavg=(graph[i]+graph[i-1]+graph[i-3]+graph[i-4])>>2;
    
//     if(abs16(graph[i])>30)
//       graphics_draw_pixel(ctx, GPoint(i,10));

//      quietwidth++;
//      loudwidth++;
//      if(abs16(graph[i])<30) {
//        if(quietwidth>2) loudwidth=0;
//      } else {
//        if(loudwidth>0)  quietwidth=0;
//      }
//     if(loudwidth>1) graphics_draw_pixel(ctx, GPoint(i-1,12));
    
// //     if(abs16(graph[i] - tempavg)<10) {
// //       quietwidth++;
// //       if(quietwidth>2)
// //         loudwidth=0;
// //     } else {
// //       loudwidth++;
// //       if(loudwidth>1) {
// //         quietwidth=0;
// //         graphics_draw_pixel(ctx, GPoint(i,loudwidth+9));
// //       }
// //     }

      
//   }
  
    

  static char text[40];  //Buffer to hold text
  GRect textframe = GRect(0, 123, 30, 20);  // Text Box Position and Size: x, y, w, h
  snprintf(text, sizeof(text), "%+03d", (int)graph[143]);  // What text to draw
  graphics_context_set_fill_color(ctx, 0); graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
  graphics_context_set_text_color(ctx, 1);  // Text Color
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
  
  
  
  
  //graphics_draw_line(ctx, GPoint(0, 0), GPoint(rand()%10, rand()%10));
  
  //graphics_context_set_stroke_color(ctx, GColorBlack);
  //for(int16_t i=0; i<5; i++) graphics_draw_line(ctx, GPoint(0, i), GPoint(143,i));
/*  
  uint32_t lastpoint, count, bpm;
  lastpoint=1000; count=0; bpm=0;
  
  graphics_context_set_stroke_color(ctx, 1);
  for(int16_t i=0; i<143; i++){
    if(abs16(graph.z[(i+offset)%144]-graph.z[(i+offset+1)%144]) > 60) {
      if(lastpoint==1000) {
        lastpoint=i;
        count=1;
        graphics_draw_pixel(ctx, GPoint(i, 0));
      }
      if(i-lastpoint>10) {
        bpm = bpm + (i-lastpoint);
        count++;
        graphics_draw_pixel(ctx, GPoint(i, 0)); 
      }
    } 

    //if(abs16(graph.z[(i+offset)%144]-graph.z[(i+offset+1)%144]) > 40) graphics_draw_pixel(ctx, GPoint(i, 1));
    //if(abs16(graph.z[(i+offset)%144]-graph.z[(i+offset+1)%144]) > 60) graphics_draw_pixel(ctx, GPoint(i, 2));
    //if(abs16(graph.z[(i+offset)%144]-graph.z[(i+offset+1)%144]) > 100) graphics_draw_pixel(ctx, GPoint(i, 3));
    
  }
  bpm=bpm/count;
  static char text[40];  //Buffer to hold text
  snprintf(text, sizeof(text), "%ld bpm", 6000/bpm);  // What text to draw
  graphics_context_set_text_color(ctx, 1);  // Text Color
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0, 0, 143, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
*/  
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
  //accel_data_service_subscribe(0, NULL);  // We will be using the accelerometer
  accel_data_service_subscribe(5, accel_data_handler);  // We will be using the accelerometer: 1 sample_per_update
  //accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  //accel_service_set_sampling_rate(ACCEL_SAMPLING_50HZ);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);  // This seems to break it ... maybe it doesnt...
  
  //accel_tap_service_subscribe(Accel_Tap);
  
  //Begin
  window_stack_push(main_window, true /* Animated */); // Display window.  Callback will be called once layer is dirtied then written
}
  
static void deinit(void) {
  accel_data_service_unsubscribe();
  //accel_tap_service_unsubscribe();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

// ------------------------------------------------------------------------ //
//  Notes
// ------------------------------------------------------------------------ //
/*
#define FONT_KEY_FONT_FALLBACK "RESOURCE_ID_FONT_FALLBACK"
#define FONT_KEY_GOTHIC_14 "RESOURCE_ID_GOTHIC_14"
#define FONT_KEY_GOTHIC_14_BOLD "RESOURCE_ID_GOTHIC_14_BOLD"
#define FONT_KEY_GOTHIC_18 "RESOURCE_ID_GOTHIC_18"
#define FONT_KEY_GOTHIC_18_BOLD "RESOURCE_ID_GOTHIC_18_BOLD"
#define FONT_KEY_GOTHIC_24 "RESOURCE_ID_GOTHIC_24"
#define FONT_KEY_GOTHIC_24_BOLD "RESOURCE_ID_GOTHIC_24_BOLD"
#define FONT_KEY_GOTHIC_28 "RESOURCE_ID_GOTHIC_28"
#define FONT_KEY_GOTHIC_28_BOLD "RESOURCE_ID_GOTHIC_28_BOLD"
#define FONT_KEY_BITHAM_30_BLACK "RESOURCE_ID_BITHAM_30_BLACK"
#define FONT_KEY_BITHAM_42_BOLD "RESOURCE_ID_BITHAM_42_BOLD"
#define FONT_KEY_BITHAM_42_LIGHT "RESOURCE_ID_BITHAM_42_LIGHT"
#define FONT_KEY_BITHAM_42_MEDIUM_NUMBERS "RESOURCE_ID_BITHAM_42_MEDIUM_NUMBERS"
#define FONT_KEY_BITHAM_34_MEDIUM_NUMBERS "RESOURCE_ID_BITHAM_34_MEDIUM_NUMBERS"
#define FONT_KEY_BITHAM_34_LIGHT_SUBSET "RESOURCE_ID_BITHAM_34_LIGHT_SUBSET"
#define FONT_KEY_BITHAM_18_LIGHT_SUBSET "RESOURCE_ID_BITHAM_18_LIGHT_SUBSET"
#define FONT_KEY_ROBOTO_CONDENSED_21 "RESOURCE_ID_ROBOTO_CONDENSED_21"
#define FONT_KEY_ROBOTO_BOLD_SUBSET_49 "RESOURCE_ID_ROBOTO_BOLD_SUBSET_49"
#define FONT_KEY_DROID_SERIF_28_BOLD "RESOURCE_ID_DROID_SERIF_28_BOLD"





*/