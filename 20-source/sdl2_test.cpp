#include "sdl2_test.h"

#include <SDL2/SDL_config.h>
#include <SDL2/SDL_test_common.h>
#include <unistd.h>

MP4Demo::MP4Demo() { this->m_done = 0; }

char *MP4Demo::GetResourceFilename(const char *user_specified,
                                   const char *def) {
  if (user_specified) {
    char *ret = SDL_strdup(user_specified);

    if (!ret) {
      SDL_OutOfMemory();
    }

    return ret;
  } else {
    return GetNearbyFilename(def);
  }
}

char *MP4Demo::GetNearbyFilename(const char *file) {
  char *base;
  char *path;

  base = SDL_GetBasePath();

  if (base) {
    SDL_RWops *rw;
    size_t len = SDL_strlen(base) + SDL_strlen(file) + 1;

    path = (char *)SDL_malloc(len);

    if (!path) {
      SDL_free(base);
      SDL_OutOfMemory();
      return NULL;
    }

    (void)SDL_snprintf(path, len, "%s%s", base, file);
    SDL_free(base);

    rw = SDL_RWFromFile(path, "rb");
    if (rw) {
      SDL_RWclose(rw);
      return path;
    }

    /* Couldn't find the file in the base path */
    SDL_free(path);
  }

  path = SDL_strdup(file);
  if (!path) {
    SDL_OutOfMemory();
  }
  return path;
}

SDL_Texture *MP4Demo::LoadTexture(SDL_Renderer *renderer, const char *file,
                                  SDL_bool transparent, int *width_out,
                                  int *height_out) {
  SDL_Surface *temp = NULL;
  SDL_Texture *texture = NULL;
  char *path;

  path = GetNearbyFilename(file);

  if (path) {
    file = path;
  }

  temp = SDL_LoadBMP(file);
  if (!temp) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s: %s", file,
                 SDL_GetError());
  } else {
    /* Set transparent pixel as the pixel at (0,0) */
    if (transparent) {
      if (temp->format->palette) {
        SDL_SetColorKey(temp, SDL_TRUE, *(Uint8 *)temp->pixels);
      } else {
        switch (temp->format->BitsPerPixel) {
          case 15:
            SDL_SetColorKey(temp, SDL_TRUE,
                            (*(Uint16 *)temp->pixels) & 0x00007FFF);
            break;
          case 16:
            SDL_SetColorKey(temp, SDL_TRUE, *(Uint16 *)temp->pixels);
            break;
          case 24:
            SDL_SetColorKey(temp, SDL_TRUE,
                            (*(Uint32 *)temp->pixels) & 0x00FFFFFF);
            break;
          case 32:
            SDL_SetColorKey(temp, SDL_TRUE, *(Uint32 *)temp->pixels);
            break;
        }
      }
    }

    if (width_out) {
      *width_out = temp->w;
    }

    if (height_out) {
      *height_out = temp->h;
    }

    texture = SDL_CreateTextureFromSurface(renderer, temp);
    if (!texture) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Couldn't create texture: %s\n", SDL_GetError());
    }
  }
  SDL_FreeSurface(temp);
  if (path) {
    SDL_free(path);
  }
  return texture;
}

CommonState *MP4Demo::create_default_state(Uint32 flags) {
  int i;
  CommonState *state;

  state = (CommonState *)SDL_calloc(1, sizeof(*state));

  if (!state) {
    SDL_OutOfMemory();
    return NULL;
  }

  state->flags = flags;
  state->window_title = "helo";
  state->videodriver = "X11";
  state->window_flags = 0;
  state->window_x = SDL_WINDOWPOS_UNDEFINED;
  state->window_y = SDL_WINDOWPOS_UNDEFINED;
  state->window_w = DEFAULT_WINDOW_WIDTH;
  state->window_h = DEFAULT_WINDOW_HEIGHT;
  state->verbose = VERBOSE_VIDEO;
  state->num_windows = 1;
  state->audiospec.freq = 22050;
  state->audiospec.format = AUDIO_S16;
  state->audiospec.channels = 2;
  state->audiospec.samples = 2048;

  /* Set some very sane GL defaults */
  state->gl_red_size = 3;
  state->gl_green_size = 3;
  state->gl_blue_size = 2;
  state->gl_alpha_size = 0;
  state->gl_buffer_size = 0;
  state->gl_depth_size = 16;
  state->gl_stencil_size = 0;
  state->gl_double_buffer = 1;
  state->gl_accum_red_size = 0;
  state->gl_accum_green_size = 0;
  state->gl_accum_blue_size = 0;
  state->gl_accum_alpha_size = 0;
  state->gl_stereo = 0;
  state->gl_multisamplebuffers = 0;
  state->gl_multisamplesamples = 0;
  state->gl_retained_backing = 1;
  state->gl_accelerated = -1;
  state->gl_debug = 0;

  return state;
}

SDL_bool MP4Demo::commoninit(CommonState *state) {
  int i, j, m, n, w, h;
  SDL_DisplayMode fullscreen_mode;
  char text[1024];

  SDL_Log("judge check condition: %d\n",
          (state->flags & SDL_INIT_VIDEO) ? 1 : 0);
  if (state->flags & SDL_INIT_VIDEO) {
    if (state->verbose & VERBOSE_VIDEO) {
      n = SDL_GetNumVideoDrivers();
      SDL_Log("built-in video drivers num is %d\n", n);
      if (n == 0) {
        SDL_Log("No built-in video drivers\n");
      } else {
        (void)SDL_snprintf(text, sizeof(text), "Built-in video drivers:");
        SDL_Log("%s\n", text);
      }
    }
    if (SDL_VideoInit(state->videodriver) < 0) {
      SDL_Log("Couldn't initialize video driver: %s\n", SDL_GetError());
      return SDL_FALSE;
    }
    if (state->verbose & VERBOSE_VIDEO) {
      SDL_Log("Video driver: %s\n", SDL_GetCurrentVideoDriver());
    }

    /* Upload GL settings */
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, state->gl_red_size);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, state->gl_green_size);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, state->gl_blue_size);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, state->gl_alpha_size);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, state->gl_double_buffer);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, state->gl_buffer_size);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, state->gl_depth_size);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, state->gl_stencil_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, state->gl_accum_red_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, state->gl_accum_green_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, state->gl_accum_blue_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, state->gl_accum_alpha_size);
    SDL_GL_SetAttribute(SDL_GL_STEREO, state->gl_stereo);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,
                        state->gl_multisamplebuffers);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,
                        state->gl_multisamplesamples);
    if (state->gl_accelerated >= 0) {
      SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, state->gl_accelerated);
    }
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, state->gl_retained_backing);
    if (state->gl_major_version) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,
                          state->gl_major_version);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,
                          state->gl_minor_version);
    }
    if (state->gl_debug) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }
    if (state->gl_profile_mask) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, state->gl_profile_mask);
    }

    if (state->verbose & VERBOSE_MODES) {
      SDL_Rect bounds, usablebounds;
      float hdpi = 0;
      float vdpi = 0;
      SDL_DisplayMode mode;
      int bpp;
      Uint32 Rmask, Gmask, Bmask, Amask;
#ifdef SDL_VIDEO_DRIVER_WINDOWS
      int adapterIndex = 0;
      int outputIndex = 0;
#endif
      n = SDL_GetNumVideoDisplays();
      SDL_Log("Number of displays: %d\n", n);
      for (i = 0; i < n; ++i) {
        SDL_Log("Display %d: %s\n", i, SDL_GetDisplayName(i));

        SDL_zero(bounds);
        SDL_GetDisplayBounds(i, &bounds);

        SDL_zero(usablebounds);
        SDL_GetDisplayUsableBounds(i, &usablebounds);

        SDL_GetDisplayDPI(i, NULL, &hdpi, &vdpi);

        SDL_Log("Bounds: %dx%d at %d,%d\n", bounds.w, bounds.h, bounds.x,
                bounds.y);
        SDL_Log("Usable bounds: %dx%d at %d,%d\n", usablebounds.w,
                usablebounds.h, usablebounds.x, usablebounds.y);
        SDL_Log("DPI: %fx%f\n", hdpi, vdpi);

        SDL_GetDesktopDisplayMode(i, &mode);
        SDL_PixelFormatEnumToMasks(mode.format, &bpp, &Rmask, &Gmask, &Bmask,
                                   &Amask);
        SDL_Log("  Current mode: %dx%d@%dHz, %d bits-per-pixel (%s)\n", mode.w,
                mode.h, mode.refresh_rate, bpp,
                SDL_GetPixelFormatName(mode.format));
        if (Rmask || Gmask || Bmask) {
          SDL_Log("      Red Mask   = 0x%.8" SDL_PRIx32 "\n", Rmask);
          SDL_Log("      Green Mask = 0x%.8" SDL_PRIx32 "\n", Gmask);
          SDL_Log("      Blue Mask  = 0x%.8" SDL_PRIx32 "\n", Bmask);
          if (Amask) {
            SDL_Log("      Alpha Mask = 0x%.8" SDL_PRIx32 "\n", Amask);
          }
        }

        /* Print available fullscreen video modes */
        m = SDL_GetNumDisplayModes(i);
        if (m == 0) {
          SDL_Log("No available fullscreen video modes\n");
        } else {
          SDL_Log("  Fullscreen video modes:\n");
          for (j = 0; j < m; ++j) {
            SDL_GetDisplayMode(i, j, &mode);
            SDL_PixelFormatEnumToMasks(mode.format, &bpp, &Rmask, &Gmask,
                                       &Bmask, &Amask);
            SDL_Log("    Mode %d: %dx%d@%dHz, %d bits-per-pixel (%s)\n", j,
                    mode.w, mode.h, mode.refresh_rate, bpp,
                    SDL_GetPixelFormatName(mode.format));
            if (Rmask || Gmask || Bmask) {
              SDL_Log("        Red Mask   = 0x%.8" SDL_PRIx32 "\n", Rmask);
              SDL_Log("        Green Mask = 0x%.8" SDL_PRIx32 "\n", Gmask);
              SDL_Log("        Blue Mask  = 0x%.8" SDL_PRIx32 "\n", Bmask);
              if (Amask) {
                SDL_Log("        Alpha Mask = 0x%.8" SDL_PRIx32 "\n", Amask);
              }
            }
          }
        }

#if defined(SDL_VIDEO_DRIVER_WINDOWS) && !defined(__XBOXONE__) && \
    !defined(__XBOXSERIES__)
        /* Print the D3D9 adapter index */
        adapterIndex = SDL_Direct3D9GetAdapterIndex(i);
        SDL_Log("D3D9 Adapter Index: %d", adapterIndex);

        /* Print the DXGI adapter and output indices */
        SDL_DXGIGetOutputInfo(i, &adapterIndex, &outputIndex);
        SDL_Log("DXGI Adapter Index: %d  Output Index: %d", adapterIndex,
                outputIndex);
#endif
      }
    }

    if (state->verbose & VERBOSE_RENDER) {
      SDL_RendererInfo info;

      n = SDL_GetNumRenderDrivers();
      if (n == 0) {
        SDL_Log("No built-in render drivers\n");
      } else {
        SDL_Log("Built-in render drivers:\n");
        for (i = 0; i < n; ++i) {
          SDL_GetRenderDriverInfo(i, &info);
        }
      }
    }

    SDL_zero(fullscreen_mode);
    switch (state->depth) {
      case 8:
        fullscreen_mode.format = SDL_PIXELFORMAT_INDEX8;
        break;
      case 15:
        fullscreen_mode.format = SDL_PIXELFORMAT_RGB555;
        break;
      case 16:
        fullscreen_mode.format = SDL_PIXELFORMAT_RGB565;
        break;
      case 24:
        fullscreen_mode.format = SDL_PIXELFORMAT_RGB24;
        break;
      default:
        fullscreen_mode.format = SDL_PIXELFORMAT_RGB888;
        break;
    }
    fullscreen_mode.refresh_rate = state->refresh_rate;

    state->windows =
        (SDL_Window **)SDL_calloc(state->num_windows, sizeof(*state->windows));
    state->renderers = (SDL_Renderer **)SDL_calloc(state->num_windows,
                                                   sizeof(*state->renderers));
    state->targets =
        (SDL_Texture **)SDL_calloc(state->num_windows, sizeof(*state->targets));
    if (!state->windows || !state->renderers) {
      SDL_Log("Out of memory!\n");
      return SDL_FALSE;
    }
    for (i = 0; i < state->num_windows; ++i) {
      char title[1024];
      SDL_Rect r;

      r.x = state->window_x;
      r.y = state->window_y;
      r.w = state->window_w;
      r.h = state->window_h;

      /* !!! FIXME: hack to make --usable-bounds work for now. */
      if ((r.x == -1) && (r.y == -1) && (r.w == -1) && (r.h == -1)) {
        SDL_GetDisplayUsableBounds(state->display, &r);
      }

      if (state->num_windows > 1) {
        (void)SDL_snprintf(title, SDL_arraysize(title), "%s %d",
                           state->window_title, i + 1);
      } else {
        SDL_strlcpy(title, state->window_title, SDL_arraysize(title));
      }
      state->windows[i] =
          SDL_CreateWindow(title, r.x, r.y, r.w, r.h, state->window_flags);
      if (!state->windows[i]) {
        SDL_Log("Couldn't create window: %s\n", SDL_GetError());
        return SDL_FALSE;
      }
      if (state->window_minW || state->window_minH) {
        SDL_SetWindowMinimumSize(state->windows[i], state->window_minW,
                                 state->window_minH);
      }
      if (state->window_maxW || state->window_maxH) {
        SDL_SetWindowMaximumSize(state->windows[i], state->window_maxW,
                                 state->window_maxH);
      }
      SDL_GetWindowSize(state->windows[i], &w, &h);
      if (!(state->window_flags & SDL_WINDOW_RESIZABLE) &&
          (w != state->window_w || h != state->window_h)) {
        SDL_Log("Window requested size %dx%d, got %dx%d\n", state->window_w,
                state->window_h, w, h);
        state->window_w = w;
        state->window_h = h;
      }
      fullscreen_mode.w = state->window_w;
      fullscreen_mode.h = state->window_h;
      if (SDL_SetWindowDisplayMode(state->windows[i], &fullscreen_mode) < 0) {
        SDL_Log("Can't set up fullscreen display mode: %s\n", SDL_GetError());
        return SDL_FALSE;
      }

      SDL_ShowWindow(state->windows[i]);

      if (!SDL_RectEmpty(&state->confine)) {
        SDL_SetWindowMouseRect(state->windows[i], &state->confine);
      }

      if (!state->skip_renderer &&
          (state->renderdriver ||
           !(state->window_flags &
             (SDL_WINDOW_OPENGL | SDL_WINDOW_VULKAN | SDL_WINDOW_METAL)))) {
        m = -1;
        if (state->renderdriver) {
          SDL_RendererInfo info;
          n = SDL_GetNumRenderDrivers();
          for (j = 0; j < n; ++j) {
            SDL_GetRenderDriverInfo(j, &info);
            if (SDL_strcasecmp(info.name, state->renderdriver) == 0) {
              m = j;
              break;
            }
          }
          if (m == -1) {
            SDL_Log("Couldn't find render driver named %s",
                    state->renderdriver);
            return SDL_FALSE;
          }
        }
        state->renderers[i] =
            SDL_CreateRenderer(state->windows[i], m, state->render_flags);
        if (!state->renderers[i]) {
          SDL_Log("Couldn't create renderer: %s\n", SDL_GetError());
          return SDL_FALSE;
        }
        if (state->logical_w && state->logical_h) {
          SDL_RenderSetLogicalSize(state->renderers[i], state->logical_w,
                                   state->logical_h);
        } else if (state->scale != 0.) {
          SDL_RenderSetScale(state->renderers[i], state->scale, state->scale);
        }
        if (state->verbose & VERBOSE_RENDER) {
          SDL_RendererInfo info;

          SDL_Log("Current renderer:\n");
          SDL_GetRendererInfo(state->renderers[i], &info);
        }
      }
    }
  }
  return SDL_TRUE;
}

void MP4Demo::Draw(PaintState *s) {
  SDL_Rect viewport;
  SDL_Texture *target;
  SDL_Point *center = NULL;
  SDL_Point origin = {0, 0};

  SDL_RenderGetViewport(s->renderer, &viewport);

  target = SDL_CreateTexture(s->renderer, SDL_PIXELFORMAT_ARGB8888,
                             SDL_TEXTUREACCESS_TARGET, viewport.w, viewport.h);
  SDL_SetRenderTarget(s->renderer, target);

  /* Draw the background */
  SDL_RenderCopy(s->renderer, s->background, NULL, NULL);

  /* Scale and draw the sprite */
  s->sprite_rect.w += s->scale_direction;
  s->sprite_rect.h += s->scale_direction;
  if (s->scale_direction > 0) {
    center = &origin;
    if (s->sprite_rect.w >= viewport.w || s->sprite_rect.h >= viewport.h) {
      s->scale_direction = -1;
    }
  } else {
    if (s->sprite_rect.w <= 1 || s->sprite_rect.h <= 1) {
      s->scale_direction = 1;
    }
  }
  s->sprite_rect.x = (viewport.w - s->sprite_rect.w) / 2;
  s->sprite_rect.y = (viewport.h - s->sprite_rect.h) / 2;

  SDL_RenderCopyEx(s->renderer, NULL, NULL, &s->sprite_rect,
                   (double)s->sprite_rect.w, center,
                   (SDL_RendererFlip)s->scale_direction);

  SDL_SetRenderTarget(s->renderer, NULL);
  SDL_RenderCopy(s->renderer, target, NULL, NULL);
  SDL_DestroyTexture(target);

  /* Update the screen! */
  SDL_RenderPresent(s->renderer);
  /* SDL_Delay(10); */
}
void MP4Demo::SetCommonState(CommonState *s) { this->m_state = s; }

void MP4Demo::SetPaintState(PaintState *s) { this->m_pstate = s; }

static void SDLTest_ScreenShot(SDL_Renderer *renderer) {
  SDL_Rect viewport;
  SDL_Surface *surface;

  if (!renderer) {
    return;
  }

  SDL_RenderGetViewport(renderer, &viewport);
  surface = SDL_CreateRGBSurface(0, viewport.w, viewport.h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                 0x00FF0000, 0x0000FF00, 0x000000FF,
#else
                                 0x000000FF, 0x0000FF00, 0x00FF0000,
#endif
                                 0x00000000);
  if (!surface) {
    SDL_Log("Couldn't create surface: %s\n", SDL_GetError());
    return;
  }

  if (SDL_RenderReadPixels(renderer, NULL, surface->format->format,
                           surface->pixels, surface->pitch) < 0) {
    SDL_Log("Couldn't read screen: %s\n", SDL_GetError());
    SDL_free(surface);
    return;
  }

  if (SDL_SaveBMP(surface, "screenshot.bmp") < 0) {
    SDL_Log("Couldn't save screenshot.bmp: %s\n", SDL_GetError());
    SDL_free(surface);
    return;
  }
}

static const char *ControllerButtonName(const SDL_GameControllerButton button) {
  switch (button) {
#define BUTTON_CASE(btn)            \
  case SDL_CONTROLLER_BUTTON_##btn: \
    return #btn
    BUTTON_CASE(INVALID);
    BUTTON_CASE(A);
    BUTTON_CASE(B);
    BUTTON_CASE(X);
    BUTTON_CASE(Y);
    BUTTON_CASE(BACK);
    BUTTON_CASE(GUIDE);
    BUTTON_CASE(START);
    BUTTON_CASE(LEFTSTICK);
    BUTTON_CASE(RIGHTSTICK);
    BUTTON_CASE(LEFTSHOULDER);
    BUTTON_CASE(RIGHTSHOULDER);
    BUTTON_CASE(DPAD_UP);
    BUTTON_CASE(DPAD_DOWN);
    BUTTON_CASE(DPAD_LEFT);
    BUTTON_CASE(DPAD_RIGHT);
#undef BUTTON_CASE
    default:
      return "???";
  }
}

static void SDLTest_PrintEvent(SDL_Event *event) {
  switch (event->type) {
    case SDL_DISPLAYEVENT:
      switch (event->display.event) {
        case SDL_DISPLAYEVENT_CONNECTED:
          SDL_Log("SDL EVENT: Display %" SDL_PRIu32 " connected",
                  event->display.display);
          break;
        case SDL_DISPLAYEVENT_MOVED:
          SDL_Log("SDL EVENT: Display %" SDL_PRIu32 " changed position",
                  event->display.display);
          break;
        case SDL_DISPLAYEVENT_ORIENTATION:
          SDL_Log("SDL EVENT: Display %" SDL_PRIu32 " changed orientation to ",
                  event->display.display);
          break;
        case SDL_DISPLAYEVENT_DISCONNECTED:
          SDL_Log("SDL EVENT: Display %" SDL_PRIu32 " disconnected",
                  event->display.display);
          break;
        default:
          SDL_Log("SDL EVENT: Display %" SDL_PRIu32
                  " got unknown event 0x%4.4x",
                  event->display.display, event->display.event);
          break;
      }
      break;
    case SDL_WINDOWEVENT:
      switch (event->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " shown",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_HIDDEN:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " hidden",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_EXPOSED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " exposed",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_MOVED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " moved to %" SDL_PRIs32
                  ",%" SDL_PRIs32,
                  event->window.windowID, event->window.data1,
                  event->window.data2);
          break;
        case SDL_WINDOWEVENT_RESIZED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " resized to %" SDL_PRIs32
                  "x%" SDL_PRIs32,
                  event->window.windowID, event->window.data1,
                  event->window.data2);
          break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32
                  " changed size to %" SDL_PRIs32 "x%" SDL_PRIs32,
                  event->window.windowID, event->window.data1,
                  event->window.data2);
          break;
        case SDL_WINDOWEVENT_MINIMIZED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " minimized",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_MAXIMIZED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " maximized",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_RESTORED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " restored",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_ENTER:
          SDL_Log("SDL EVENT: Mouse entered window %" SDL_PRIu32,
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_LEAVE:
          SDL_Log("SDL EVENT: Mouse left window %" SDL_PRIu32,
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " gained keyboard focus",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " lost keyboard focus",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_CLOSE:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " closed",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_TAKE_FOCUS:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " take focus",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_HIT_TEST:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " hit test",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_ICCPROF_CHANGED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " ICC profile changed",
                  event->window.windowID);
          break;
        case SDL_WINDOWEVENT_DISPLAY_CHANGED:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32
                  " display changed to %" SDL_PRIs32,
                  event->window.windowID, event->window.data1);
          break;
        default:
          SDL_Log("SDL EVENT: Window %" SDL_PRIu32 " got unknown event 0x%4.4x",
                  event->window.windowID, event->window.event);
          break;
      }
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
      char modstr[64];
      if (event->key.keysym.mod) {
        modstr[0] = '\0';
      } else {
        SDL_strlcpy(modstr, "NONE", sizeof(modstr));
      }
      SDL_Log(
          "SDL EVENT: Keyboard: key %s in window %" SDL_PRIu32
          ": scancode 0x%08X = %s, keycode 0x%08" SDL_PRIX32 " = %s, mods = %s",
          (event->type == SDL_KEYDOWN) ? "pressed" : "released",
          event->key.windowID, event->key.keysym.scancode,
          SDL_GetScancodeName(event->key.keysym.scancode),
          event->key.keysym.sym, SDL_GetKeyName(event->key.keysym.sym), modstr);
      break;
    }
    case SDL_TEXTEDITING:
      SDL_Log("SDL EVENT: Keyboard: text editing \"%s\" in window %" SDL_PRIu32,
              event->edit.text, event->edit.windowID);
      break;
    case SDL_TEXTINPUT:
      SDL_Log("SDL EVENT: Keyboard: text input \"%s\" in window %" SDL_PRIu32,
              event->text.text, event->text.windowID);
      break;
    case SDL_KEYMAPCHANGED:
      SDL_Log("SDL EVENT: Keymap changed");
      break;
    case SDL_MOUSEMOTION:
      SDL_Log("SDL EVENT: Mouse: moved to %" SDL_PRIs32 ",%" SDL_PRIs32
              " (%" SDL_PRIs32 ",%" SDL_PRIs32 ") in window %" SDL_PRIu32,
              event->motion.x, event->motion.y, event->motion.xrel,
              event->motion.yrel, event->motion.windowID);
      break;
    case SDL_MOUSEBUTTONDOWN:
      SDL_Log("SDL EVENT: Mouse: button %d pressed at %" SDL_PRIs32
              ",%" SDL_PRIs32 " with click count %d in window %" SDL_PRIu32,
              event->button.button, event->button.x, event->button.y,
              event->button.clicks, event->button.windowID);
      break;
    case SDL_MOUSEBUTTONUP:
      SDL_Log("SDL EVENT: Mouse: button %d released at %" SDL_PRIs32
              ",%" SDL_PRIs32 " with click count %d in window %" SDL_PRIu32,
              event->button.button, event->button.x, event->button.y,
              event->button.clicks, event->button.windowID);
      break;
    case SDL_MOUSEWHEEL:
      SDL_Log("SDL EVENT: Mouse: wheel scrolled %" SDL_PRIs32
              " in x and %" SDL_PRIs32 " in y (reversed: %" SDL_PRIu32
              ") in window %" SDL_PRIu32,
              event->wheel.x, event->wheel.y, event->wheel.direction,
              event->wheel.windowID);
      break;
    case SDL_JOYDEVICEADDED:
      SDL_Log("SDL EVENT: Joystick index %" SDL_PRIs32 " attached",
              event->jdevice.which);
      break;
    case SDL_JOYDEVICEREMOVED:
      SDL_Log("SDL EVENT: Joystick %" SDL_PRIs32 " removed",
              event->jdevice.which);
      break;
    case SDL_JOYBALLMOTION:
      SDL_Log("SDL EVENT: Joystick %" SDL_PRIs32 ": ball %d moved by %d,%d",
              event->jball.which, event->jball.ball, event->jball.xrel,
              event->jball.yrel);
      break;
    case SDL_JOYHATMOTION: {
      const char *position = "UNKNOWN";
      switch (event->jhat.value) {
        case SDL_HAT_CENTERED:
          position = "CENTER";
          break;
        case SDL_HAT_UP:
          position = "UP";
          break;
        case SDL_HAT_RIGHTUP:
          position = "RIGHTUP";
          break;
        case SDL_HAT_RIGHT:
          position = "RIGHT";
          break;
        case SDL_HAT_RIGHTDOWN:
          position = "RIGHTDOWN";
          break;
        case SDL_HAT_DOWN:
          position = "DOWN";
          break;
        case SDL_HAT_LEFTDOWN:
          position = "LEFTDOWN";
          break;
        case SDL_HAT_LEFT:
          position = "LEFT";
          break;
        case SDL_HAT_LEFTUP:
          position = "LEFTUP";
          break;
      }
      SDL_Log("SDL EVENT: Joystick %" SDL_PRIs32 ": hat %d moved to %s",
              event->jhat.which, event->jhat.hat, position);
    } break;
    case SDL_JOYBUTTONDOWN:
      SDL_Log("SDL EVENT: Joystick %" SDL_PRIs32 ": button %d pressed",
              event->jbutton.which, event->jbutton.button);
      break;
    case SDL_JOYBUTTONUP:
      SDL_Log("SDL EVENT: Joystick %" SDL_PRIs32 ": button %d released",
              event->jbutton.which, event->jbutton.button);
      break;
    case SDL_CONTROLLERDEVICEADDED:
      SDL_Log("SDL EVENT: Controller index %" SDL_PRIs32 " attached",
              event->cdevice.which);
      break;
    case SDL_CONTROLLERDEVICEREMOVED:
      SDL_Log("SDL EVENT: Controller %" SDL_PRIs32 " removed",
              event->cdevice.which);
      break;
    case SDL_CONTROLLERAXISMOTION:
      break;
    case SDL_CONTROLLERBUTTONDOWN:
      SDL_Log("SDL EVENT: Controller %" SDL_PRIs32 "button %d ('%s') down",
              event->cbutton.which, event->cbutton.button,
              ControllerButtonName(
                  (SDL_GameControllerButton)event->cbutton.button));
      break;
    case SDL_CONTROLLERBUTTONUP:
      SDL_Log("SDL EVENT: Controller %" SDL_PRIs32 " button %d ('%s') up",
              event->cbutton.which, event->cbutton.button,
              ControllerButtonName(
                  (SDL_GameControllerButton)event->cbutton.button));
      break;
    case SDL_CLIPBOARDUPDATE:
      SDL_Log("SDL EVENT: Clipboard updated");
      break;

    case SDL_FINGERMOTION:
      SDL_Log(
          "SDL EVENT: Finger: motion touch=%ld, finger=%ld, x=%f, y=%f, dx=%f, "
          "dy=%f, pressure=%f",
          (long)event->tfinger.touchId, (long)event->tfinger.fingerId,
          event->tfinger.x, event->tfinger.y, event->tfinger.dx,
          event->tfinger.dy, event->tfinger.pressure);
      break;
    case SDL_FINGERDOWN:
    case SDL_FINGERUP:
      SDL_Log(
          "SDL EVENT: Finger: %s touch=%ld, finger=%ld, x=%f, y=%f, dx=%f, "
          "dy=%f, pressure=%f",
          (event->type == SDL_FINGERDOWN) ? "down" : "up",
          (long)event->tfinger.touchId, (long)event->tfinger.fingerId,
          event->tfinger.x, event->tfinger.y, event->tfinger.dx,
          event->tfinger.dy, event->tfinger.pressure);
      break;
    case SDL_DOLLARGESTURE:
      SDL_Log("SDL_EVENT: Dollar gesture detect: %ld",
              (long)event->dgesture.gestureId);
      break;
    case SDL_DOLLARRECORD:
      SDL_Log("SDL_EVENT: Dollar gesture record: %ld",
              (long)event->dgesture.gestureId);
      break;
    case SDL_MULTIGESTURE:
      SDL_Log("SDL_EVENT: Multi gesture fingers: %d",
              event->mgesture.numFingers);
      break;

    case SDL_RENDER_DEVICE_RESET:
      SDL_Log("SDL EVENT: render device reset");
      break;
    case SDL_RENDER_TARGETS_RESET:
      SDL_Log("SDL EVENT: render targets reset");
      break;

    case SDL_APP_TERMINATING:
      SDL_Log("SDL EVENT: App terminating");
      break;
    case SDL_APP_LOWMEMORY:
      SDL_Log("SDL EVENT: App running low on memory");
      break;
    case SDL_APP_WILLENTERBACKGROUND:
      SDL_Log("SDL EVENT: App will enter the background");
      break;
    case SDL_APP_DIDENTERBACKGROUND:
      SDL_Log("SDL EVENT: App entered the background");
      break;
    case SDL_APP_WILLENTERFOREGROUND:
      SDL_Log("SDL EVENT: App will enter the foreground");
      break;
    case SDL_APP_DIDENTERFOREGROUND:
      SDL_Log("SDL EVENT: App entered the foreground");
      break;
    case SDL_DROPBEGIN:
      SDL_Log("SDL EVENT: Drag and drop beginning");
      break;
    case SDL_DROPFILE:
      SDL_Log("SDL EVENT: Drag and drop file: '%s'", event->drop.file);
      break;
    case SDL_DROPTEXT:
      SDL_Log("SDL EVENT: Drag and drop text: '%s'", event->drop.file);
      break;
    case SDL_DROPCOMPLETE:
      SDL_Log("SDL EVENT: Drag and drop ending");
      break;
    case SDL_QUIT:
      SDL_Log("SDL EVENT: Quit requested");
      break;
    case SDL_USEREVENT:
      SDL_Log("SDL EVENT: User event %" SDL_PRIs32, event->user.code);
      break;
    default:
      SDL_Log("Unknown event 0x%4.4" SDL_PRIu32, event->type);
      break;
  }
}

void MP4Demo::CommonEvent(CommonState *state, SDL_Event *event, int *done) {
  int i;
  static SDL_MouseMotionEvent lastEvent;

  if (state->verbose & VERBOSE_EVENT) {
    if (((event->type != SDL_MOUSEMOTION) &&
         (event->type != SDL_FINGERMOTION)) ||
        ((state->verbose & VERBOSE_MOTION) != 0)) {
      SDLTest_PrintEvent(event);
    }
  }

  switch (event->type) {
    case SDL_WINDOWEVENT:
      switch (event->window.event) {
        case SDL_WINDOWEVENT_CLOSE: {
          SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
          if (window) {
            for (i = 0; i < state->num_windows; ++i) {
              if (window == state->windows[i]) {
                if (state->targets[i]) {
                  SDL_DestroyTexture(state->targets[i]);
                  state->targets[i] = NULL;
                }
                if (state->renderers[i]) {
                  SDL_DestroyRenderer(state->renderers[i]);
                  state->renderers[i] = NULL;
                }
                SDL_DestroyWindow(state->windows[i]);
                state->windows[i] = NULL;
                break;
              }
            }
          }
        } break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
          if (state->flash_on_focus_loss) {
            SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
            if (window) {
              SDL_FlashWindow(window, SDL_FLASH_UNTIL_FOCUSED);
            }
          }
          break;
        default:
          break;
      }
      break;
    case SDL_KEYDOWN: {
      SDL_bool withControl = (SDL_bool)(!!(event->key.keysym.mod & KMOD_CTRL));
      SDL_bool withShift = (SDL_bool)(!!(event->key.keysym.mod & KMOD_SHIFT));
      SDL_bool withAlt = (SDL_bool)(!!(event->key.keysym.mod & KMOD_ALT));

      switch (event->key.keysym.sym) {
          /* Add hotkeys here */
        case SDLK_PRINTSCREEN: {
          SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
          if (window) {
            for (i = 0; i < state->num_windows; ++i) {
              if (window == state->windows[i]) {
                SDLTest_ScreenShot(state->renderers[i]);
              }
            }
          }
        } break;
        case SDLK_EQUALS:
          if (withControl) {
            /* Ctrl-+ double the size of the window */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              int w, h;
              SDL_GetWindowSize(window, &w, &h);
              SDL_SetWindowSize(window, w * 2, h * 2);
            }
          }
          break;
        case SDLK_MINUS:
          if (withControl) {
            /* Ctrl-- half the size of the window */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              int w, h;
              SDL_GetWindowSize(window, &w, &h);
              SDL_SetWindowSize(window, w / 2, h / 2);
            }
          }
          break;
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
          if (withAlt) {
            /* Alt-Up/Down/Left/Right switches between displays */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              int currentIndex = SDL_GetWindowDisplayIndex(window);
              int numDisplays = SDL_GetNumVideoDisplays();

              if (currentIndex >= 0 && numDisplays >= 1) {
                int dest;
                if (event->key.keysym.sym == SDLK_UP ||
                    event->key.keysym.sym == SDLK_LEFT) {
                  dest = (currentIndex + numDisplays - 1) % numDisplays;
                } else {
                  dest = (currentIndex + numDisplays + 1) % numDisplays;
                }
                SDL_Log("Centering on display %d\n", dest);
                SDL_SetWindowPosition(window,
                                      SDL_WINDOWPOS_CENTERED_DISPLAY(dest),
                                      SDL_WINDOWPOS_CENTERED_DISPLAY(dest));
              }
            }
          }
          if (withShift) {
            /* Shift-Up/Down/Left/Right shift the window by 100px */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              const int delta = 100;
              int x, y;
              SDL_GetWindowPosition(window, &x, &y);

              if (event->key.keysym.sym == SDLK_UP) {
                y -= delta;
              }
              if (event->key.keysym.sym == SDLK_DOWN) {
                y += delta;
              }
              if (event->key.keysym.sym == SDLK_LEFT) {
                x -= delta;
              }
              if (event->key.keysym.sym == SDLK_RIGHT) {
                x += delta;
              }

              SDL_Log("Setting position to (%d, %d)\n", x, y);
              SDL_SetWindowPosition(window, x, y);
            }
          }
          break;
        case SDLK_o:
          if (withControl) {
            /* Ctrl-O (or Ctrl-Shift-O) changes window opacity. */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              float opacity;
              if (SDL_GetWindowOpacity(window, &opacity) == 0) {
                if (withShift) {
                  opacity += 0.20f;
                } else {
                  opacity -= 0.20f;
                }
                SDL_SetWindowOpacity(window, opacity);
              }
            }
          }
          break;

        case SDLK_c:
          if (withControl) {
            /* Ctrl-C copy awesome text! */
            SDL_SetClipboardText("SDL rocks!\nYou know it!");
            SDL_Log("Copied text to clipboard\n");
          }
          if (withAlt) {
            /* Alt-C toggle a render clip rectangle */
            for (i = 0; i < state->num_windows; ++i) {
              int w, h;
              if (state->renderers[i]) {
                SDL_Rect clip;
                SDL_GetWindowSize(state->windows[i], &w, &h);
                SDL_RenderGetClipRect(state->renderers[i], &clip);
                if (SDL_RectEmpty(&clip)) {
                  clip.x = w / 4;
                  clip.y = h / 4;
                  clip.w = w / 2;
                  clip.h = h / 2;
                  SDL_RenderSetClipRect(state->renderers[i], &clip);
                } else {
                  SDL_RenderSetClipRect(state->renderers[i], NULL);
                }
              }
            }
          }
          if (withShift) {
            SDL_Window *current_win = SDL_GetKeyboardFocus();
            if (current_win) {
              const SDL_bool shouldCapture =
                  (SDL_bool)((SDL_GetWindowFlags(current_win) &
                              SDL_WINDOW_MOUSE_CAPTURE) == 0);
              const int rc = SDL_CaptureMouse(shouldCapture);
              SDL_Log("%sapturing mouse %s!\n", shouldCapture ? "C" : "Unc",
                      (rc == 0) ? "succeeded" : "failed");
            }
          }
          break;
        case SDLK_v:
          if (withControl) {
            /* Ctrl-V paste awesome text! */
            char *text = SDL_GetClipboardText();
            if (*text) {
              SDL_Log("Clipboard: %s\n", text);
            } else {
              SDL_Log("Clipboard is empty\n");
            }
            SDL_free(text);
          }
          break;
        case SDLK_f:
          if (withControl) {
            /* Ctrl-F flash the window */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              SDL_FlashWindow(window, SDL_FLASH_BRIEFLY);
            }
          }
          break;
        case SDLK_g:
          if (withControl) {
            /* Ctrl-G toggle mouse grab */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              SDL_SetWindowGrab(
                  window, !SDL_GetWindowGrab(window) ? SDL_TRUE : SDL_FALSE);
            }
          }
          break;
        case SDLK_k:
          if (withControl) {
            /* Ctrl-K toggle keyboard grab */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              SDL_SetWindowKeyboardGrab(
                  window,
                  !SDL_GetWindowKeyboardGrab(window) ? SDL_TRUE : SDL_FALSE);
            }
          }
          break;
        case SDLK_m:
          if (withControl) {
            /* Ctrl-M maximize */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              Uint32 flags = SDL_GetWindowFlags(window);
              if (flags & SDL_WINDOW_MAXIMIZED) {
                SDL_RestoreWindow(window);
              } else {
                SDL_MaximizeWindow(window);
              }
            }
          }
          break;
        case SDLK_r:
          if (withControl) {
            /* Ctrl-R toggle mouse relative mode */
            SDL_SetRelativeMouseMode(!SDL_GetRelativeMouseMode() ? SDL_TRUE
                                                                 : SDL_FALSE);
          }
          break;
        case SDLK_t:
          if (withControl) {
            /* Ctrl-T toggle topmost mode */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              Uint32 flags = SDL_GetWindowFlags(window);
              if (flags & SDL_WINDOW_ALWAYS_ON_TOP) {
                SDL_SetWindowAlwaysOnTop(window, SDL_FALSE);
              } else {
                SDL_SetWindowAlwaysOnTop(window, SDL_TRUE);
              }
            }
          }
          break;
        case SDLK_z:
          if (withControl) {
            /* Ctrl-Z minimize */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              SDL_MinimizeWindow(window);
            }
          }
          break;
        case SDLK_RETURN:
          if (withControl) {
            /* Ctrl-Enter toggle fullscreen */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              Uint32 flags = SDL_GetWindowFlags(window);
              if (flags & SDL_WINDOW_FULLSCREEN) {
                SDL_SetWindowFullscreen(window, SDL_FALSE);
              } else {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
              }
            }
          } else if (withAlt) {
            /* Alt-Enter toggle fullscreen desktop */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              Uint32 flags = SDL_GetWindowFlags(window);
              if (flags & SDL_WINDOW_FULLSCREEN) {
                SDL_SetWindowFullscreen(window, SDL_FALSE);
              } else {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
              }
            }
          } else if (withShift) {
            /* Shift-Enter toggle fullscreen desktop / fullscreen */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              Uint32 flags = SDL_GetWindowFlags(window);
              if ((flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ==
                  SDL_WINDOW_FULLSCREEN_DESKTOP) {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
              } else {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
              }
            }
          }

          break;
        case SDLK_b:
          if (withControl) {
            /* Ctrl-B toggle window border */
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            if (window) {
              const Uint32 flags = SDL_GetWindowFlags(window);
              const SDL_bool b =
                  (flags & SDL_WINDOW_BORDERLESS) ? SDL_TRUE : SDL_FALSE;
              SDL_SetWindowBordered(window, b);
            }
          }
          break;
        case SDLK_a:
          if (withControl) {
            /* Ctrl-A reports absolute mouse position. */
            int x, y;
            const Uint32 mask = SDL_GetGlobalMouseState(&x, &y);
            SDL_Log("ABSOLUTE MOUSE: (%d, %d)%s%s%s%s%s\n", x, y,
                    (mask & SDL_BUTTON_LMASK) ? " [LBUTTON]" : "",
                    (mask & SDL_BUTTON_MMASK) ? " [MBUTTON]" : "",
                    (mask & SDL_BUTTON_RMASK) ? " [RBUTTON]" : "",
                    (mask & SDL_BUTTON_X1MASK) ? " [X2BUTTON]" : "",
                    (mask & SDL_BUTTON_X2MASK) ? " [X2BUTTON]" : "");
          }
          break;
        case SDLK_0:
          if (withControl) {
            SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Test Message",
                                     "You're awesome!", window);
          }
          break;
        case SDLK_ESCAPE:
          *done = 1;
          break;
        case SDLK_SPACE: {
          char message[256];
          SDL_Window *window = SDL_GetWindowFromID(event->key.windowID);

          (void)SDL_snprintf(message, sizeof(message),
                             "(%" SDL_PRIs32 ", %" SDL_PRIs32
                             "), rel (%" SDL_PRIs32 ", %" SDL_PRIs32 ")\n",
                             lastEvent.x, lastEvent.y, lastEvent.xrel,
                             lastEvent.yrel);
          SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                                   "Last mouse position", message, window);
          break;
        }
        default:
          break;
      }
      break;
    }
    case SDL_QUIT:
      *done = 1;
      break;
    case SDL_MOUSEMOTION:
      lastEvent = event->motion;
      break;

    case SDL_DROPFILE:
    case SDL_DROPTEXT:
      SDL_free(event->drop.file);
      break;
  }
}

void MP4Demo::loop() {
  int i;
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    this->CommonEvent(m_state, &event, &m_done);
  }
  for (i = 0; i < m_state->num_windows; ++i) {
    if (m_state->windows[i] == NULL) {
      continue;
    }
    Draw(&m_pstate[i]);
  }
#ifdef __EMSCRIPTEN__
  if (done) {
    emscripten_cancel_main_loop();
  }
#endif
}

void MP4Demo::SetDoneFlag(int val) { this->m_done = val; }

int MP4Demo::GetDoneFlag() { return m_done; }

/* Moving Rectangle */
int main_bak(int argc, char *argv[]) {
  int i;

  /* Enable standard application logging */
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

  /* Initialize parameters */
  int num_objects = 100;

  /* Initialize test framework */
  MP4Demo *cur = new MP4Demo();
  CommonState *state = cur->create_default_state(SDL_INIT_VIDEO);
  if (!state) {
    return 1;
  }
  if (!cur->commoninit(state)) {
    return 2;
  }

  PaintState *kStates = SDL_stack_alloc(PaintState, 1);
  for (i = 0; i < state->num_windows; ++i) {
    PaintState *kState = &kStates[i];

    kState->window = state->windows[i];
    kState->renderer = state->renderers[i];
    kState->background =
        cur->LoadTexture(kState->renderer, "sample.bmp", SDL_FALSE, NULL, NULL);
    SDL_QueryTexture(kState->background, NULL, NULL, &kState->sprite_rect.w,
                     &kState->sprite_rect.h);
    kState->scale_direction = 1;
  }
  /* Main render loop */
  int frames = 0;
  Uint32 then = SDL_GetTicks();

  cur->SetCommonState(state);
  cur->SetPaintState(kStates);

  while (!cur->GetDoneFlag()) {
    ++frames;
    cur->loop();
  }
  return 0;
}

long getFileSize(FILE *file) {
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  return size;
}

int main(int argc, char **argv) {
  // 初始化SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
    return -1;
  }

  // 创建窗口
  SDL_Window *window =
      SDL_CreateWindow("YUV Player", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  // 创建渲染器
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // 创建纹理
  SDL_Texture *texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STATIC, 1280, 720);
  if (texture == NULL) {
    fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // 打开YUV文件
  FILE *yuvFile = fopen("output.yuv", "rb");
  if (yuvFile == NULL) {
    fprintf(stderr, "Failed to open YUV file\n");
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // 获取文件大小并计算单帧大小
  long fileSize = getFileSize(yuvFile);
  long width = 1280;
  long height = 720;
  long frameSize = width * height * 3 / 2;  // For YUV420 (YV12)

  // 分配内存读取YUV帧数据
  uint8_t *yuvFrameData = (uint8_t *)malloc(frameSize);
  if (yuvFrameData == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    fclose(yuvFile);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  SDL_Event event;
  int isRunning = 1;
  while (isRunning) {
    // 读取一帧数据
    size_t bytesRead = fread(yuvFrameData, 1, frameSize, yuvFile);
    if (bytesRead != frameSize) {
      if (feof(yuvFile)) {
        // 如果已经读完文件，可以选择重新回到文件开头继续播放
        fseek(yuvFile, 0, SEEK_SET);
      } else {
        fprintf(stderr, "Failed to read a complete frame\n");
        break;
      }
    }

    // 更新纹理 (注意这里YUV数据需要是YV12格式)
    SDL_UpdateTexture(texture, NULL, yuvFrameData, width);

    // 渲染纹理
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // 事件处理
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        isRunning = 0;
      }
    }

    // 控制帧率
    SDL_Delay(40);  // 每帧等待40ms，控制大约25fps
  }

  // 清理资源
  free(yuvFrameData);
  fclose(yuvFile);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}