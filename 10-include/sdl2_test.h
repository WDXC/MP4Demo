#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

typedef struct {
  /* SDL init flags */
  char **argv;
  Uint32 flags;
  Uint32 verbose;

  /* Video info */
  const char *videodriver;
  int display;
  const char *window_title;
  const char *window_icon;
  Uint32 window_flags;
  SDL_bool flash_on_focus_loss;
  int window_x;
  int window_y;
  int window_w;
  int window_h;
  int window_minW;
  int window_minH;
  int window_maxW;
  int window_maxH;
  int logical_w;
  int logical_h;
  float scale;
  int depth;
  int refresh_rate;
  int num_windows;
  SDL_Window **windows;

  /* Renderer info */
  const char *renderdriver;
  Uint32 render_flags;
  SDL_bool skip_renderer;
  SDL_Renderer **renderers;
  SDL_Texture **targets;

  /* Audio info */
  const char *audiodriver;
  SDL_AudioSpec audiospec;

  /* GL settings */
  int gl_red_size;
  int gl_green_size;
  int gl_blue_size;
  int gl_alpha_size;
  int gl_buffer_size;
  int gl_depth_size;
  int gl_stencil_size;
  int gl_double_buffer;
  int gl_accum_red_size;
  int gl_accum_green_size;
  int gl_accum_blue_size;
  int gl_accum_alpha_size;
  int gl_stereo;
  int gl_multisamplebuffers;
  int gl_multisamplesamples;
  int gl_retained_backing;
  int gl_accelerated;
  int gl_major_version;
  int gl_minor_version;
  int gl_debug;
  int gl_profile_mask;

  /* Additional fields added in 2.0.18 */
  SDL_Rect confine;

} CommonState;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *background;
  SDL_Rect sprite_rect;
  int scale_direction;
} PaintState;

class MP4Demo {
 public:
  MP4Demo();

 public:
  CommonState *create_default_state(Uint32 flags);
  SDL_bool commoninit(CommonState *state);
  void SetCommonState(CommonState *s);
  void SetPaintState(PaintState *s);
  void SetDoneFlag(int val);
  int GetDoneFlag();
  void CommonEvent(CommonState *state, SDL_Event *event, int *done);
  void loop();
  void Draw(PaintState *s);
  char *GetNearbyFilename(const char *file);
  char *GetResourceFilename(const char *user_specified, const char *def);
  SDL_Texture *LoadTexture(SDL_Renderer *renderer, const char *file,
                           SDL_bool transparent, int *width_out,
                           int *height_out);

 private:
  CommonState *m_state;
  PaintState *m_pstate;
  int m_done;
};