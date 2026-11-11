// Included ahead of SDL's src/hidapi/SDL_hidapi.c on a TV build.
// SDL-webOS 3.4.16-webos.5 resets the bookkeeping of its hidraw hotplug poll
// in DeleteHIDDeviceWrapper even when HIDAPI is off, where neither the state
// nor the function is declared, and the file does not compile.

static struct {
  unsigned m_unPresenceFlags;
  unsigned long long m_unLastDetect;
} SDL_HIDAPI_discovery;

#define SDL_webOSGetDevicePresenceFlags(check) 0u
#define SDL_WEBOS_DEVICE_PRESENCE_CHECK_HIDRAW 0
