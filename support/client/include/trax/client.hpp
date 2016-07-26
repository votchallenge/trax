#ifndef __TRAX_CLIENT
#define __TRAX_CLIENT

#include <string>
#include <map>
#include <vector>
#include <trax.h>

using namespace std;

#define TRAX_DEFAULT_PORT 9090

namespace trax {

enum ConnectionMode {CONNECTION_DEFAULT, CONNECTION_EXPLICIT, CONNECTION_SOCKETS};
	
enum VerbosityMode {VERBOSITY_SILENT, VERBOSITY_DEFAULT, VERBOSITY_DEBUG};

class TrackerProcess {
public:

	TrackerProcess(const string& command, map<string, string> environment, int timeout = 10, 
		ConnectionMode connection = CONNECTION_DEFAULT, VerbosityMode verbosity = VERBOSITY_DEFAULT);
	~TrackerProcess();

	int image_formats();
	int region_formats();

	bool ready();
	bool tracking();

	bool initialize(Image& image, Region& region, Properties& properties);
	bool wait(Region& region, Properties& properties);
	bool frame(Image& image, Properties& properties);

	bool reset();

private:

	class State;
	State* state;

};

void load_trajectory(const std::string& file, std::vector<Region>& trajectory);

void save_trajectory(const std::string& file, std::vector<Region>& trajectory);

}

#endif