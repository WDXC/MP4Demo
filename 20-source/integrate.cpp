#include <SDL2/SDL.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <stdio.h>
#include <stdlib.h>

void initFFmpeg(const char *filename, AVCodecContext **codec_ctx,
                AVFrame **frame, AVFormatContext **format_ctx,
                AVCodec **codec) {
  // Open the input file with FFmpeg
  if (avformat_open_input(format_ctx, filename, NULL, NULL) < 0) {
    fprintf(stderr, "Could not open input file\n");
    exit(1);
  }

  if (avformat_find_stream_info(*format_ctx, NULL) < 0) {
    fprintf(stderr, "Could not find stream information\n");
    exit(1);
  }

  // Find the video stream
  int video_stream_index = -1;
  for (int i = 0; i < (*format_ctx)->nb_streams; i++) {
    if ((*format_ctx)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
      break;
    }
  }

  if (video_stream_index == -1) {
    fprintf(stderr, "Could not find video stream\n");
    exit(1);
  }

  // Find the decoder for the video stream
  *codec = const_cast<AVCodec *>(avcodec_find_decoder(
      (*format_ctx)->streams[video_stream_index]->codecpar->codec_id));
  if (!*codec) {
    fprintf(stderr, "Codec not found\n");
    exit(1);
  }

  // Allocate codec context
  *codec_ctx = avcodec_alloc_context3(*codec);
  if (!*codec_ctx) {
    fprintf(stderr, "Could not allocate codec context\n");
    exit(1);
  }

  if (avcodec_parameters_to_context(
          *codec_ctx, (*format_ctx)->streams[video_stream_index]->codecpar) <
      0) {
    fprintf(stderr, "Could not copy codec parameters to context\n");
    exit(1);
  }

  if (avcodec_open2(*codec_ctx, *codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  *frame = av_frame_alloc();
  if (!*frame) {
    fprintf(stderr, "Could not allocate frame\n");
    exit(1);
  }
}

void renderFrame(SDL_Renderer *renderer, SDL_Texture *texture, AVFrame *frame) {
  // Create a SwsContext for pixel format conversion
  struct SwsContext *sws_ctx = sws_getContext(
      frame->width, frame->height, (AVPixelFormat)frame->format, frame->width,
      frame->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

  fprintf(stderr, "frame->width: %d, frame->height: %d\n", frame->width,
          frame->height);
  if (!sws_ctx) {
    fprintf(stderr, "Error creating SwsContext\n");
    return;
  }

  // Allocate memory for the converted frame
  uint8_t *yuv_buffer = (uint8_t *)malloc(frame->width * frame->height * 3 / 2);
  if (yuv_buffer == NULL) {
    fprintf(stderr, "Memory allocation failed for yuv_buffer\n");
    sws_freeContext(sws_ctx);
    return;
  } else {
    fprintf(stderr, "Allocated buffer size: %d bytes\n",
            frame->width * frame->height * 3 / 2);
  }
  int width = frame->width;
  int height = frame->height;

  int y_size = width * height;
  int uv_size = y_size / 4;

  // Set YUV plane pointers
  uint8_t *yuv_planes[3] = {
      yuv_buffer,                    // Y plane
      yuv_buffer + y_size,           // U plane
      yuv_buffer + y_size + uv_size  // V plane
  };

  int yuv_linesize[3] = {
      width,      // Y stride
      width / 2,  // U stride
      width / 2   // V stride
  };

  // Now call sws_scale
  int ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, height,
                      yuv_planes, yuv_linesize);

  if (ret < 0) {
    fprintf(stderr, "Error during sws_scale\n");
    free(yuv_buffer);
    sws_freeContext(sws_ctx);
    return;
  }

  // Update the SDL texture with the converted frame
  ret = SDL_UpdateTexture(texture, NULL, yuv_buffer, frame->width);
  if (ret != 0) {
    fprintf(stderr, "SDL_UpdateTexture failed: %s\n", SDL_GetError());
    free(yuv_buffer);
    sws_freeContext(sws_ctx);
    return;
  }

  // Render the texture to the window
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);

  // Free the YUV buffer after use
  free(yuv_buffer);
  sws_freeContext(sws_ctx);
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
    return -1;
  }

  const char *filename = argv[1];
  AVFormatContext *format_ctx = NULL;
  AVCodecContext *codec_ctx = NULL;
  AVFrame *frame = NULL;
  AVCodec *codec = NULL;
  AVPacket pkt;
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Texture *texture = NULL;
  int isRunning = 1;
  SDL_Event event;

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
    return -1;
  }

  // Create SDL window and renderer
  window =
      SDL_CreateWindow("MP4 Player", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // Create texture for video display
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                              SDL_TEXTUREACCESS_STREAMING, 1280, 720);
  if (texture == NULL) {
    fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  // Initialize FFmpeg
  initFFmpeg(filename, &codec_ctx, &frame, &format_ctx, &codec);

  // Find correct video stream index again
  int video_stream_index = -1;
  for (int i = 0; i < format_ctx->nb_streams; i++) {
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
        !(format_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
      video_stream_index = i;
      break;
    }
  }
  if (video_stream_index == -1) {
    fprintf(stderr, "Could not find video stream\n");
    return -1;
  }

  AVStream *video_stream = format_ctx->streams[video_stream_index];

  // Calculate average frame duration as fallback
  double avg_fps = av_q2d(video_stream->avg_frame_rate);
  double frame_duration_ms = (avg_fps > 0) ? (1000.0 / avg_fps) : 40.0;

  // Print detected frame rate for debugging
  fprintf(stderr, "Detected average frame rate: %.3f fps, expected frame duration: %.2f ms\n", avg_fps, frame_duration_ms);

  // Initialize timing variables
  int64_t last_pts = 0, current_pts = 0;
  uint32_t start_time = SDL_GetTicks();
  uint32_t frame_timer = start_time;

  // Main loop for decoding and rendering frames
  while (isRunning) {
    if (av_read_frame(format_ctx, &pkt) >= 0) {
      if (pkt.stream_index == video_stream_index) {  // Correct video stream

        int ret = avcodec_send_packet(codec_ctx, &pkt);
        if (ret < 0) {
          fprintf(stderr, "Error sending packet to decoder\n");
          break;
        }

        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == 0) {
          current_pts = frame->pts;

          // Compute delay based on PTS difference if available
          int64_t delay_ms;
          if (last_pts != 0 && current_pts > last_pts) {
            double pts_diff = (current_pts - last_pts) * av_q2d(video_stream->time_base);
            delay_ms = (int64_t)(pts_diff * 1000);
          } else {
            // fallback: use average frame duration
            delay_ms = (int64_t)frame_duration_ms;
          }

          // Update expected next frame time
          frame_timer += delay_ms;

          // Calculate actual delay needed to sync with real time
          int64_t actual_delay = frame_timer - SDL_GetTicks();
          if (actual_delay > 0) {
            SDL_Delay((Uint32)actual_delay);
          }

          renderFrame(renderer, texture, frame);  // Render the decoded frame
          last_pts = current_pts;
        }
      }
      av_packet_unref(&pkt);
    }

    // Handle SDL events (e.g., quit)
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        isRunning = 0;
      }
    }
  }

  // Cleanup
  av_frame_free(&frame);
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&format_ctx);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
