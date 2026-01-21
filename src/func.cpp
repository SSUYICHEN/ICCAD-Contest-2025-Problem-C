#include "headers.h"

const string TMP_DEF_FILE_NAME= "pd25_tmp.def";

/**Function*************************************************************
    
    Parse the output from openroad interface
    
***********************************************************************/
void skipResult(const string& command, bp::opstream& in, bp::ipstream& out) {
    in << command << std::flush;
    string word;
    while (out >> word) {
        if (word == "openroad>") {
            break;
        }
    }
}

void checkResult(const string& command, bp::opstream& in, bp::ipstream& out) {
    in << command << std::flush;
    string prev_word, word;
    while (out >> word) {
        if (word == "openroad>") {
            if (prev_word != "1")
                std::cerr << command << endl;
            assert(prev_word == "1");
            break;
        }
        prev_word = word;
    }
}

string getResult(const string& command, bp::opstream& in, bp::ipstream& out) {
    in << command << std::flush;
    string prev_word, word;
    while (out >> word) {
        if (word == "openroad>") {
            break;
        }
        prev_word = word;
    }
    return prev_word;
}

vector<vector<string>> parseResult(const string& command, bp::opstream& in, bp::ipstream& out, bool print) {
    in << command << std::flush;
    vector<vector<string>> result(1);
    string word;
    while (std::getline(out, word, ' ')) {
        if (word.empty())
            continue;
          
        stringstream sub(word);
        string tmp;
        
        bool is_first = (word[0] != '\n');
        bool break_flag = false;
        
        while (sub >> tmp) {
            if (tmp == "openroad>") {
                break_flag = true;
                break;
            }
            
            if (!is_first) {
                result.push_back(vector<string>());
            }
            if (!tmp.empty())
              result.back().push_back(tmp);
            is_first = false;
        }
        
        if (word[word.length() - 1] == '\n') {
            result.push_back(vector<string>());
        }
        
        if (break_flag)
            break;
    }
    
    // debugging
    if (print) {
        for (vector<string>& line : result) {
            for (string& word : line) {
                cout << word << " " << flush;
            }
            cout << endl;
        }
    }
    return result;
}

/**Function*************************************************************
    
    Read .lib files
    
***********************************************************************/

unordered_map<string, vector<pair<string, double>>> readLibs(const string& main_folder) { 
    unordered_map<string, vector<pair<string, double>>> lib_cells;
    
    for (const auto& entry : fs::directory_iterator(main_folder + "/ASAP7/LIB")) {
        ifstream input_file_stream(entry.path(), ios::in);
        string line;
        
        string cell_type;
        string cell_name;
        double cell_area;
        
        while (getline(input_file_stream, line)) {
            if (line.empty())
                continue;
            
            stringstream line_ss(line);
            string word;
            line_ss >> word;
            if (word == "cell") {        // e.g., cell (AOI22xp33_ASAP7_75t_L) {
                line_ss >> word;
                bool flag = false;
                for (char& c : word) {
                    if (c == '(')
                        continue;
                    if (c == ')')
                        break;
                        
                    cell_name += c;
                    if (c == 'x') {
                        flag = true;
                    }
                    if (!flag) {
                        cell_type += c;
                    }
                }
    
            }
            else if (word == "area") {   // e.g., area : 0.08748;
                if (cell_name[cell_name.length() - 2] == 'S' && cell_name[cell_name.length() - 1] == 'L') {    // use _SRAM as representative
                    line_ss >> word >> cell_area;
                    lib_cells[cell_type].push_back(make_pair(cell_name, cell_area));
                }    
                cell_type = "";
                cell_name = "";
            }
        }
    }
    
    for (auto& item : lib_cells) {
        sort(item.second.begin(), item.second.end(), [](const auto& lhs, const auto& rhs) { return get<1>(lhs) < get<1>(rhs); });
        
        // for debugging
        if (false) {
            cout << item.first << ":" << endl;
            for (auto& subitem : item.second) {
                cout << "  " << get<0>(subitem) << " " << get<1>(subitem) << endl;
            }
        }
    }
    
    return lib_cells;
}

/**Function*************************************************************
    
    Get initial placement and library
    
***********************************************************************/

pair<unordered_map<string, pair<int, int>>, unordered_map<string, string>> getInitData(const string& def_file_name) {
    unordered_map<string, pair<int, int>> locations;
    unordered_map<string, string> cell2lib;
    
    ifstream input_file_stream(def_file_name, ios::in);
    string line;
    bool started = false;
    while (getline(input_file_stream, line)) {
        if (line.empty())
            continue;
        
        stringstream line_ss(line);
        string word;
        line_ss >> word;
        if (word == "COMPONENTS") {
            started = true;
            continue;
        }
        
        if (word == "END") {
            line_ss >> word;
            if (word == "COMPONENTS") 
                break;
        } 
        
        if (!started || word != "-")
            continue;       
        
        int flag = 0;    // 0: init;  1: name_read;  2: lib_read;  3: (_found;  4: x_read;  5: y_read
        string name;
        string lib;
        int x, y;
        while (true) {
            line_ss >> word;
            if (flag == 0) {
                name = word;
                flag = 1;
                continue;
            }
            if (flag == 1) {
                lib = word;
                flag = 2;
                continue;
            }
            if (word == "(") {
                assert(flag == 2);
                flag = 3;
                continue;
            }
            else if (flag == 2)
                continue;
            
            if (flag == 3) {
                x = stoi(word);
                flag = 4;
            }
            else if (flag == 4) {
                y = stoi(word);
                flag = 5;
            }
            else {
                assert(word == ")");
                break;
            }
        }
        locations[name] = make_pair(x, y);
        cell2lib[name] = lib;
    }
    return make_pair(locations, cell2lib);
}

/**Function*************************************************************
    
    Update placement by writing and reading the .def file
    
***********************************************************************/

void updatePlacementHelper(unordered_map<string, pair<int, int>>& locations, const string& def_file_name) {
    ifstream input_file_stream(def_file_name, ios::in);
    string line;
    bool started = false;
    while (getline(input_file_stream, line)) {
        if (line.empty())
            continue;
        
        stringstream line_ss(line);
        string word;
        line_ss >> word;
        if (word == "COMPONENTS") {
            started = true;
            continue;
        }
        
        if (word == "END") {
            line_ss >> word;
            if (word == "COMPONENTS") 
                break;
        } 
        
        if (!started || word != "-")
            continue;       
        
        int flag = 0;    // 0: init;  1: name_read;  2: (_found;  3: x_read;  4: y_read
        string name;
        int x, y;
        while (true) {
            line_ss >> word;
            if (flag == 0) {
                name = word;
                flag = 1;
                continue;
            }
            if (word == "(") {
                assert(flag == 1);
                flag = 2;
                continue;
            }
            else if (flag == 1)
                continue;
            
            if (flag == 2) {
                x = stoi(word);
                flag = 3;
            }
            else if (flag == 3) {
                y = stoi(word);
                flag = 4;
            }
            else {
                assert(word == ")");
                break;
            }
        }
        locations[name] = make_pair(x, y);
    }
}

void updatePlacement(unordered_map<string, pair<int, int>>& locations, bp::opstream& in, bp::ipstream& out) {
    skipResult("write_def " + TMP_DEF_FILE_NAME + "\n", in, out);
    updatePlacementHelper(locations, TMP_DEF_FILE_NAME);
}

/**Function*************************************************************
    
    Restore placement from recordings
    
***********************************************************************/

void restorePlacement(const unordered_map<string, pair<int, int>>& prev_locations, const unordered_map<string, pair<int, int>>& new_locations, bp::opstream& in, bp::ipstream& out) {
    for (auto& item : prev_locations) {
        if (item.second == new_locations.at(item.first))
            continue;
        string name_parsed = item.first;
        boost::replace_all( name_parsed, "\\[", "\\\\\\\\\\[");
        boost::replace_all( name_parsed, "\\]", "\\\\\\\\\\]");
        
        std::stringstream stream;
        stream << std::fixed << std::setprecision(6);;
        stream << "place_inst -name " << name_parsed << " -location {" << (float)get<0>(item.second) / 1000 << " " << (float)get<1>(item.second) / 1000 << "}\n" << std::flush;
         
        skipResult(stream.str(), in, out);
    }
    in.unsetf( ios::fixed );

    // update status
    skipResult("detailed_placement\n", in, out);
    skipResult("estimate_parasitics -placement\n", in, out);    
}

/**Function*************************************************************
    
    Calculate displacement
    
***********************************************************************/

double calDisplacement(const unordered_map<string, pair<int, int>>& init_locations, const unordered_map<string, pair<int, int>>& locations) {
    long long displacement = 0;
    
    for (auto& item : init_locations) {
        displacement += std::abs(get<0>(item.second) - get<0>(locations.at(item.first)));
        displacement += std::abs(get<1>(item.second) - get<1>(locations.at(item.first)));
    }
    
    return double(displacement) / 1000 / locations.size();
}

/**Function*************************************************************
    
    Get metrics
    
***********************************************************************/

tuple<double, double, double, unordered_map<string, pair<int, int>>> calMetrics(bp::opstream& in, bp::ipstream& out) {
    vector<vector<string>> openroad_output = parseResult("detailed_placement\n", in, out);
    double hpwl = stod(openroad_output[openroad_output.size() - 2][2]);
    skipResult("estimate_parasitics -placement\n", in, out);
    
    openroad_output = parseResult("report_tns\n", in, out);
    double tns = stod(openroad_output.back()[2]);
    
    openroad_output = parseResult("report_power\n", in, out);
    double pwr = stod(openroad_output[openroad_output.size() - 2][4]);            
    
    unordered_map<string, pair<int, int>> new_locations;
    updatePlacement(new_locations, in, out);
        
    tuple<double, double, double, unordered_map<string, pair<int, int>>> metrics(hpwl, tns, pwr, new_locations);
    return metrics;
}

/**Function*************************************************************
    
    Export placement
    
***********************************************************************/

void def2pl(const string& def_file, const string& pl_file) {
    ofstream output_file_stream;
    output_file_stream.open(pl_file);
    output_file_stream <<  "UCLA pl 1.0\n\n";
    
    if (true) { // fixed
        ifstream input_file_stream(def_file, ios::in);
        string line;
        bool started = false;
        string name;
        while (getline(input_file_stream, line)) {
            if (line.empty())
                continue;
            
            stringstream line_ss(line);
            string word;
            line_ss >> word;
            if (word == "PINS") {
                started = true;
                continue;
            }
            
            if (word == "END") {
                line_ss >> word;
                if (word == "PINS") 
                    break;
            } 
            
            if (!started)
                continue;       
            
            int x, y;
            string direction;
            if (word == "-") {
                line_ss >> name;
            }
            else if (word == "+") {
                line_ss >> word;
                if (word == "PLACED") {
                    line_ss >> word >> x >> y >> word >> direction;
                    output_file_stream << name << " " << x << " " << y << " : " << direction << " /FIXED_NI\n";
                }
            }
        }
    }  
        
    if (true) { // movable
        ifstream input_file_stream(def_file, ios::in);
        string line;
        bool started = false;
        while (getline(input_file_stream, line)) {
            if (line.empty())
                continue;
            
            stringstream line_ss(line);
            string word;
            line_ss >> word;
            if (word == "COMPONENTS") {
                started = true;
                continue;
            }
            
            if (word == "END") {
                line_ss >> word;
                if (word == "COMPONENTS") 
                    break;
            } 
            
            if (!started || word != "-")
                continue;       
            
            int flag = 0;    // 0: init;  1: name_read;  2: (_found;  3: x_read;  4: y_read
            string name;
            int x, y;
            string direction;
            while (true) {
                line_ss >> word;
                if (flag == 0) {
                    name = word;
                    flag = 1;
                    continue;
                }
                if (word == "(") {
                    assert(flag == 1);
                    flag = 2;
                    continue;
                }
                else if (flag == 1)
                    continue;
                
                if (flag == 2) {
                    x = stoi(word);
                    flag = 3;
                }
                else if (flag == 3) {
                    y = stoi(word);
                    flag = 4;
                }
                else {
                    assert(word == ")");
                    line_ss >> direction;
                    break;
                }
            }
            output_file_stream << name << " " << x << " " << y << " : " << direction << "\n";
        }
    }
}