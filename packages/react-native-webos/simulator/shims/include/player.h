#pragma once

/* The part of Tizen 9.0's media/player.h the host calls, with the rootstrap's
 * values. The simulator has no player: player_create fails, so a VideoPlayer
 * reports an error instead of drawing anything. */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct player_s *player_h;
typedef void *player_display_h;

#define GET_DISPLAY(x) (void *)(x)

#define PLAYER_ERROR_CLASS (-0x01940000 | 0x20)

typedef enum {
  PLAYER_STATE_NONE,
  PLAYER_STATE_IDLE,
  PLAYER_STATE_READY,
  PLAYER_STATE_PLAYING,
  PLAYER_STATE_PAUSED,
} player_state_e;

typedef enum {
  PLAYER_ERROR_NONE = 0,
  PLAYER_ERROR_OUT_OF_MEMORY = -ENOMEM,
  PLAYER_ERROR_INVALID_PARAMETER = -EINVAL,
  PLAYER_ERROR_NO_SUCH_FILE = -ENOENT,
  PLAYER_ERROR_INVALID_OPERATION = -ENOSYS,
  PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE = -ENOSPC,
  PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE = -1073741822,
  PLAYER_ERROR_SEEK_FAILED = PLAYER_ERROR_CLASS | 0x01,
  PLAYER_ERROR_INVALID_STATE = PLAYER_ERROR_CLASS | 0x02,
  PLAYER_ERROR_NOT_SUPPORTED_FILE = PLAYER_ERROR_CLASS | 0x03,
  PLAYER_ERROR_INVALID_URI = PLAYER_ERROR_CLASS | 0x04,
  PLAYER_ERROR_SOUND_POLICY = PLAYER_ERROR_CLASS | 0x05,
  PLAYER_ERROR_CONNECTION_FAILED = PLAYER_ERROR_CLASS | 0x06,
  PLAYER_ERROR_VIDEO_CAPTURE_FAILED = PLAYER_ERROR_CLASS | 0x07,
  PLAYER_ERROR_DRM_EXPIRED = PLAYER_ERROR_CLASS | 0x08,
  PLAYER_ERROR_DRM_NO_LICENSE = PLAYER_ERROR_CLASS | 0x09,
  PLAYER_ERROR_DRM_FUTURE_USE = PLAYER_ERROR_CLASS | 0x0a,
  PLAYER_ERROR_DRM_NOT_PERMITTED = PLAYER_ERROR_CLASS | 0x0b,
  PLAYER_ERROR_RESOURCE_LIMIT = PLAYER_ERROR_CLASS | 0x0c,
  PLAYER_ERROR_PERMISSION_DENIED = -EACCES,
  PLAYER_ERROR_SERVICE_DISCONNECTED = PLAYER_ERROR_CLASS | 0x0d,
  PLAYER_ERROR_BUFFER_SPACE = -ENOBUFS,
  PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC = PLAYER_ERROR_CLASS | 0x0e,
  PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC = PLAYER_ERROR_CLASS | 0x0f,
  PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE = PLAYER_ERROR_CLASS | 0x10,
  PLAYER_ERROR_NOT_SUPPORTED_FORMAT = PLAYER_ERROR_CLASS | 0x11,
  PLAYER_ERROR_NOT_AVAILABLE = PLAYER_ERROR_CLASS | 0x12,
} player_error_e;

typedef enum {
  PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT = 4,
} player_interrupted_code_e;

typedef enum {
  PLAYER_DISPLAY_TYPE_OVERLAY = 0,
  PLAYER_DISPLAY_TYPE_EVAS = 3,
  PLAYER_DISPLAY_TYPE_NONE = 4,
  PLAYER_DISPLAY_TYPE_OVERLAY_SYNC_UI = 5,
} player_display_type_e;

typedef enum {
  PLAYER_STREAM_TYPE_DEFAULT,
  PLAYER_STREAM_TYPE_AUDIO,
  PLAYER_STREAM_TYPE_VIDEO,
  PLAYER_STREAM_TYPE_TEXT,
} player_stream_type_e;

typedef enum {
  PLAYER_DISPLAY_MODE_LETTER_BOX = 0,
  PLAYER_DISPLAY_MODE_ORIGIN_SIZE,
  PLAYER_DISPLAY_MODE_FULL_SCREEN,
  PLAYER_DISPLAY_MODE_CROPPED_FULL,
  PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER,
  PLAYER_DISPLAY_MODE_DST_ROI,
  PLAYER_DISPLAY_MODE_NUM,
} player_display_mode_e;

typedef void (*player_subtitle_updated_cb)(unsigned long duration, char *text, void *user_data);
typedef void (*player_prepared_cb)(void *user_data);
typedef void (*player_completed_cb)(void *user_data);
typedef void (*player_seek_completed_cb)(void *user_data);
typedef void (*player_interrupted_cb)(player_interrupted_code_e code, void *user_data);
typedef void (*player_error_cb)(int error_code, void *user_data);
typedef void (*player_video_stream_changed_cb)(int width, int height, int fps, int bit_rate, void *user_data);
typedef void (*player_buffering_cb)(int percent, void *user_data);

int player_create(player_h *player);
int player_destroy(player_h player);
int player_prepare_async(player_h player, player_prepared_cb callback, void *user_data);
int player_unprepare(player_h player);
int player_set_uri(player_h player, const char *uri);
int player_get_state(player_h player, player_state_e *state);
int player_set_volume(player_h player, float left, float right);
int player_start(player_h player);
int player_stop(player_h player);
int player_pause(player_h player);
int player_set_play_position(
    player_h player, int milliseconds, bool accurate, player_seek_completed_cb callback, void *user_data);
int player_get_play_position(player_h player, int *milliseconds);
int player_set_mute(player_h player, bool muted);
int player_set_looping(player_h player, bool looping);
int player_set_display(player_h player, player_display_type_e type, player_display_h display);
int player_set_display_mode(player_h player, player_display_mode_e mode);
int player_set_display_roi_area(player_h player, int x, int y, int width, int height);
int player_set_display_visible(player_h player, bool visible);
int player_set_video_roi_area(player_h player, double x_scale, double y_scale, double w_scale, double h_scale);
int player_get_codec_info(player_h player, char **audio_codec, char **video_codec);
int player_get_video_stream_info(player_h player, int *fps, int *bit_rate);
int player_get_video_size(player_h player, int *width, int *height);
int player_get_duration(player_h player, int *milliseconds);
int player_set_streaming_buffering_time(player_h player, int prebuffer_ms, int rebuffer_ms);
int player_get_streaming_download_progress(player_h player, int *start, int *end);
int player_set_streaming_cookie(player_h player, const char *cookie, int size);
int player_set_streaming_user_agent(player_h player, const char *user_agent, int size);
int player_set_completed_cb(player_h player, player_completed_cb callback, void *user_data);
int player_unset_completed_cb(player_h player);
int player_set_interrupted_cb(player_h player, player_interrupted_cb callback, void *user_data);
int player_unset_interrupted_cb(player_h player);
int player_set_error_cb(player_h player, player_error_cb callback, void *user_data);
int player_unset_error_cb(player_h player);
int player_set_buffering_cb(player_h player, player_buffering_cb callback, void *user_data);
int player_unset_buffering_cb(player_h player);
int player_set_playback_rate(player_h player, float rate);
int player_set_subtitle_updated_cb(player_h player, player_subtitle_updated_cb callback, void *user_data);
int player_unset_subtitle_updated_cb(player_h player);
int player_set_video_stream_changed_cb(player_h player, player_video_stream_changed_cb callback, void *user_data);
int player_unset_video_stream_changed_cb(player_h player);
int player_get_current_track(player_h player, player_stream_type_e type, int *index);
int player_get_track_language_code(player_h player, player_stream_type_e type, int index, char **code);
int player_get_track_count(player_h player, player_stream_type_e type, int *count);
int player_select_track(player_h player, player_stream_type_e type, int index);
int player_set_max_adaptive_variant_limit(player_h player, int bandwidth, int width, int height);

#ifdef __cplusplus
}
#endif
