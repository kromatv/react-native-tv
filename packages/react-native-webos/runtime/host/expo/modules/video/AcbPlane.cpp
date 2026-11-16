#include "AcbPlane.h"

#include "PipelineQuestions.h"
#include "PlayerLog.h"

#include "logging/Log.h"

#include <chrono>
#include <format>
#include <utility>

namespace rnwebos::expo {

namespace {

// The values of webosbrew's AcbAPI.h. The URI path is the plain video player on
// the main sink: MSE and the sub sink are for apps that feed the pipeline
// themselves, and put the picture on another plane (section 2.1).
constexpr int kPlayerTypeVideo = 0;
constexpr int kSinkMain = 0;
constexpr int kForeground = 1;
constexpr int kUnloaded = 0;
constexpr int kLoaded = 1;
constexpr int kPlaying = 2;
constexpr int kPaused = 3;

// Any string binds a webOS 4 load; an empty one is answered and never fetched.
constexpr auto kWindowId = "window_id_dummy";

// Unpaced, the video data overtakes the media's registration and the plane stays black.
constexpr auto kPace = std::chrono::milliseconds(200);
constexpr auto kBeforeWindow = std::chrono::milliseconds(300);

// The plane is the panel's, in the graphics plane's 1920x1080, and full screen ignores the rectangle.
constexpr long kScreenWidth = 1920;
constexpr long kScreenHeight = 1080;

const char *taskOutcome(long event) {
  switch (event) {
    case 1:
      return "done";
    case 0:
      return "cancelled";
    case -1:
      return "failed";
    case -2:
      return "timed out";
    default:
      return "answered";
  }
}

// Called on the library's own thread, which logging allows.
void onTask(long, long task, long event, long, long playState, const char *reply) {
  log(event == 1 ? LogLevel::Debug : LogLevel::Warning,
      std::format(
          "[video] control block task {} {}, play state {}: {}",
          task,
          taskOutcome(event),
          playState,
          reply != nullptr ? reply : ""));
}

// The description the plane needs before it lights, as the webOS 4 apps that
// show a picture send it, SDR. `context` must be the media: the service passes
// the payload on as it is, and the display refuses one without it.
std::string videoData(const std::string &mediaId, const VideoStream &video, int player) {
  const bool sized = video.width > 0 && video.height > 0;
  if (!sized || video.fps <= 0 || video.bitrate <= 0) {
    pipeline::openQuestion(player, "control block told 1920x1080, 24 fps, 8 Mbps where the pipeline did not say");
  }
  return std::format(
      R"({{"content":"movie","context":"{}","video":{{"adaptive":false,"bitRate":{},)"
      R"("data3D":{{"currentPattern":"2d","originalPattern":"2d","typeLR":"LR"}},)"
      R"("frameRate":{}.0,"hdrType":"none","height":{},"width":{},)"
      R"("mediaSei":{{"displayPrimariesX0":0,"displayPrimariesX1":0,"displayPrimariesX2":0,)"
      R"("displayPrimariesY0":0,"displayPrimariesY1":0,"displayPrimariesY2":0,"maxContentLightLevel":0,)"
      R"("maxDisplayMasteringLuminance":0,"maxPicAverageLightLevel":0,"minDisplayMasteringLuminance":0,)"
      R"("whitePointX":0,"whitePointY":0}},)"
      R"("mediaVui":{{"colorPrimaries":2,"matrixCoeffs":2,"transferCharacteristics":2,"videoFullRangeFlag":false}},)"
      R"("hfr":false,"path":"network","pixelAspectRatio":{{"height":1,"width":1}},"rotation":"0",)"
      R"("scanType":"VIDEO_PROGRESSIVE","specificRendering":"none"}}}})",
      mediaId,
      video.bitrate > 0 ? video.bitrate : 8000000,
      video.fps > 0 ? video.fps : 24,
      sized ? video.height : 1080,
      sized ? video.width : 1920);
}

} // namespace

AcbSession::AcbSession(const AcbApi &api, const std::string &appId) : api_(api) {
  using enum LogLevel;
  const long handle = api_.create();
  if (handle == 0) {
    log(Warning, "[video] the TV made no control block: sound without a picture");
    return;
  }
  // Another app's id would bind the plane to that app.
  if (!api_.initialize(handle, kPlayerTypeVideo, appId.c_str(), onTask)) {
    log(Warning, "[video] the control block refused to start: sound without a picture");
    return;
  }
  api_.setSinkType(handle, kSinkMain);
  handle_ = handle;
  log(Info, std::format("[video] control block {} for {}", handle_, appId));
}

AcbPlane::AcbPlane(AcbSession &session, int player) : session_(session), player_(player) {}

std::string AcbPlane::windowId() {
  return kWindowId;
}

bool AcbPlane::namesTransport() const {
  return false;
}

void AcbPlane::show(Scene &, const PixelRect &, const PixelRect &destination) {
  playerLog(
      player_,
      std::format(
          "plane full screen, asked for {},{} {}x{}",
          destination.x,
          destination.y,
          destination.width,
          destination.height),
      LogLevel::Debug);
}

void AcbPlane::hide() {
  // The media is unloaded with its view, and the unload unbinds the plane.
}

void AcbPlane::bind(const std::string &mediaId, const VideoStream &video) {
  pace_.cancel();
  bound_ = false;
  mediaId_ = mediaId;
  video_ = video;
  if (session_.handle() == 0) return;
  advance(0);
}

// A bind or a release cancels the pace, so a step never runs for a media that is gone.
void AcbPlane::advance(int step) {
  const auto &api = session_.api();
  const long acb = session_.handle();
  long task = 0;
  const auto next = [this, step](std::chrono::milliseconds wait) {
    pace_.start(wait, [this, step] { advance(step + 1); });
  };
  switch (step) {
    case 0:
      playerLog(player_, std::format("control block media {}: {}", mediaId_, api.setMediaId(acb, mediaId_.c_str())));
      return next(kPace);
    case 1:
      playerLog(player_, std::format("control block loaded: {}", api.setState(acb, kForeground, kLoaded, &task)));
      return next(kPace);
    case 2:
      playerLog(player_, std::format("control block video data: {}", sendVideoData()));
      return next(kBeforeWindow);
    default:
      bound_ = true;
      showFullScreen();
      setState(playing_ ? kPlaying : kPaused);
      return;
  }
}

void AcbPlane::playingChanged(bool playing) {
  playing_ = playing;
  if (!bound_) return;
  setState(playing ? kPlaying : kPaused);
  // The apps that ship place the window again each time the media plays.
  if (playing) showFullScreen();
}

void AcbPlane::release() {
  pace_.cancel();
  bound_ = false;
  playing_ = false;
  if (std::exchange(mediaId_, {}).empty() || session_.handle() == 0) return;
  setState(kUnloaded);
}

int AcbPlane::sendVideoData() const {
  long task = 0;
  const auto data = videoData(mediaId_, video_, player_);
  return session_.api().setMediaVideoData(session_.handle(), data.c_str(), &task);
}

void AcbPlane::setState(int playState) const {
  long task = 0;
  const int sent = session_.api().setState(session_.handle(), kForeground, playState, &task);
  playerLog(player_, std::format("control block play state {}: {}", playState, sent), LogLevel::Debug);
}

void AcbPlane::showFullScreen() const {
  long task = 0;
  const int sent =
      session_.api().setDisplayWindow(session_.handle(), 0, 0, kScreenWidth, kScreenHeight, true, &task);
  playerLog(player_, std::format("control block window full screen: {}", sent), LogLevel::Debug);
}

} // namespace rnwebos::expo
