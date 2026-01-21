#include <iostream>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <filesystem>           // for list_dir 
#include <fstream>              // for IO
#include <sstream>              // for IO
#include <boost/process.hpp>    // for interaction with OpenRoad

using namespace std;

namespace bp = boost::process;
namespace fs = std::filesystem;

void skipResult(const string& command, bp::opstream& in, bp::ipstream& out);
void checkResult(const string& command, bp::opstream& in, bp::ipstream& out);
//string getResult(const string& command, bp::opstream& in, bp::ipstream& out);
vector<vector<string>> parseResult(const string& command, bp::opstream& in, bp::ipstream& out, bool print = false);

unordered_map<string, vector<pair<string, double>>> readLibs(const string& main_folder);

pair<unordered_map<string, pair<int, int>>, unordered_map<string, string>> getInitData(const string& def_file_name);

void updatePlacementHelper(unordered_map<string, pair<int, int>>& locations, const string& def_file_name);
void updatePlacement(unordered_map<string, pair<int, int>>& locations, bp::opstream& in, bp::ipstream& out);
void restorePlacement(const unordered_map<string, pair<int, int>>& prev_locations, const unordered_map<string, pair<int, int>>& new_locations, bp::opstream& in, bp::ipstream& out);
double calDisplacement(const unordered_map<string, pair<int, int>>& init_locations, const unordered_map<string, pair<int, int>>& locations);

tuple<double, double, double, unordered_map<string, pair<int, int>>> calMetrics(bp::opstream& in, bp::ipstream& out);

void def2pl(const string& def_file, const string& pl_file);