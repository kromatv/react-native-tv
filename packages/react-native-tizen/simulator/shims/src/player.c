#include <player.h>

#include <stddef.h>

int player_create(player_h *player) {
  if (player != NULL) *player = NULL;
  return PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE;
}

int player_destroy(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_prepare_async(player_h player, player_prepared_cb callback, void *user_data) {
  (void)player;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_unprepare(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_uri(player_h player, const char *uri) {
  (void)player;
  (void)uri;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_state(player_h player, player_state_e *state) {
  (void)player;
  (void)state;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_volume(player_h player, float left, float right) {
  (void)player;
  (void)left;
  (void)right;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_start(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_stop(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_pause(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_play_position(player_h player, int milliseconds, bool accurate, player_seek_completed_cb callback, void *user_data) {
  (void)player;
  (void)milliseconds;
  (void)accurate;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_play_position(player_h player, int *milliseconds) {
  (void)player;
  (void)milliseconds;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_mute(player_h player, bool muted) {
  (void)player;
  (void)muted;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_looping(player_h player, bool looping) {
  (void)player;
  (void)looping;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_display(player_h player, player_display_type_e type, player_display_h display) {
  (void)player;
  (void)type;
  (void)display;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_display_mode(player_h player, player_display_mode_e mode) {
  (void)player;
  (void)mode;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_display_roi_area(player_h player, int x, int y, int width, int height) {
  (void)player;
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_display_visible(player_h player, bool visible) {
  (void)player;
  (void)visible;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_video_roi_area(player_h player, double x_scale, double y_scale, double w_scale, double h_scale) {
  (void)player;
  (void)x_scale;
  (void)y_scale;
  (void)w_scale;
  (void)h_scale;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_codec_info(player_h player, char **audio_codec, char **video_codec) {
  (void)player;
  (void)audio_codec;
  (void)video_codec;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_video_stream_info(player_h player, int *fps, int *bit_rate) {
  (void)player;
  (void)fps;
  (void)bit_rate;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_video_size(player_h player, int *width, int *height) {
  (void)player;
  (void)width;
  (void)height;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_duration(player_h player, int *milliseconds) {
  (void)player;
  (void)milliseconds;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_streaming_buffering_time(player_h player, int prebuffer_ms, int rebuffer_ms) {
  (void)player;
  (void)prebuffer_ms;
  (void)rebuffer_ms;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_streaming_download_progress(player_h player, int *start, int *end) {
  (void)player;
  (void)start;
  (void)end;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_completed_cb(player_h player, player_completed_cb callback, void *user_data) {
  (void)player;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_unset_completed_cb(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_interrupted_cb(player_h player, player_interrupted_cb callback, void *user_data) {
  (void)player;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_unset_interrupted_cb(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_error_cb(player_h player, player_error_cb callback, void *user_data) {
  (void)player;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_unset_error_cb(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_buffering_cb(player_h player, player_buffering_cb callback, void *user_data) {
  (void)player;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_unset_buffering_cb(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_playback_rate(player_h player, float rate) {
  (void)player;
  (void)rate;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_subtitle_updated_cb(player_h player, player_subtitle_updated_cb callback, void *user_data) {
  (void)player;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_unset_subtitle_updated_cb(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_video_stream_changed_cb(player_h player, player_video_stream_changed_cb callback, void *user_data) {
  (void)player;
  (void)callback;
  (void)user_data;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_unset_video_stream_changed_cb(player_h player) {
  (void)player;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_current_track(player_h player, player_stream_type_e type, int *index) {
  (void)player;
  (void)type;
  (void)index;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_track_language_code(player_h player, player_stream_type_e type, int index, char **code) {
  (void)player;
  (void)type;
  (void)index;
  (void)code;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_get_track_count(player_h player, player_stream_type_e type, int *count) {
  (void)player;
  (void)type;
  (void)count;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_select_track(player_h player, player_stream_type_e type, int index) {
  (void)player;
  (void)type;
  (void)index;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_max_adaptive_variant_limit(player_h player, int bandwidth, int width, int height) {
  (void)player;
  (void)bandwidth;
  (void)width;
  (void)height;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_streaming_cookie(player_h player, const char *cookie, int size) {
  (void)player;
  (void)cookie;
  (void)size;
  return PLAYER_ERROR_INVALID_PARAMETER;
}

int player_set_streaming_user_agent(player_h player, const char *user_agent, int size) {
  (void)player;
  (void)user_agent;
  (void)size;
  return PLAYER_ERROR_INVALID_PARAMETER;
}
