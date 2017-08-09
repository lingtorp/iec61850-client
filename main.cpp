/*
 * sv_subscriber_example.c
 *
 * Example program for Sampled Values (SV) subscriber
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

#include <signal.h>
#include <stdio.h>
#include "hal_thread.h"
#include "sv_subscriber.h"


enum {INT_, FLOAT_};
struct sv_channel {
  const char* name;
  int int_value;
  float float_value;
  int dataType;
};

static bool running = true;
struct nk_color background;
static int win_width, win_height;
static SDL_Window *win;
static SVReceiver receiver;
struct nk_context *ctx;
static sv_channel channels[16];
int channel_counter = 0;

static void cleanup();

int fingChannelByName(const char* name){
  for(int i = 0; i < channel_counter;i++){
    if(strcmp(name,channels[i].name) == 0)
      return i;
  }
  return -1;
}

void sigint_handler(int signalId) {
  running = 0;
}

float getSVFloat(SVClientASDU asdu){
  if (SVClientASDU_getDataSize(asdu) >= 8) {
      return SVClientASDU_getFLOAT32(asdu, 0);
  }
  return 0;
}

int getSVInt(SVClientASDU asdu){
  if (SVClientASDU_getDataSize(asdu) >= 8) {
      return SVClientASDU_getINT32(asdu, 0);
  }
  return 0;
}

/* Callback handler for received SV messages */
static void svUpdateListener (SVSubscriber subscriber, void* parameter, SVClientASDU asdu) {
  printf("svUpdateListener called\n");

  const char* svID = SVClientASDU_getSvId(asdu);
  if (svID != NULL)
    printf("  svID=(%s)\n", svID);

  printf("  smpCnt: %i\n", getSVFloat(asdu));

  /*
  int channelIndex = fingChannelByName(svID);
  if(channelIndex == -1){
    sv_channel newChannel;
    newChannel.name = svID;
    newChannel.float_value = getSVFloat(asdu);
    newChannel.dataType = FLOAT_;
    channels[channel_counter] = newChannel;
    channel_counter++;
  } else {
    if(channels[channelIndex].dataType == FLOAT_){
      channels[channelIndex].float_value = getSVFloat(asdu);
    } else channels[channelIndex].int_value = getSVInt(asdu);
  } */
}

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
win = SDL_CreateWindow("Samle Values",
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

static void sv_client_init(){
  // SV client
      receiver = SVReceiver_create();

      printf("Using interface enp0s3  ");
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

int main(int argc, char** argv) {
    gui_init();
    sv_client_init();

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
      if (nk_begin(ctx, "Sample Values Client", nk_rect(0, 0, 800, 600),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
        NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
      {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(120, 200))) {
          nk_layout_row_dynamic(ctx, 30, 1);
          nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT);
          nk_menu_item_label(ctx, "CLOSE", NK_TEXT_LEFT);
          nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "EDIT", NK_TEXT_LEFT, nk_vec2(120, 200))) {
          nk_layout_row_dynamic(ctx, 30, 1);
          nk_menu_item_label(ctx, "COPY", NK_TEXT_LEFT);
          nk_menu_item_label(ctx, "CUT", NK_TEXT_LEFT);
          nk_menu_item_label(ctx, "PASTE", NK_TEXT_LEFT);
          nk_menu_end(ctx);
        }
        nk_layout_row_end(ctx);
        nk_menubar_end(ctx);


        static int op = INT_;
        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "REFRESH"))
          fprintf(stdout, "refreshed\n");
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "int", op == INT_)) op = INT_;
        if (nk_option_label(ctx, "float", op == FLOAT_)) op = FLOAT_;
        nk_layout_row_dynamic(ctx, 25, 1);
        float test = 10;
        for(int i = 0; i < channel_counter;i++){
          nk_property_int(ctx, channels[i], 0, &test, 100, 10, 1);
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
      /* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
       * with blending, scissor, face culling, depth test and viewport and
       * defaults everything back into a default state.
       * Make sure to either a.) save and restore or b.) reset your own state after
       * rendering the UI. */
        nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);}

        SDL_GL_SwapWindow(win);
      }
      return 0;
    }

    static void cleanup(){
      /* Stop listening to SV messages */
          SVReceiver_stop(receiver);

      /* Cleanup and free resources */
          SVReceiver_destroy(receiver);

          running = false;
    }
