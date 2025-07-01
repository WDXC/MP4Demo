#include <SDL2/SDL.h>
#include <SDL2/SDL_config.h>

#include <cstdlib>

static struct {
  SDL_AudioSpec spec;
  Uint8* sound;    /* Pointer to wave data*/
  Uint32 soundLen; /* Length of wave data*/
  int soundpos;    /* Current play position*/
} wave;

static SDL_AudioDeviceID device;

static void quit(int rc) {
  SDL_Quit();
  exit(rc);
}

static void close_audio(void) {
  if (device != 0) {
    SDL_CloseAudioDevice(device);
    device = 0;
  }
}

static void open_audio(void) {
  device = SDL_OpenAudioDevice(NULL, SDL_FALSE, &wave.spec, NULL, 0);
  if (!device) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't opne audio: %s\n",
                 SDL_GetError());
    SDL_FreeWAV(wave.sound);
    quit(2);
  }
  SDL_PauseAudioDevice(device, SDL_FALSE);
}

void SDLCALL fillerup(void* unused, Uint8* stream, int len) {
  Uint8* waveptr;
  int waveleft;

  /* set up the pointers */
  waveptr = wave.sound + wave.soundpos;
  waveleft = wave.soundLen - wave.soundpos;

  /* Go ! */
  while (waveleft <= len) {
    SDL_memcpy(stream, waveptr, waveleft);
    stream += waveleft;
    len -= waveleft;
    waveptr = wave.sound;
    waveleft = wave.soundLen;
    wave.soundpos = 0;
  }
  SDL_memcpy(stream, waveptr, len);
  wave.soundpos += len;
}

static int done = 0;

char* GetNearbyFilename(const char* file) {
  char* base;
  char* path;

  base = SDL_GetBasePath();

  if (base) {
    SDL_RWops* rw;
    size_t len = SDL_strlen(base) + SDL_strlen(file) + 1;

    path = (char*)SDL_malloc(len);

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

    SDL_free(path);
  }

  path = SDL_strdup(file);
  if (!path) {
    SDL_OutOfMemory();
  }

  return path;
}

char* GetResourceFilename(const char* user_specified, const char* def) {
  if (user_specified) {
    char* ret = SDL_strdup(user_specified);

    if (!ret) {
      SDL_OutOfMemory();
    }

    return ret;
  } else {
    return GetNearbyFilename(def);
  }
}

static void reopen_audio(void) {
  close_audio();
  open_audio();
}

int main(int argc, char* argv[]) {
  int i;
  char* filename = NULL;
  SDL_setenv("SDL_AUDIODRIVER", "alsa", 0);

  /* Enable standard application logging */
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

  /** Load the sdl library */
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n",
                 SDL_GetError());
    return 1;
  }

  filename = GetResourceFilename(argc > 1 ? argv[1] : NULL, "sample.wav");

  /** Read file  */
  SDL_RWops* src = SDL_RWFromFile(filename, "rb");
  if (!src) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s\n", SDL_GetError());
    quit(1);
  }

  /* Get the length of the PCM file */
  SDL_RWseek(src, 0, SEEK_END);
  wave.soundLen = SDL_RWtell(src);
  SDL_RWseek(src, 0, SEEK_SET);

  wave.sound = (Uint8*)SDL_malloc(wave.soundLen);

  if (!wave.sound) {
    SDL_OutOfMemory();
    quit(1);
  }

  if (SDL_RWread(src, wave.sound, 1, wave.soundLen) != wave.soundLen) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read PCM data: %s\n",
                 SDL_GetError());
    quit(1);
  }

  SDL_RWclose(src);

  wave.spec.userdata = nullptr;
  wave.spec.format = AUDIO_S16LSB;
  wave.spec.channels = 2;
  wave.spec.samples = 4096;
  wave.spec.callback = fillerup;

  /** show the list of available driver */
  SDL_Log("Avaliable audio drivers;");
  for (i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
    SDL_Log("%i: %s", i, SDL_GetAudioDriver(i));
  }

  SDL_Log("Using audio driver: %s\n", SDL_GetCurrentAudioDriver());

  open_audio();

  SDL_FlushEvents(SDL_AUDIODEVICEADDED, SDL_AUDIODEVICEREMOVED);

  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) {
      if (event.type == SDL_QUIT) {
        done = 1;
      }
      if ((event.type == SDL_AUDIODEVICEADDED && !event.adevice.iscapture) ||
          (event.type == SDL_AUDIODEVICEREMOVED && !event.adevice.iscapture &&
           event.adevice.which == device)) {
        reopen_audio();
      }
    }
    SDL_Delay(100);
  }

  close_audio();
  SDL_free(filename);
  SDL_Quit();
  return 0;
}