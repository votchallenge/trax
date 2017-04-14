#ifndef __TRAX_CLIENT
#define __TRAX_CLIENT

#include <string>
#include <map>
#include <vector>
#include <trax.h>

#ifdef TRAX_CLIENT_STATIC_DEFINE
#  define __TRAX_CLIENT_EXPORT
#else
#  ifndef __TRAX_CLIENT_EXPORT
#    if defined(_MSC_VER)
#      ifdef trax_client_EXPORTS
         /* We are building this library */
#        define __TRAX_CLIENT_EXPORT __declspec(dllexport)
#      else
         /* We are using this library */
#        define __TRAX_CLIENT_EXPORT __declspec(dllimport)
#      endif
#    elif defined(__GNUC__)
#      ifdef trax_client_EXPORTS
         /* We are building this library */
#        define __TRAX_CLIENT_EXPORT __attribute__((visibility("default")))
#      else
         /* We are using this library */
#        define __TRAX_CLIENT_EXPORT __attribute__((visibility("default")))
#      endif
#    endif
#  endif
#endif

using namespace std;

#define TRAX_DEFAULT_PORT 9090

namespace trax {

enum ConnectionMode {CONNECTION_DEFAULT, CONNECTION_EXPLICIT, CONNECTION_SOCKETS};

enum VerbosityMode {VERBOSITY_SILENT, VERBOSITY_DEFAULT, VERBOSITY_DEBUG};

typedef bool (WatchdogCallback)();

class __TRAX_CLIENT_EXPORT TrackerProcess {
public:

	TrackerProcess(const string& command, const map<string, string> environment = map<string, string>(), int timeout = 10,
		ConnectionMode connection = CONNECTION_DEFAULT, VerbosityMode verbosity = VERBOSITY_DEFAULT);
	~TrackerProcess();

	int image_formats();
	int region_formats();

	bool ready();
	bool tracking();

	bool initialize(const Image& image, const Region& region, const Properties& properties = Properties());
	bool wait(Region& region, Properties& properties);
	bool frame(const Image& image, const Properties& properties = Properties());

	bool reset();

	void register_watchdog(WatchdogCallback* callback);

	void set_log(ostream *output);

private:

	class State;
	State* state;

};

int __TRAX_CLIENT_EXPORT load_trajectory(const std::string& file, std::vector<Region>& trajectory);

void __TRAX_CLIENT_EXPORT save_trajectory(const std::string& file, std::vector<Region>& trajectory);

typedef unsigned long long timer_state;

timer_state __TRAX_CLIENT_EXPORT timer_clock();

double __TRAX_CLIENT_EXPORT timer_elapsed(timer_state begin);

}

#endif
