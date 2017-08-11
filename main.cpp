/*
*
*  Sample Values Client
*
*/
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "../../nuklear.h"
#include "nuklear_sdl_gl3.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define PLOT_SAMPLE_SIZE 100

#include <signal.h>
#include <stdio.h>
#include "hal_thread.h"
#include "sv_subscriber.h"
#include "vector"
#include <iostream>
#include <string>
#include <sstream>
#include <math.h>
#include <iomanip>

using namespace std;


enum {INT_, FLOAT_};

// Struct that hold info abous sample value channels
struct sv_channel {
  const char* name;
  vector<int> int_values;
  vector<float> float_values;
  int dataType;
  bool visible;
};


////////////////////////////////
//// Variables declaration ////
//////////////////////////////
static bool running = true;
struct nk_color background;
static int win_width, win_height;
static SDL_Window *win;
static SVReceiver receiver;
struct nk_context *ctx;
static vector<sv_channel> channels;
static bool advanced = false;
static sv_channel *channel_advanced;
static int advancedMenuOp = 0;


// test
static float plot_arr_float[PLOT_SAMPLE_SIZE];
static float plot_arr_int[PLOT_SAMPLE_SIZE];
static int plot_count = 0;
static bool plot_sampling = false;


////////////////////////////////
//// Functions declaration ////
//////////////////////////////
static void cleanup();
static void sv_client_init();
static int fingChannelByName(const char* name);
static void sigint_handler(int signalId);
static float getSVFloat(SVClientASDU asdu);
static int getSVInt(SVClientASDU asdu);
static void gui_init();
static string intToString(int number);
static void leaveEmptySpace(int height);
static void clearIntSample();
static void clearFloatSample();
static float rms_float();
static float rms_int();
static string floatToString(float number);


static void addChannelSim(){
  sv_channel sv;
  sv.name = "random";
  sv.dataType = FLOAT_;
  sv.float_values.push_back(0.1);
  sv.float_values.push_back(0.02);
  sv.visible = false;
  channels.push_back(sv);
}


int main(int argc, char** argv) {
  gui_init();
  sv_client_init();

  addChannelSim();
  while(running) {
    /* Input */
    SDL_Event evt;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt)) {
      if (evt.type == SDL_QUIT) cleanup();
      nk_sdl_handle_event(&evt);
    }
    nk_input_end(ctx);


    /* GUI */
    if (nk_begin(ctx, "Sample Values Client", nk_rect(0, 0, 800, 600),NK_WINDOW_BORDER|NK_WINDOW_SCALABLE|NK_WINDOW_TITLE)) {
      if(advanced){

        nk_layout_row_dynamic(ctx,10,1);
        nk_label(ctx, "---------- ADVANCED ----------", NK_TEXT_CENTERED);
        int op = channel_advanced->dataType;
        nk_layout_row_dynamic(ctx, 30, 12);
        if (nk_option_label(ctx, "float", op == FLOAT_)) op = FLOAT_;
        if (nk_option_label(ctx, "int", op == INT_)) op = INT_;
        channel_advanced->dataType = op;
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_button_label(ctx,channel_advanced->name);
        if(channel_advanced->dataType == FLOAT_){
          for(int j = 0; j < channel_advanced->float_values.size(); j++){
            nk_property_float(ctx, ("Value " + intToString(j+1)).c_str(), channel_advanced->float_values[j], &(channel_advanced->float_values[j]),channel_advanced->float_values[j], 10, 1);
          }
        }
        else {
          for(int j = 0; j < channel_advanced->int_values.size(); j++){
            nk_property_int(ctx, ("Value " + intToString(j+1)).c_str(), channel_advanced->int_values[j], &(channel_advanced->int_values[j]),channel_advanced->int_values[j], 10, 1);
          }
        }
        leaveEmptySpace(30);

        int optionsCount;
        if(channel_advanced->dataType == FLOAT_) optionsCount = channel_advanced->float_values.size();
        else optionsCount = channel_advanced->int_values.size();



        nk_layout_row_static(ctx, 30, 80, optionsCount + 2);
        if (nk_menu_begin_label(ctx, "PLOT", NK_TEXT_LEFT, nk_vec2(120, 200))) {
                nk_layout_row_dynamic(ctx, 30, 1);
                if(nk_menu_item_label(ctx, "START", NK_TEXT_LEFT)) plot_sampling = true;
                if(nk_menu_item_label(ctx, "STOP", NK_TEXT_LEFT)) plot_sampling = false;
                nk_menu_end(ctx);
        }

        for(int s = 0; s < optionsCount; s++){
          if(nk_option_label(ctx, ("Value " + intToString(s+1)).c_str(), advancedMenuOp == s)) advancedMenuOp = s;
        }

        nk_layout_row_static(ctx,200, 800, 1);
        if(channel_advanced->dataType == FLOAT_) nk_plot(ctx,NK_CHART_LINES,plot_arr_float,PLOT_SAMPLE_SIZE,0.1);
        else nk_plot(ctx,NK_CHART_LINES,plot_arr_int,PLOT_SAMPLE_SIZE,0.1);

        if(plot_sampling){
          nk_layout_row_dynamic(ctx,30,1);
          if(channel_advanced->dataType == FLOAT_) nk_label(ctx, ("RMS Value:  " + floatToString(rms_float())).c_str(), NK_TEXT_LEFT);
          else nk_label(ctx, ("RMS Value:  " + floatToString(rms_int())).c_str(), NK_TEXT_LEFT);
        }

        leaveEmptySpace(50);

        nk_layout_row_static(ctx, 30, 80, 1);
        if(nk_button_label(ctx,"BACK")) {
          advanced = false;
          plot_sampling = false;
          clearIntSample();
          clearFloatSample();
        }
      } else {
      nk_layout_row_dynamic(ctx,10,1);
      nk_label(ctx, "---------- CHANNELS ----------", NK_TEXT_CENTERED);

      for(int i = 0; i < channels.size();i++){
        int op = channels[i].dataType;
        nk_layout_row_dynamic(ctx, 30, 12);
        if (nk_option_label(ctx, "float", op == FLOAT_)) op = FLOAT_;
        if (nk_option_label(ctx, "int", op == INT_)) op = INT_;
        channels[i].dataType = op;
        bool visible = channels[i].visible;
        nk_layout_row_dynamic(ctx, 25, 1);
        if(nk_button_label(ctx,channels[i].name)){
          if(visible) channels[i].visible = false;
          else channels[i].visible = true;
        }
        if(channels[i].visible){
          if(channels[i].dataType == FLOAT_){
            for(int j = 0; j < channels[i].float_values.size(); j++){
              nk_property_float(ctx, ("Value " + intToString(j+1)).c_str(), channels[i].float_values[j], &channels[i].float_values[j],channels[i].float_values[j], 10, 1);
            }
          }
          else {
            for(int j = 0; j < channels[i].int_values.size(); j++){
              nk_property_int(ctx, ("Value " + intToString(j+1)).c_str(), channels[i].int_values[j], &channels[i].int_values[j],channels[i].int_values[j], 10, 1);
            }
          }
          nk_layout_row_static(ctx, 30, 80, 1);
          if(nk_button_label(ctx,"ADVANCED")) {
            advanced = true;
            channel_advanced = &channels[i];
          }
        }
      }
      if(channels.size() > 0){
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "REFRESH")) channels.clear();
      } else {
        nk_layout_row_dynamic(ctx,10,1);
        nk_label(ctx, "NO AVAILABLE CHANNELS", NK_TEXT_CENTERED);
        nk_label(ctx, "Please check your connection", NK_TEXT_CENTERED);
      }
    }
  }
    nk_end(ctx);

    /* Draw */
    {float bg[4];
      nk_color_fv(bg, background);
      SDL_GetWindowSize(win, &win_width, &win_height);
      glViewport(0, 0, win_width, win_height);
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(bg[0], bg[1], bg[2], bg[3]);
      nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);}

      SDL_GL_SwapWindow(win);
    }
    return 0;
  }

  static float rms_int(){
    long sum = 0;
    for(int i = 0; i < PLOT_SAMPLE_SIZE; i++){
      sum += plot_arr_int[i] * plot_arr_int[i];
    }
    return sqrt(sum/(float)PLOT_SAMPLE_SIZE);
  }

  static float rms_float(){
    float sum = 0;
    for(int i = 0; i < PLOT_SAMPLE_SIZE; i++){
      sum += plot_arr_float[i] * plot_arr_float[i];
    }
    return sqrt(sum/PLOT_SAMPLE_SIZE);
  }

  static void cleanup(){
    /* Stop listening to SV messages */
    SVReceiver_stop(receiver);

    /* Cleanup and free resources */
    SVReceiver_destroy(receiver);

    running = false;
  }

  static string intToString(int number){
    stringstream ss;
    ss << number;
    return ss.str();
  }

  static string floatToString(float number){
    stringstream ss;
    ss <<fixed<<setprecision(4)<< number;
    return ss.str();
  }

  static void leaveEmptySpace(int height){
    nk_layout_row_dynamic(ctx,height,1);
    nk_label(ctx,"",NK_TEXT_LEFT);
  }

  static void clearIntSample(){
    for(int i = 0; i < PLOT_SAMPLE_SIZE; i++){
      plot_arr_int[i] = 0;
    }
  }

  static void clearFloatSample(){
    for(int i = 0; i < PLOT_SAMPLE_SIZE; i++){
      plot_arr_float[i] = 0;
    }
  }


  /*
   * Search the vector channels for a channel with name.
   * Return its index if channel is found, -1 otherwise.
  */
  static int fingChannelByName(const char* name){
    for(int i = 0; i < channels.size();i++){
      if(strcmp(name,channels[i].name) == 0)
      return i;
    }
    return -1;
  }

  static void sigint_handler(int signalId) {
    running = 0;
  }

  /*
   * Read and return float from ethernet.
  */
  static float getSVFloat(SVClientASDU asdu, int pos){
      return SVClientASDU_getFLOAT32(asdu, pos);
  }

  /*
   * Read and return int from ethernet.
  */
  static int getSVInt(SVClientASDU asdu,int pos){
      return SVClientASDU_getINT32(asdu, pos);
  }

  /* Callback handler for received SV messages */
  static void svUpdateListener (SVSubscriber subscriber, void* parameter, SVClientASDU asdu) {
    //printf("svUpdateListener called\n");

    const char* svID = SVClientASDU_getSvId(asdu);
    //if (svID != NULL)
    //cout<<svID<<endl;
    int channelIndex = fingChannelByName(svID);
    int dataSize = SVClientASDU_getDataSize(asdu);
    //cout<<dataSize<<endl;
    if(channelIndex == -1){
      sv_channel newChannel;
      newChannel.name = svID;
      for(int i = 0; i < dataSize/4; i++){
        newChannel.float_values.push_back(getSVFloat(asdu, i*4));
      }
      newChannel.int_values.reserve(dataSize/4);
      newChannel.dataType = FLOAT_;
      newChannel.visible = false;
      channels.push_back(newChannel);
    } else {
      if(channels[channelIndex].dataType == FLOAT_){
        for(int i = 0; i < dataSize/4; i++){
          channels[channelIndex].float_values[i] = getSVFloat(asdu, i*4);
            if(plot_sampling && i == advancedMenuOp){
              plot_arr_float[plot_count] = channels[channelIndex].float_values[i] ;
              plot_count++;
              if(plot_count >= PLOT_SAMPLE_SIZE){
                plot_count = 0;
              }
            }
        }
      } else {
          for(int i = 0; i < dataSize/4; i++){
            if(channels[channelIndex].int_values.size() <= i)
              channels[channelIndex].int_values.push_back(getSVInt(asdu, i*4));
            else {
              channels[channelIndex].int_values[i] = getSVInt(asdu, i*4);
              if(plot_sampling && i == advancedMenuOp){
                plot_arr_int[plot_count] = channels[channelIndex].int_values[i] ;
                plot_count++;
                if(plot_count >= PLOT_SAMPLE_SIZE){
                  plot_count = 0;
                }
              }
            }
          }
        }
      }
  }

  /*
   * Initilize gui window.
  */
  static void gui_init(){
    /* Platform */
    SDL_GLContext glContext;
    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Sample Values",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &win_width, &win_height);

    /* OpenGL setup */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to setup GLEW\n");
      exit(1);
    }

    ctx = nk_sdl_init(win);

    /* Load Fonts: if none of these are loaded a default font will be used */
    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../Roboto-Regular.ttf", 16, 0); // Can be safely removed
    nk_sdl_font_stash_end();
  }

  /*
   * Initilize sample value client that listens on ethernet interface "enp0s3"
  */
  static void sv_client_init(){
    // SV client
    receiver = SVReceiver_create();

    printf("Using interface enp0s3 ");
    SVReceiver_setInterfaceId(receiver, "enp0s3");

    /* Create a subscriber listening to SV messages with APPID 4000h */
    SVSubscriber subscriber = SVSubscriber_create(NULL, 0x4000);

    /* Install a callback handler for the subscriber */
    SVSubscriber_setListener(subscriber, svUpdateListener, NULL);

    /* Connect the subscriber to the receiver */
    SVReceiver_addSubscriber(receiver, subscriber);

    /* Start listening to SV messages - starts a new receiver background thread */
    SVReceiver_start(receiver);

    signal(SIGINT, sigint_handler);
  }
