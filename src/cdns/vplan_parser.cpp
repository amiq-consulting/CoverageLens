/******************************************************************************
 * (C) Copyright 2017 AMIQ Consulting
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************/

#include "vplan_parser.hpp"

// Debug
static bool debug_switch;
static ofstream debug_log;

static ifstream vplan_xml;

/**
 * @brief Parses the type of a code coverage entry
 * @param open_sections Current recursivity level
 * @param acc Current accumulated path
 * @param folders Where we'll add a valid folder
 */
void parse_metrics_types(int open_sections, const string & acc,
    unordered_map<string, char> &folders) {
  string line;
  size_t aux, aux2;

  string kind;

  while (1) {

    getline(vplan_xml, line);

    aux = line.find("<");
    aux2 = line.find(" ", aux);

    if (aux == string::npos || aux2 == string::npos)
      break;

    /*
     * How this line works:
     * 1) we stored the path in acc
     * 2) line should be only:
     * a) fsm-...
     * b) block...
     * c) expression...
     * 3) type will be s,l or x
     * 4) we uppercase it with ^32
     * 5) we mapped the section to its type
     */
    folders[acc] = line[aux + 2] ^ 32;

    if (line.find("</metricsTypes>") != string::npos) {
      break;
    }
  }
}

/**
 * @brief Searches for a <metricsTypes> tag while building a path to it
 * @param open_sections: current recursivity level
 * @param acc: current accumulated path
 * @param folders: where we'll add a valid folder
 */
void parse_metrics_port(int open_sections, const string & acc,
    unordered_map<string, char> &folders) {

  string line;
  size_t aux, aux2;

  string name;
  string kind;

  while (1) {
    getline(vplan_xml, line);

    // Get section name
    if (line.find("<name") != string::npos && name.empty()) {
      aux = line.find('>');
      aux2 = line.find("</");

      name = string(line, aux + 1, aux2 - aux - 1);
    }

    // Get its type
    if (line.find("<metricsTypes>") != string::npos)
      parse_metrics_types(open_sections + 1, (acc + "/" + name), folders);

    // Get name of the folder
    if (line.find("metrics_port_kind") != string::npos && kind.empty()) {
      getline(vplan_xml, line);

      aux = line.find('>');
      aux2 = line.find("</");

      kind = string(line, aux + 1, aux2 - aux - 1);

      for (int i = 0; i < open_sections; ++i)
        debug_log << "\t";

      debug_log << kind << ": " << name << "\n";
    }

    if (line.find("</metricsPort>") != string::npos) {
      break;
    }
  }
}

/**
 * @brief Parses a <section> tag while building a path
 * @param open_sections Current recursivity level
 * @param acc Current accumulated path
 * @param folders Where we'll add a valid folder
 */
void parse_section(int &open_sections, const string &acc, unordered_map<string, char> &folders) {
  string line;

  size_t aux, aux2;
  string name;

  while (1) {
    getline(vplan_xml, line);

    if (line.find("<name") != string::npos && name.empty()) {
      aux = line.find('>');
      aux2 = line.find("</");

      name = string(line, aux + 1, aux2 - aux - 1);

      for (int i = 0; i < open_sections; ++i)
        debug_log << "\t";
      debug_log << "Folder: " << name << "\n";
    }

    // Start searching for a code coverage folder
    if (line.find("<metricsPort ") != string::npos)
      parse_metrics_port(open_sections + 1, acc + "/" + name, folders);

    if (line.find("<section ") != string::npos)
      parse_section(++open_sections, acc + "/" + name, folders);

    if (line.find("</section>") != string::npos) {
      open_sections--;
      break;
    }
  }
}

/**
 * Used in logging
 */
static void pretty_print_type(unordered_map<string, char> &folders) {
  debug_log << "Code coverage mapped @:\n";
  debug_log << "=============================================\n\n";
  for (auto& x : folders)
    debug_log << x.first << " " << x.second << "\n";
  debug_log << "\n=============================================\n";
}

/**
 * @brief Handles I/O and populates the folder map
 * @param vplan Path to vplan file
 * @param folders Where we'll add a valid folder
 * @param debug Generate debug file
 * @param silent Don't generate console output
 */
int plan_parser_main(string vplan, unordered_map<string, char> &folders, bool debug, bool silent) {

  debug_switch = debug;

  if (vplan.empty()) {
    cerr << "No vplan\n";
    return -1;
  }

  ifstream vplan_vplanx(vplan);
  if (!vplan_vplanx.good()) {
    cerr << "Given vplan doesn't exist!\n";
    return -1;
  }

  if (debug_switch)
    debug_log.open("vplan_debug.log");

  // Dearchivate vplan
  string to_xml("/bin/zcat " + vplan + " > amiq_vplan_parser.xml");

  system(to_xml.c_str());

  vplan_xml.open("amiq_vplan_parser.xml", ifstream::in);

  if (!vplan_xml.good()) {
    cerr << "Could not create xml vplan!\n";
    return -1;
  }

  // Skip meta data
  string line;
  getline(vplan_xml, line);

  while (line.find("rootElements") == string::npos)
    getline(vplan_xml, line);

  // Parse vplan
  while (line.find("/rootElements") == string::npos) {

    while (line.find("<section ") == string::npos)
      getline(vplan_xml, line);

    int open = 1;
    parse_section(open, "", folders);

    getline(vplan_xml, line);
  }

  pretty_print_type(folders);

  // Delete dearchivated file
  system("/bin/rm amiq_vplan_parser.xml");

  if (!silent)
  cout << "Vplan parser finished successfully!\n";

  return 0;
}
