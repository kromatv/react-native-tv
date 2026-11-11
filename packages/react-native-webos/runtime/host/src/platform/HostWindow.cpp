#include "HostWindow.h"

#include "../logging/Log.h"

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h"

#include <GLES2/gl2.h>
#include <SDL3/SDL.h>

#include <array>
#include <format>
#include <stdexcept>

namespace rnwebos {

namespace {

constexpr unsigned int kRgba8 = 0x8058;

struct ContextRequest {
  int major;
  int stencil;
};

// OpenGL ES 3 where the TV has it, 2 everywhere; some drivers have no stencil
// for the window (docs/reference/webos-window-input.md, section 11).
constexpr std::array<ContextRequest, 4> kContextRequests{{{3, 8}, {2, 8}, {3, 0}, {2, 0}}};

std::runtime_error sdlError(const char *what) {
  return std::runtime_error(std::format("{}: {}", what, SDL_GetError()));
}

const char *glString(unsigned int name) {
  const auto *text = glGetString(name);
  return text != nullptr ? reinterpret_cast<const char *>(text) : "unknown";
}

} // namespace

HostWindow::HostWindow(const std::string &title, PixelSize size) {
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) throw sdlError("Cannot start SDL video");
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  for (const auto &request : kContextRequests) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, request.major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, request.stencil);
    window_ = SDL_CreateWindow(title.c_str(), size.width, size.height, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (window_ == nullptr) continue;
    context_ = SDL_GL_CreateContext(window_);
    if (context_ != nullptr) break;
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  if (window_ == nullptr || context_ == nullptr) throw sdlError("Cannot open an OpenGL ES window");
  SDL_GL_SetSwapInterval(1);
  SDL_GetWindowSizeInPixels(window_, &size_.width, &size_.height);
  createSurface();
  log(LogLevel::Info,
      std::format(
          "Rendering with Skia on {} ({}), SDL {} driver, {}x{} at {:.2f} Hz",
          glString(GL_VERSION),
          glString(GL_RENDERER),
          SDL_GetCurrentVideoDriver(),
          size_.width,
          size_.height,
          refreshRate()));
}

HostWindow::~HostWindow() {
  surface_.reset();
  if (gpu_) gpu_->abandonContext();
  gpu_.reset();
  if (context_ != nullptr) SDL_GL_DestroyContext(context_);
  if (window_ != nullptr) SDL_DestroyWindow(window_);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void HostWindow::createSurface() {
  gpu_ = GrDirectContexts::MakeGL(GrGLInterfaces::MakeEGL());
  if (!gpu_) throw std::runtime_error("Skia cannot use the window's OpenGL ES context");
  int stencil = 0;
  SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil);
  const GrGLFramebufferInfo framebuffer{.fFBOID = 0, .fFormat = kRgba8};
  const auto target = GrBackendRenderTargets::MakeGL(size_.width, size_.height, 0, stencil, framebuffer);
  surface_ = SkSurfaces::WrapBackendRenderTarget(
      gpu_.get(), target, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
  if (!surface_) throw std::runtime_error("Skia cannot draw into the window's framebuffer");
}

PixelSize HostWindow::size() const {
  return size_;
}

SkCanvas &HostWindow::canvas() {
  return *surface_->getCanvas();
}

GrDirectContext &HostWindow::gpu() {
  return *gpu_;
}

SDL_Window *HostWindow::sdlWindow() const {
  return window_;
}

void HostWindow::flush() {
  gpu_->flushAndSubmit(surface_.get());
}

void HostWindow::present() {
  flush();
  SDL_GL_SwapWindow(window_);
}

bool HostWindow::readPixels(SkBitmap &pixels) {
  if (!pixels.tryAllocPixels(SkImageInfo::Make(size_.width, size_.height, kRGBA_8888_SkColorType, kPremul_SkAlphaType))) {
    return false;
  }
  return surface_->readPixels(pixels, 0, 0);
}

float HostWindow::refreshRate() const {
  const auto *mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window_));
  return mode != nullptr ? mode->refresh_rate : 0;
}

} // namespace rnwebos
