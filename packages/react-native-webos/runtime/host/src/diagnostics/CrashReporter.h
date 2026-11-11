#pragma once

namespace rnwebos {

/// Reports a native crash before the process dies: the signal and the stack as
/// `#N library+0xoffset` lines, on stderr and on the log stream, since a retail
/// TV keeps no log and no core dump an app can read. `rn-webos logs` turns the
/// offsets into function names. A fault at an address no loaded object holds
/// passes to the previous handler untouched.
void installCrashReporter();

/// The descriptor crash reports also go to; -1 for none. Safe to call from any thread.
void setCrashReportDescriptor(int descriptor);

} // namespace rnwebos
