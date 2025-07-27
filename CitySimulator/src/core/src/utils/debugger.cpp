#include <core/stdafx.h>

#include <core/utils/debugger.h>

#include <csignal>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <fstream>
#include <string>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#endif

namespace tjs::debugger {

	bool is_debugger_present() {
#if defined(_WIN32)
		return IsDebuggerPresent();
#elif defined(__APPLE__)
		int mib[4];
		kinfo_proc info;
		size_t size = sizeof(info);

		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PID;
		mib[3] = getpid();

		if (sysctl(mib, 4, &info, &size, NULL, 0) != 0) {
			return false;
		}

		return (info.kp_proc.p_flag & P_TRACED) != 0;
#elif defined(__linux__)
		std::ifstream status("/proc/self/status");
		std::string line;
		while (std::getline(status, line)) {
			if (line.rfind("TracerPid:", 0) == 0) {
				int tracer_pid = std::stoi(line.substr(10));
				return tracer_pid != 0;
			}
		}
		return false;
#else
		return false; // Not supported on this platform
#endif
	}

#if TJS_SIMULATION_DEBUG
	void debug_break() {
		static int __skip = 0;
		if (!is_debugger_present() || __skip != 0) {
			return;
		}
#if defined(_MSC_VER)
		__debugbreak(); // Visual Studio / Windows
#elif defined(__GNUC__) || defined(__clang__)
		kill(getpid(), SIGINT);
#endif
	}
#else
	void debug_break() {}
#endif
} // namespace tjs::debugger
