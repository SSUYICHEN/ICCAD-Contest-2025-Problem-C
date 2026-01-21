#include "headers.h"

const string TCL_FILE_NAME= "pd25.tcl";
const int MAX_N_UNIMPROVE = 5;

int main(int argc, char** argv) {
    time_t start_wall = time(nullptr);
    srand(0);
    
    // setup
    
    if (argc < 8) {
        cout << "Usage: ./opt <main_folder> <alpha> <beta> <gamma> <output_def> <output_pl> <output_eco>" << endl;
        cout << "Example: ./opt ./ICCAD25_PorbC 0.334 0.333 0.333 pd25.def pd25.pl pd25.changelist" << endl;
        exit(-1);
    }
    const string main_folder = argv[1];
    const double alpha = stod(argv[2]);
    const double beta  = stod(argv[3]);
    const double gamma = stod(argv[4]);
    string resulting_file_name = argv[5];
    string resulting_pl_name = argv[6];
    string resulting_eco_name = argv[7];
    
    if (true) {
        ofstream output_file_stream;
        output_file_stream.open(TCL_FILE_NAME);
        output_file_stream <<  "foreach libFile [glob \"" << main_folder << "/ASAP7/LIB/*nldm*.lib\"] {\n";
        output_file_stream <<  "    read_liberty $libFile\n";
        output_file_stream <<  "}\n";
        output_file_stream <<  "read_lef " << main_folder << "/ASAP7/techlef/asap7_tech_1x_201209.lef\n";
        output_file_stream <<  "foreach lef [glob \"" << main_folder << "/ASAP7/LEF/*.lef\"] {\n";
        output_file_stream <<  "    read_lef $lef\n";
        output_file_stream <<  "}\n";
        output_file_stream <<  "read_def " << main_folder << "/aes_cipher_top/aes_cipher_top.def\n";
        output_file_stream <<  "read_sdc " << main_folder << "/aes_cipher_top/aes_cipher_top.sdc\n";
        output_file_stream <<  "source " << main_folder << "/ASAP7/setRC.tcl\n";
        output_file_stream <<  "detailed_placement\n";
        output_file_stream <<  "estimate_parasitics -placement\n";
    }
    
    ofstream eco_file_stream;
    eco_file_stream.open(resulting_eco_name);
    
    bp::opstream in;
    bp::ipstream out;
    vector<vector<string>> openroad_output;
    
    bp::child c("openroad " + TCL_FILE_NAME, bp::std_in < in, bp::std_out > out);
    cout << "initializing openroad... " << std::flush; 
    string word;
    while (out >> word) 
        if (word == "openroad>")
            break;
    cout << "finished." << endl;
    
    cout << "reading library cells... " << std::flush; 
    unordered_map<string, vector<pair<string, double>>> lib_cells = readLibs(main_folder);
    cout << "finished." << endl;
    
    // get initial metrics
    
    openroad_output = parseResult("detailed_placement\n", in, out);
    const double init_hpwl = stod(openroad_output[openroad_output.size() - 2][2]);
    cout << "init_hpwl = " << init_hpwl << endl;
    
    openroad_output = parseResult("report_tns\n", in, out);
    const double init_tns = stod(openroad_output.back()[2]);
    cout << "init_tns = " << init_tns << endl;
    
    openroad_output = parseResult("report_power\n", in, out);
    const double init_pwr = stod(openroad_output[openroad_output.size() - 2][4]);
    cout << "init_pwr = " << init_pwr << endl;

    auto [init_locations, cell2lib] = getInitData(main_folder + "/aes_cipher_top/aes_cipher_top.def");
    unordered_map<string, pair<int, int>> prev_locations = init_locations;
    
    double prev_score = 0;  
    
    // fix cells with neg paths by resizing and buffering 
      
    int new_cell_id = 1000;    
    while (true) {
        openroad_output = parseResult("report_cells_neg_path -percentage 100\n", in, out);

        unordered_map<string, double> load_delays;
        unordered_map<string, string> old_libs;
        for (int i = 1; i < openroad_output.size(); ++i) {
            string pin_name = openroad_output[i][0];
            string cell_name = pin_name.substr(0, pin_name.find('/'));
            
            string old_lib = cell2lib[cell_name];
            string postfix = old_lib.substr(old_lib.length() - 2);
            string cell_type = old_lib.substr(0, old_lib.find('x'));
            
            if (postfix == "SL" && old_lib == lib_cells[cell_type].back().first) {  // buffering
                load_delays[pin_name] += stod(openroad_output[i][1]);
                old_libs[pin_name] = old_lib;
            }
            else {  // resizing or VT flavors upgrade
                load_delays[cell_name] += stod(openroad_output[i][1]);
                old_libs[cell_name] = old_lib;
            }
        }
                           
        vector<pair<string, double>> load_delays_list;
        for (auto& item : load_delays) {
            load_delays_list.push_back(item);
        }
        sort(
            load_delays_list.begin(),
            load_delays_list.end(),
            [](const auto& lhs, const auto& rhs) {
                return lhs.second > rhs.second;
        });     
              
        bool improved = false;
        int n_unimproved = 0;
        for (const auto& item : load_delays_list) {
            string name = item.first;
            string old_lib = old_libs[name];
            
            // try to fix
            string new_lib = "";
            string net_name;
            string new_buf_name;
            string new_net_name;
            vector<string> out_pins_g1, out_pins_g2;
            
            string postfix = old_lib.substr(old_lib.length() - 2);
            if (postfix == "SL") { 
                string cell_type = old_lib.substr(0, old_lib.find('x'));
                if (old_lib != lib_cells[cell_type].back().first) {  // resizing
                    for (int i = 0; i < lib_cells[cell_type].size(); ++i) {
                        if (i == lib_cells[cell_type].size() - 1)
                            assert(false);
                        if (lib_cells[cell_type][i].first == old_lib) {
                            new_lib = lib_cells[cell_type][i + 1].first;
                            cell2lib[name] = new_lib;
                            checkResult("replace_cell " + name + " " + new_lib + "\n", in, out);
                            break;
                        }
                    }
                }
                else {                                               // buffering
                    openroad_output = parseResult("get_name [get_nets -of_objects " + name + "]\n", in, out);
                    net_name = openroad_output.back()[0];
                    
                    openroad_output = parseResult("get_pins -of_objects " + net_name + "\n", in, out);
                    int g1_cnt = 0;
                    for (const string& pin_fake_name : openroad_output.back()) {
                        vector<vector<string>> sub_output = parseResult("get_name " + pin_fake_name + "\n", in, out);
                        string port_name = sub_output.back()[0];
                        
                        sub_output = parseResult("get_name [get_cells -of_objects " + pin_fake_name + "]\n", in, out);
                        string cell_name = sub_output.back()[0];
                        string pin_real_name = cell_name + "/" + port_name;
                        if (pin_real_name == name) {
                            continue;
                        }
                        if (pin_real_name == "while/while") {  // (weird?)
                            continue;
                        }
                        
                        if (rand() % 2 == 0) {
                            out_pins_g1.push_back(pin_real_name);
                        }
                        else {
                            out_pins_g2.push_back(pin_real_name);
                        }
                    }
                                
                    new_buf_name = "pd25_buf_" + to_string(new_cell_id);
                    new_net_name = "pd25_net_" + to_string(new_cell_id);
                    string cell_name = name.substr(0, name.find('/'));
                    
                    skipResult("place_inst -name " + new_buf_name + \
                              " -location {" + to_string(prev_locations[name].first) + " " + to_string(prev_locations[name].second) + \
                              "} -cell " + lib_cells["BUF"].back().first + "\n", in, out);
                    cell2lib[new_buf_name] = lib_cells["BUF"].back().first;
                    skipResult("make_net " + new_net_name + "\n", in, out);
                    new_cell_id++;
                    
                    checkResult("connect_pin " + net_name + " " + new_buf_name + "/A\n", in, out);
                    checkResult("connect_pin " + new_net_name + " " + new_buf_name + "/Y\n", in, out);
                     
                    for (const string& out_pin : out_pins_g1) {
                        skipResult("disconnect_pin " + net_name + " " + out_pin + "\n", in, out);
                        checkResult("connect_pin " + new_net_name + " " + out_pin + "\n", in, out);
                    }          
                }     
            }
            else {                                                   // VT flavors upgrade    
                if (postfix == "SRAM") {
                    new_lib = old_lib.substr(0, old_lib.length() - 4) + "R";
                }
                else if (postfix == "_R") {
                    new_lib = old_lib.substr(0, old_lib.length() - 1) + "L";
                }
                else if (postfix == "_L") {
                    new_lib = old_lib.substr(0, old_lib.length() - 1) + "SL";
                }
                else {
                    assert(false);
                }
                cell2lib[name] = new_lib;
                checkResult("replace_cell " + name + " " + new_lib + "\n", in, out);
            }
            
            // see effects
            
            auto [hpwl, tns, pwr, new_locations] = calMetrics(in, out);
            double tns_imp = (tns - init_tns) / abs(init_tns);
            double pwr_imp = (init_pwr - pwr) / init_pwr;
            double hpwl_imp = (init_hpwl - hpwl) / init_hpwl;
            double displacement = calDisplacement(init_locations, new_locations);
            double score = 1000.0 * (alpha * tns_imp + beta * pwr_imp + gamma * hpwl_imp) - 50.0 * displacement;  

            // save or recover
            if (score > prev_score * 1.01) { // save
                prev_locations = new_locations;
                prev_score = score;
                skipResult("write_def " + resulting_file_name + "\n", in, out);
                cout << std::setprecision(5) << "score = " << score << ", at time = " << time(nullptr) - start_wall << std::endl;
                
                if (new_lib == "") {
                    eco_file_stream << "insert_buffer ";    
                
                    for (const string& out_pin : out_pins_g1) {
                        eco_file_stream << out_pin << " ";
                    }   
                    eco_file_stream << lib_cells["BUF"].back().first << " " << new_buf_name << " " << new_net_name << "\n";
                }
                else {
                    eco_file_stream << "size_cell " << name << " " << new_lib << "\n";
                }

                improved = true;
                n_unimproved = 0;
                break;
            }
            else {                           // recover
                if (new_lib == "") {
                    for (const string& out_pin : out_pins_g1) {
                        skipResult("disconnect_pin " + new_net_name + " " + out_pin + "\n", in, out);
                        checkResult("connect_pin " + net_name + " " + out_pin + "\n", in, out);
                    }  
                    
                    skipResult("disconnect_pin " + net_name + " " + new_buf_name + "/A\n", in, out);
                    skipResult("disconnect_pin " + new_net_name + " " + new_buf_name + "/Y\n", in, out);
    
                    skipResult("delete_instance " + new_buf_name + "\n", in, out);
                    skipResult("delete_net " + new_net_name + "\n", in, out);
                    
                    restorePlacement(prev_locations, new_locations, in, out);
                } 
                else {
                    cell2lib[name] = old_lib;
                    checkResult("replace_cell " + name + " " + old_lib + "\n", in, out);
                }
                
                n_unimproved++;
                if (n_unimproved > MAX_N_UNIMPROVE) 
                    break;
            }
        }
        
        if (!improved)
            break;
    }
                
    in << "exit\n" << std::flush;    
    c.wait();
    
    def2pl(resulting_file_name, resulting_pl_name);
    
    assert(false && "END");  // make sure assertions are turned off for released version
    return 0;
}