#pragma once

#include "../network/Base64.h"

#include <memory>
#include <string>
#include <string_view>

namespace rnwebos::testing {

// Each picture is 64x48, its left half red and its right half blue, in
// base64. `kTurnedJpeg` is stored that way with EXIF orientation 6, so it
// shows 48x64 with red on top.
inline constexpr std::string_view kPng =
    "iVBORw0KGgoAAAANSUhEUgAAAEAAAAAwCAIAAAAuKetIAAAAZ0lEQVR4nNXOAQ0AIBCAQJ7+nTWE+zmuAMxh17BbkDiJkziJkziJ"
    "kziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJkziJ8/fAqwvJlQJfjEkJSgAA"
    "AABJRU5ErkJggg==";

inline constexpr std::string_view kJpeg =
    "/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAA0JCgsKCA0LCgsODg0PEyAVExISEyccHhcgLikxMC4pLSwzOko+MzZGNywtQFdBRkxO"
    "UlNSMj5aYVpQYEpRUk//2wBDAQ4ODhMREyYVFSZPNS01T09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09P"
    "T09PT09PT0//wAARCAAwAEADASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUF"
    "BAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVW"
    "V1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi"
    "4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAEC"
    "AxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVm"
    "Z2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq"
    "8vP09fb3+Pn6/9oADAMBAAIRAxEAPwDn6KKK8Y/SQooooAwqKKK/Yj8qCiiigDdooor8dP1UKKKKAMKiiiv2I/KgooooA3aKKK/H"
    "T9VCiiigDCooor9iPyoKKKKAP//Z";

inline constexpr std::string_view kWebp =
    "UklGRiYAAABXRUJQVlA4TBoAAAAvP8ALAA8Q87//8x8OxILJ/KVT6Izof1TBdw==";

inline constexpr std::string_view kGif =
    "R0lGODdhQAAwAIEAAP8AAAAA/wAAAAAAACwAAAAAQAAwAEAItAABCBxIsKDBgwQDKFzIsKHDhwwRSpxYEKLFiw0papSIsaPFjSAr"
    "ehwZMaRJkigDmDyZcuTKkC1dvtwY0+NMmjUx3tSYU+fOiT0v/gQaFOJQjkUfHkWYVOlSg00dPoUateTUgVWtXgWQdeFWrF1VfuUa"
    "dizZrmbDiv2qNm3ZsW3hvmU7d2tcumjl5sWb1e1eu3Wv3gX8V3DgqYMNF0Z8+GlixosdN176WHJkypOPVsZ8WXPYgAA7";

inline constexpr std::string_view kTurnedJpeg =
    "/9j/4AAQSkZJRgABAQAAAQABAAD/4QAiRXhpZgAATU0AKgAAAAgAAQESAAMAAAABAAYAAAAAAAD/2wBDAA0JCgsKCA0LCgsODg0P"
    "EyAVExISEyccHhcgLikxMC4pLSwzOko+MzZGNywtQFdBRkxOUlNSMj5aYVpQYEpRUk//2wBDAQ4ODhMREyYVFSZPNS01T09PT09P"
    "T09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT0//wAARCAAwAEADASIAAhEBAxEB/8QAHwAAAQUBAQEB"
    "AQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAk"
    "M2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKj"
    "pKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAA"
    "AAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl"
    "8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmq"
    "srO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwDn6KKK8Y/SQooooAwqKKK/"
    "Yj8qCiiigDdooor8dP1UKKKKAMKiiiv2I/KgooooA3aKKK/HT9VCiiigDCooor9iPyoKKKKAP//Z";

inline std::shared_ptr<const std::string> imageBytes(std::string_view base64) {
  return std::make_shared<const std::string>(decodeBase64(base64));
}

} // namespace rnwebos::testing
