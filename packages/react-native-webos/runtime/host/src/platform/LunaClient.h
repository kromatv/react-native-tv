#pragma once

#include "LunaConnection.h"

#include <memory>
#include <string>

namespace rnwebos {

/// A connection to the Luna bus under a name the caller chooses, which
/// `libhelpers.so.2` cannot give: a service such as `com.webos.media` answers
/// only clients whose name the app's role allows
/// (docs/reference/webos-media-playback.md, section 1.2). It registers through
/// `libluna-service2.so.3`, opened at run time, and takes its replies on a
/// thread of its own. Any thread.
class LunaClient final : public LunaConnection {
 public:
  /// Registers as `name`. Null when the hub refuses the name, and `refusal`
  /// then says why; null with `refusal` left empty where luna-service2 is
  /// missing, as in the simulator.
  static std::unique_ptr<LunaClient> registerAs(const std::string &name, std::string *refusal = nullptr);

  ~LunaClient() override;

  LunaClient(const LunaClient &) = delete;
  LunaClient &operator=(const LunaClient &) = delete;

  bool isAvailable() const override;

  bool call(const std::string &uri, const folly::dynamic &payload, Reply onReply = {}) override;

  std::unique_ptr<Subscription> subscribe(
      const std::string &uri, const folly::dynamic &payload, Reply onReply) override;

  struct Registration;

 private:
  explicit LunaClient(std::shared_ptr<Registration> registration);

  const std::shared_ptr<Registration> registration_;
};

} // namespace rnwebos
