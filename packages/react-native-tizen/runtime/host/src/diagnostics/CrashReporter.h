#pragma once

namespace rntizen {

/// Reports a native crash before the process dies: the signal and the stack as
/// `#N library+0xoffset` lines, on stderr and on the log stream, since a retail
/// TV keeps neither dlog nor core dumps. `rn-tizen logs` turns the offsets into
/// function names. Faults outside native libraries, such as the null
/// dereferences .NET turns into exceptions, pass to the previous handler untouched.
void installCrashReporter();

/// The descriptor crash reports also go to; -1 for none. Safe to call from any thread.
void setCrashReportDescriptor(int descriptor);

} // namespace rntizen
