#include "player.h"
#include "SDL3/SDL_events.h"
#include "clock.h"
#include "defs.h"
#include "ui.h"

f32   *inSamples = NULL;
f32   *out = NULL;
f32   *in = NULL;
cmplx *fft_out = NULL;
cmplx *fft_out_woz = NULL;
size_t frames_size = 0;

box_t *prev_song_button = NULL;
box_t *next_song_button = NULL;
box_t *total_time_box = NULL;
box_t *time_left_box = NULL;
box_t *progress_bar = NULL;
box_t *play_button = NULL;
box_t *pb_state_button = NULL;
box_t *slider = NULL;
box_t *sidebar_box = NULL;
box_t **sidebar_box_arr = NULL;
size_t sidebar_box_arr_size = 0;
SDL_FRect sidebar_rect = {0};
SDL_Texture *button_textures[9] = {0};

f32 progress_bar_h = 20;
f32 progress_bar_w = 300;

playlist_t test_pl = {0};

ma_result err;

void pb_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    ma_vars_t *ma_vars = (ma_vars_t*)pDevice->pUserData;
    if (ma_vars->pb_state & PB_PLAYING) {
        err = ma_decoder_read_pcm_frames(&ma_vars->decoder, pOutput, frameCount, NULL);

//      if (err == MA_AT_END) {
//          if (ma_vars->pb_state & PB_LOOPING) {
//              ma_decoder_seek_to_pcm_frame(&ma_vars->decoder, 0);
//          }
//          if (ma_vars->pb_state & PB_ONCE) {
//              printf("Playback has finished!\n"); 
//              exit(1);
//          }
//      }
//      else if (err != MA_SUCCESS) {
//          fprintf(stderr, "An error has ocurred. ma_result: %d\n", err);
//          exit(1);
//      }

//      out = (f32*)pOutput;
//      fft_out = (cmplx*)malloc(sizeof(cmplx) * frameCount);
//      fft(out, 1, fft_out, frameCount);
    } else if (ma_vars->pb_state & PB_PAUSED) {

    }
    ma_decoder_get_cursor_in_pcm_frames(&ma_vars->decoder, &ma_vars->pb_info.cursor);
    frames_size = frameCount;

    (void)pInput;
}

void init_player(SDL_Renderer *renderer, ma_vars_t *ma_vars) {
    button_textures[BUTTON_PLAY] = IMG_LoadTexture(renderer, "assets/buttons/play.png");
    button_textures[BUTTON_PAUSE] = IMG_LoadTexture(renderer, "assets/buttons/pause.png");
    button_textures[BUTTON_NEXT_SONG] = IMG_LoadTexture(renderer, "assets/buttons/next.png");
    button_textures[BUTTON_PREV_SONG] = IMG_LoadTexture(renderer, "assets/buttons/prev.png");
    button_textures[BUTTON_SLIDER] = IMG_LoadTexture(renderer, "assets/buttons/slider.png");
    button_textures[BUTTON_ONCE] = IMG_LoadTexture(renderer, "assets/buttons/once.png");
    button_textures[BUTTON_LOOP] = IMG_LoadTexture(renderer, "assets/buttons/loop.png");
    button_textures[BUTTON_SHUFFLE] = IMG_LoadTexture(renderer, "assets/buttons/shuffle.png");
    button_textures[BUTTON_SIDEBAR] = IMG_LoadTexture(renderer, "assets/buttons/sidebar.png");

    sidebar_box = create_box(renderer, 
                              (SDL_FRect) {
                                    .x = 0,
                                    .y = 0,
                                    .w = 0,
                                    .h = WIN_HEIGHT, 
                              },
                              WHITE,
                              NULL,
                              NULL,
                              WHITE,
                              NULL,
                              BOX_BORDER
                     ); 

    progress_bar = create_box(renderer, 
                              (SDL_FRect) {
                                    .x = ((f32)WIN_WIDTH/2) - progress_bar_w/2,
                                    .y = ((f32)WIN_HEIGHT/2),
                                    .w = progress_bar_w,
                                    .h = 10, 
                              },
                              WHITE,
                              NULL,
                              NULL,
                              WHITE,
                              NULL,
                              BOX_VISIBLE | BOX_COLOR_FILL
                    ); 

    time_left_box = create_box(renderer, 
                              (SDL_FRect) {
                                .w = 60,
                                .h = 32,
                                .x = (progress_bar->rect.x - 60) - 32,
                                .y = (progress_bar->rect.y + progress_bar->rect.h/2) - 32/2,
                              },
                              WHITE,
                              NULL,
                              "00:00:00",
                              WHITE,
                              NULL,
                              BOX_VISIBLE
                  ); 

    total_time_box = create_box(renderer, 
                              (SDL_FRect) {
                                .w = 60,
                                .h = 32,
                                .x = (progress_bar->rect.x + progress_bar->rect.w) + 32,
                                .y = (progress_bar->rect.y + progress_bar->rect.h/2) - 32/2,
                              },
                              WHITE,
                              NULL,
                              "00:00:00",
                              WHITE,
                              NULL,
                              BOX_VISIBLE
                  ); 

    play_button = create_box(renderer, 
                              (SDL_FRect) {
                                .w = 32,
                                .h = 32,
                                .x = ((f32)WIN_WIDTH/2) - 16,
                                .y = (f32)WIN_HEIGHT/2 + 32,
                              },
                              WHITE,
                              button_textures[BUTTON_PLAY],
                              NULL,
                              WHITE,
                              NULL,
                              BOX_VISIBLE
                  ); 

    slider = create_box(renderer, 
                              (SDL_FRect) {
                                  .w = 32,
                                  .h = 32,
                                  .x = 0,
                                  .y = 0,
                              },
                              WHITE,
                              button_textures[BUTTON_SLIDER],
                              NULL,
                              WHITE,
                              NULL,
                              BOX_VISIBLE
                              ); 

    prev_song_button = create_box(renderer, 
                              (SDL_FRect) {
                                .x = play_button->rect.x - play_button->rect.w*2,
                                .y = (f32)WIN_HEIGHT/2 + 32,
                                .w = 32,
                                .h = 32,
                              },
                              WHITE,
                              button_textures[BUTTON_PREV_SONG],
                              NULL,
                              WHITE,
                              NULL,
                              BOX_VISIBLE
                              ); 
    next_song_button = create_box(renderer, 
                              (SDL_FRect) {
                                .x = play_button->rect.x+play_button->rect.w*2,
                                .y = (f32)WIN_HEIGHT/2 + 32,
                                .w = 32,
                                .h = 32,
                              },
                              WHITE,
                              button_textures[BUTTON_NEXT_SONG],
                              NULL,
                              WHITE,
                              NULL,
                              BOX_VISIBLE
                              ); 

    pb_state_button = create_box(renderer, 
                              (SDL_FRect) {
                                .x = ((f32)WIN_WIDTH/2) - 16,
                                .y = (f32)WIN_HEIGHT/2 + 96,
                                .w = 32,
                                .h = 32,
                              },
                              WHITE,
                              button_textures[BUTTON_ONCE],
                              NULL,
                              WHITE,
                              NULL,
                              BOX_VISIBLE
                              ); 

    for (size_t i = 0; i < ma_vars->playlist.mp3_list_size; i++) {
        new_sidebar_item(renderer, ma_vars);
    }
    ma_vars->pb_state |= PB_ONCE;
    play_mp3(ma_vars->playlist.mp3_list[ma_vars->playlist.current_mp3], ma_vars);
}

void pause_pb(pb_state *state) {
    *state &= ~PB_PLAYING;
    *state |= PB_PAUSED;
}

void unpause_pb(pb_state *state) {
    *state &= ~PB_PAUSED;
    *state |= PB_PLAYING;
}

void repos_buttons() {
    progress_bar->rect.x     = (sidebar_box->rect.x + sidebar_box->rect.w) + ((WIN_WIDTH - (sidebar_box->rect.x +sidebar_box->rect.w)) - progress_bar->rect.w)/2;
    time_left_box->rect.x    = (progress_bar->rect.x - time_left_box->rect.w) - 32;
    total_time_box->rect.x   = (progress_bar->rect.x + progress_bar->rect.w) + 32;
    play_button->rect.x      = (progress_bar->rect.x) + (progress_bar->rect.w - play_button->rect.w)/2;
    prev_song_button->rect.x = play_button->rect.x - play_button->rect.w * 2;
    next_song_button->rect.x = play_button->rect.x + play_button->rect.w * 2;
    pb_state_button->rect.x  = play_button->rect.x;

}

void open_sidebar(SDL_Renderer *renderer) {
    for (size_t i = 0; i < sidebar_box_arr_size; i++) {
        sidebar_box_arr[i]->rect = (SDL_FRect) {
                                        .w = sidebar_box->rect.w,
                                        .h = sidebar_rect.h,
                                        .x = 0,
                                        .y = (sidebar_rect.y + sidebar_rect.h + sidebar_rect.y) + (i * sidebar_rect.h)
                                    }; 
        sidebar_box_arr[i]->state |= BOX_VISIBLE;
    }

}

void close_sidebar(SDL_Renderer *renderer) {
    for (size_t i = 0; i < sidebar_box_arr_size; i++) {
        sidebar_box_arr[i]->rect = (SDL_FRect) {
                                        .w = 0,
                                        .h = 0,
                                        .x = 0,
                                        .y = 0 
                                    }; 
        sidebar_box_arr[i]->state &= ~BOX_VISIBLE;
    }

}

void update_sidebar(SDL_Renderer *renderer, mouse_t mouse) {
    sidebar_rect = (SDL_FRect) {
                        .w = 32, 
                        .h = 32, 
                        .x = 8, 
                        .y = 8, 
    }; 
    if (check_mouse_rect_collision(mouse, sidebar_rect)) {
        SDL_SetTextureColorMod(button_textures[BUTTON_SIDEBAR], 150, 150, 150);
        if (mouse_clicked(mouse)) {
            if (sidebar_box->state & BOX_VISIBLE) {
                sidebar_box->state &= ~BOX_VISIBLE;
                sidebar_box->rect.w = 0;
                close_sidebar(renderer);
                repos_buttons();
            } else {
                sidebar_box->state |= BOX_VISIBLE;
                sidebar_box->rect.w = 200;
                open_sidebar(renderer);
                repos_buttons();
            }
        }
    } else {
        SDL_SetTextureColorMod(button_textures[BUTTON_SIDEBAR], 255, 255, 255);
    }

}

void render_sidebar(SDL_Renderer *renderer, mouse_t mouse) {
    SDL_RenderTexture(renderer, button_textures[BUTTON_SIDEBAR], NULL, &sidebar_rect);

}

void play_mp3(mp3_t mp3, ma_vars_t *ma_vars) {
    ma_result err = {0};
    // CHECK CHECK CHECK
    ma_decoder_uninit(&ma_vars->decoder);
    pause_pb(&ma_vars->pb_state);

//  char *str = NULL;
//  str = strcat(mp3.dir, mp3.filename);

    printf("dir: %s\n", mp3.dir);
    err = ma_decoder_init_file(mp3.dir, &ma_vars->decoder_config, &ma_vars->decoder); 

    if (err != MA_SUCCESS) {
        fprintf(stderr, "Failed to open '%s' mp3 file. MA_ERROR: %d\n", mp3.filename, err);
        exit(1);
    }

    ma_vars->device_config.playback.format   = ma_vars->decoder.outputFormat;
    ma_vars->device_config.playback.channels = ma_vars->decoder.outputChannels;
    ma_vars->device_config.sampleRate        = ma_vars->decoder.outputSampleRate;
    ma_decoder_get_length_in_pcm_frames(&ma_vars->decoder, &ma_vars->pb_info.current_mp3.frames);

    switch (ma_vars->decoder.outputFormat) {
    case ma_format_u8:
        ma_vars->pb_info.current_mp3.format = "uint8";
       break; 
    case ma_format_s16:
        ma_vars->pb_info.current_mp3.format = "int16";
       break; 
    case ma_format_s24:
        ma_vars->pb_info.current_mp3.format = "int24";
       break; 
    case ma_format_s32:
        ma_vars->pb_info.current_mp3.format = "int32";
       break; 
    case ma_format_f32:
        ma_vars->pb_info.current_mp3.format = "float32";
        break; 
    default:
       break; 
    }

    strcpy(ma_vars->pb_info.current_mp3.filename, mp3.filename);
    ma_vars->pb_info.current_mp3.sample_rate = ma_vars->decoder.outputSampleRate;
    ma_vars->pb_info.current_mp3.channels = ma_vars->decoder.outputChannels;
    ma_vars->pb_info.last_cursor = 0;

    ma_vars->pb_state &= ~PB_PAUSED;
    ma_vars->pb_state |= PB_PLAYING;

    char time[10] = {0};

    time_24hrs(time, ma_vars->pb_info.current_mp3.frames/ma_vars->pb_info.current_mp3.sample_rate);
    strcpy(total_time_box->text, time+3);
    total_time_box->new_text = true;
}



playlist_t create_playlist(const char *dir_name) {
    playlist_t result = {
        .name = "playlist",
        .dir = dir_name,
        .mp3_list = NULL,
        .mp3_list_size = 0,
        .current_mp3 = 0,
    };
 
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        fprintf(stderr, "Could not open directory\n");
    }

    de de = {0};
    
    de = readdir(dir);
    while (de != NULL) {
        if (de->d_type == DT_REG) {
            if (check_file_mp3(de->d_name)) {
                result.mp3_list = realloc(result.mp3_list, sizeof(mp3_t) * (result.mp3_list_size+1));
                strcpy(result.mp3_list[result.mp3_list_size].filename, de->d_name); 
                char str[128] = {0};
                strcpy(str, dir_name);
                strcat(str, de->d_name);
                strcpy(result.mp3_list[result.mp3_list_size].dir, str);
                
                printf("result %zu: %s\n", result.mp3_list_size, result.mp3_list[result.mp3_list_size].filename);
                result.mp3_list_size++;
            }
        }
        de = readdir(dir);
    }
    closedir(dir);

    return result;
}

void new_sidebar_item(SDL_Renderer *renderer, ma_vars_t *ma_vars) {
    sidebar_box_arr = realloc(sidebar_box_arr, sizeof(box_t*) * (sidebar_box_arr_size+1));
    char file[128] = {0};
    strncpy(file, ma_vars->playlist.mp3_list[sidebar_box_arr_size].filename, strlen(ma_vars->playlist.mp3_list[sidebar_box_arr_size].filename)-4);

    sidebar_box_arr[sidebar_box_arr_size] = create_box(renderer, 
                                  (SDL_FRect) {
                                    .x = 0,
                                    .y = 0,
                                    .w = 0,
                                    .h = 0,
                                  },
                                    WHITE, 
                                    NULL, 
                                    file,//test_pl.mp3_list[i].filename, 
                                    WHITE, 
                                    NULL, 
                                    BOX_BORDER);
    sidebar_box_arr_size++;
}

void add_mp3_to_playlist(SDL_Renderer *renderer, ma_vars_t *ma_vars, const char *filename) {
    ma_vars->playlist.mp3_list = realloc(ma_vars->playlist.mp3_list, sizeof(mp3_t) * (ma_vars->playlist.mp3_list_size+1));
    mp3_t mp3 = {
        .frames = 0,
        .sample_rate = 0,
        .channels = 0
    };

    strcpy(mp3.dir, filename);
    char *str = strrchr(filename, '/')+1;
    strncpy(mp3.filename, str, strlen(str)-4);

    ma_vars->playlist.mp3_list[ma_vars->playlist.mp3_list_size] = mp3; 
    ma_vars->playlist.mp3_list_size++;
    new_sidebar_item(renderer, ma_vars);

}

void update_pb(SDL_Event event, SDL_Renderer *renderer, ma_vars_t *ma_vars, mouse_t mouse) {
    f32 sec = ((f32)ma_vars->pb_info.cursor/SAMPLE_RATE);
    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.keysym.sym == SDLK_SPACE) {
            if (ma_vars->pb_state & PB_PLAYING) {
                pause_pb(&ma_vars->pb_state);
            } else {
                unpause_pb(&ma_vars->pb_state);
            }
        }
        // CHECK CHECK CHECK CHECK CHECK
        if (event.key.keysym.sym == SDLK_RIGHT) {
            pause_pb(&ma_vars->pb_state);
            i32 t = (ma_vars->pb_info.cursor/ma_vars->pb_info.current_mp3.sample_rate) + 5;
            if (t+5 >= (i32)(ma_vars->pb_info.current_mp3.frames/ma_vars->pb_info.current_mp3.sample_rate)) {
                t = ma_vars->pb_info.current_mp3.frames/ma_vars->pb_info.current_mp3.sample_rate;
            } else {
                t += 5;
            }
            ma_vars->pb_info.cursor = t * ma_vars->pb_info.current_mp3.sample_rate;
            ma_decoder_seek_to_pcm_frame(&ma_vars->decoder, ma_vars->pb_info.cursor);
            unpause_pb(&ma_vars->pb_state);
        }
        if (event.key.keysym.sym == SDLK_LEFT) {
            pause_pb(&ma_vars->pb_state);
            i32 t = (ma_vars->pb_info.cursor/ma_vars->pb_info.current_mp3.sample_rate);
            if (t-5 <= 0) {
                t = 0;
            } else {
                t -= 5;
            }
            ma_vars->pb_info.cursor = t * ma_vars->pb_info.current_mp3.sample_rate;
            ma_decoder_seek_to_pcm_frame(&ma_vars->decoder, ma_vars->pb_info.cursor);
            unpause_pb(&ma_vars->pb_state);
        }

    } else if (event.type == SDL_EVENT_KEY_UP) {


    }

    if (event.type == SDL_EVENT_DROP_FILE) {
        const char *file = event.drop.data;
        printf("Dropped: %s\n", file);
        add_mp3_to_playlist(renderer, ma_vars, file);

    }



    if (err == MA_AT_END) {
        if (ma_vars->pb_state & PB_LOOPING) {
            pause_pb(&ma_vars->pb_state);
            ma_decoder_seek_to_pcm_frame(&ma_vars->decoder, 0);
            unpause_pb(&ma_vars->pb_state);
        }
        if (ma_vars->pb_state & PB_ONCE) {
            printf("Playback has finished!\n"); 
            exit(1);
        }
        if (ma_vars->pb_state & PB_SHUFFLE) {
            size_t ran = rand()%ma_vars->playlist.mp3_list_size;

            pause_pb(&ma_vars->pb_state);
            play_mp3(ma_vars->playlist.mp3_list[ran], ma_vars);
            unpause_pb(&ma_vars->pb_state);
            ma_vars->playlist.current_mp3 = ran;
        }
    } else if (err != MA_SUCCESS) {
        fprintf(stderr, "An error has ocurred. ma_result: %d\n", err);
        exit(1);
    }

    for (size_t i = 0; i < sidebar_box_arr_size; i++) {
        if (sidebar_box_arr[i]->state & BOX_HOVERED) {
            SDL_SetTextureColorMod(sidebar_box_arr[i]->font_texture, 150, 150, 150);
            if (mouse_clicked(mouse) && i != ma_vars->playlist.current_mp3) {
                play_mp3(ma_vars->playlist.mp3_list[i], ma_vars); 
                ma_vars->playlist.current_mp3 = i;
            }
        } else {
            SDL_SetTextureColorMod(sidebar_box_arr[i]->font_texture, 255, 255, 255);
        }
    }
    SDL_SetTextureColorMod(sidebar_box_arr[ma_vars->playlist.current_mp3]->font_texture, 150, 150, 150);

    char time[10] = {0};
    time_24hrs(time, ma_vars->pb_info.cursor/ma_vars->pb_info.current_mp3.sample_rate);
    strcpy(time_left_box->text, time+3);
    time_left_box->new_text = true;

}

void render_pb(SDL_Renderer *renderer, mouse_t mouse, ma_vars_t *ma_vars) { 
    slider->rect.x = (progress_bar->rect.x + ((f32)progress_bar_w/ma_vars->pb_info.current_mp3.frames) * ma_vars->pb_info.cursor) - 16;
    slider->rect.y = progress_bar->rect.y - 10;

    if (progress_bar->state & BOX_HOVERED) {
        if (mouse_clicked(mouse)) {
            pause_pb(&ma_vars->pb_state);
            u64 pos = ((ma_vars->pb_info.current_mp3.frames * (mouse.pos.x - progress_bar->rect.x) )/progress_bar->rect.w);
            ma_decoder_seek_to_pcm_frame(&ma_vars->decoder, pos); 
            unpause_pb(&ma_vars->pb_state);
        } 
    }

    if (slider->state & BOX_HOVERED) {
        SDL_SetTextureColorMod(slider->texture, 150, 150, 150);
    } else {
        SDL_SetTextureColorMod(slider->texture, 255, 255, 255);
    }

    if (next_song_button->state & BOX_HOVERED) {
        if (mouse_clicked(mouse)) {    
            size_t t = 0;
            if (ma_vars->pb_state & PB_SHUFFLE) {
                t = rand()%ma_vars->playlist.mp3_list_size; 
            } else if (ma_vars->playlist.current_mp3 < ma_vars->playlist.mp3_list_size-1) {
                t = ma_vars->playlist.current_mp3 + 1;
            } else {
                t = ma_vars->playlist.current_mp3 = 0;
            }
            play_mp3(ma_vars->playlist.mp3_list[t], ma_vars);
            ma_vars->playlist.current_mp3 = t;
        }
        SDL_SetTextureColorMod(next_song_button->texture, 150, 150, 150);
    } else {
        SDL_SetTextureColorMod(next_song_button->texture, 255, 255, 255);
    }
    if (prev_song_button->state & BOX_HOVERED) {
        if (mouse_clicked(mouse)) {    
            size_t t = 0;
            if (ma_vars->pb_state & PB_SHUFFLE) {
                t = rand()%ma_vars->playlist.mp3_list_size; 
            } else if (ma_vars->playlist.current_mp3 > 0) {
                t = ma_vars->playlist.current_mp3-1;
            } else {
                t = ma_vars->playlist.current_mp3 = ma_vars->playlist.mp3_list_size-1;
            }
            play_mp3(ma_vars->playlist.mp3_list[t], ma_vars);
            ma_vars->playlist.current_mp3 = t;
        }
        SDL_SetTextureColorMod(prev_song_button->texture, 150, 150, 150);
    } else {
        SDL_SetTextureColorMod(prev_song_button->texture, 255, 255, 255);
    }

    if (pb_state_button->state & BOX_HOVERED) {
        if (mouse_clicked(mouse)) {
            if (ma_vars->pb_state & PB_ONCE) {
                ma_vars->pb_state &= ~PB_ONCE;
                ma_vars->pb_state |= PB_LOOPING;
                pb_state_button->texture = button_textures[BUTTON_LOOP];
            } else if (ma_vars->pb_state & PB_SHUFFLE) {
                ma_vars->pb_state &= ~PB_SHUFFLE;
                ma_vars->pb_state |= PB_ONCE;
                pb_state_button->texture = button_textures[BUTTON_ONCE];
            } else if (ma_vars->pb_state & PB_LOOPING) {
                ma_vars->pb_state &= ~PB_LOOPING;
                ma_vars->pb_state |= PB_SHUFFLE;
                pb_state_button->texture = button_textures[BUTTON_SHUFFLE];
            }

        }
        SDL_SetTextureColorMod(pb_state_button->texture, 150, 150, 150);
    } else {
        SDL_SetTextureColorMod(pb_state_button->texture, 255, 255, 255);
    }

    if (ma_vars->pb_state & PB_PLAYING) {
        play_button->texture = button_textures[BUTTON_PAUSE];
    }
    if (ma_vars->pb_state & PB_PAUSED) {
        play_button->texture = button_textures[BUTTON_PLAY];
    }

    if (play_button->state & BOX_HOVERED) {
        if (mouse_clicked(mouse)) {
            if (ma_vars->pb_state & PB_PLAYING) {
                pause_pb(&ma_vars->pb_state);
            } else {
                unpause_pb(&ma_vars->pb_state);
            }
        }
        SDL_SetTextureColorMod(play_button->texture, 150, 150, 150);
    } else {
        SDL_SetTextureColorMod(play_button->texture, 255, 255, 255);
    }

}

bool check_file_mp3(const char *file) {
    size_t len = strlen(file);
    if (!strcasecmp(file+(len-3), "mp3")) {
        return true;
    }

    return false;
}

bool check_file_wav(const char *file) {
    size_t len = strlen(file);
    if (!strcasecmp(file+(len-3), "wav")) {
        return true;
    }

    return false;
}

void print_playlist(playlist_t playlist) {
    printf("[Playlist: %s]\n", playlist.name);
    for (size_t i = 0; i < playlist.mp3_list_size; i++) {
        printf("File %zu: %s\n", i, playlist.mp3_list[i].filename);
    }
}

void print_pb_info(pb_info pb_info) {
    u32 cursor_to_ms = pb_info.cursor / SAMPLE_RATE;
    u32 frames_to_ms = pb_info.current_mp3.frames / SAMPLE_RATE;
    char file[128] = {0};
    char time[10] = {0};
    strncpy(file, pb_info.current_mp3.filename, strlen(pb_info.current_mp3.filename)-4);

    printf("[PLAYBACK INFO]\n");
    printf("  File: %s\n", file);
    printf("  Format: %s\n", pb_info.current_mp3.format);
    printf("  Sample rate: %uhz\n", pb_info.current_mp3.sample_rate);
    printf("  Channels: %u\n", pb_info.current_mp3.channels);
    time_24hrs(time, frames_to_ms);
    printf("  Duration: %s\n", time);
    printf("  Frames cursor: %llu\n", pb_info.cursor);
    printf("  Last frames cursor: %llu\n", pb_info.last_cursor);
    time_24hrs(time, cursor_to_ms);
    printf("  Progress: %s", time); 

    printf("\n");
}

void print_pb_state(pb_state pb_state) {
    printf("[PLAYBACK STATE]\n");
    printf("  Playing: %d\n", pb_state&PB_PLAYING);
    printf("  Paused: %d\n", pb_state&PB_PAUSED);
    printf("  Once: %d\n", pb_state&PB_ONCE);
    printf("  Loop: %d\n", pb_state&PB_LOOPING);
    printf("  Shuffle: %d\n", pb_state&PB_SHUFFLE);

    printf("\n");
}
