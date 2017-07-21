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

#ifndef INCLUDES_FORMATTER_HPP_
#define INCLUDES_FORMATTER_HPP_

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "node_info.hpp"

using std::ifstream;
using std::ofstream;
using std::string;
using std::to_string;
using std::vector;
using std::endl;

class reporter {

protected:
  ofstream report;

public:

  uint err_count;
  char kind;

  explicit reporter(string file, bool mode=true) {

    if (mode)
      report.open(file, std::ofstream::out | std::ofstream::app);
    else
      report.open(file, std::ofstream::out);

    kind = 'd';
    err_count = 0;

  };

  virtual void start() {};
  virtual void title() {};
  virtual void tree_title(string) {};

  virtual void tree_start() {};
  virtual void tree_end() {};

  virtual void end() {};

  virtual void format(const node_info_t&, const string &class_name) {};

  virtual void format_default(const node_info_t&) {};
  virtual void format_fail(const node_info_t&) {};

  virtual ~reporter() {
    report.close();
  };

};

class reporter_html: public virtual reporter {

 vector<string> headers;
 string testname;
 string style_file = "./includes/style.css";

public:

  explicit reporter_html(string file, bool mode=false, string test="") :
      reporter(file, mode), testname(test) {
  }

  void start();
  void title();

  void tree_title(string);

  void tree_start();
  void tree_end();

  void end();

  void format(const node_info_t&, const string &class_name);

  void format_default(const node_info_t&);
  void format_fail(const node_info_t&);

  void add_row(const node_info_t&);

};

class reporter_log: public virtual reporter {

 string testname;

public:

  explicit reporter_log(string file, string test="") :
      reporter(file, true), testname(test) {
    err_count = 0;
  }

  void start();
  void title() {};

  void tree_title(string) {};

  void tree_start() {};
  void tree_end() {};

  void end();

  void format(const node_info_t&, const string &class_name);

  void format_default(const node_info_t&) {};
  void format_fail(const node_info_t&);

  string get_string_kind();
  string assemble_info(const node_info_t&);
};

#endif  // INCLUDES_FORMATTER_HPP_
