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

#ifndef INCLUDES_EXCLUDER_HPP_
#define INCLUDES_EXCLUDER_HPP_

#include <string>
#include <iostream>

//using namespace std;
using std::string;

enum operations {
  smaller = 0, equals = 1, contains = 2, bigger = ~smaller, different = ~equals
};

/*
 * Class used to store filters
 * Currently used only to filter comments
 */
class excluder {
  string field;
  string valid;
  int op;
  bool negated = false;

  /*
   * Applies a stored operation on val and ref
   * @param val: value to check
   * @param ref: value stored as reference
   */
  bool do_op(string val, string ref) {
    switch (op) {
    case smaller:  // int represented as string is smaller
      return val.size() <= ref.size() && (val.compare(ref) < 0);
    case bigger:   // int represented as string is bigger
      return val.size() >= ref.size() && (val.compare(ref) > 0);
    case equals:
      return !val.compare(ref);
    case contains:
      return !(val.find(ref) == string::npos);
    default:
      return false;
    }
  }

 public:
  excluder(string field, string valid, int op, bool negated = false) :
      field(field), valid(valid), op(op), negated(negated) {
  }

  /*
   * Applies the stored operation on val and the stored reference
   * @param val: value that we want to check
   * @return : the result of the operation
   */
  bool run_check(const string & val) {
    bool rez = do_op(val, valid);

    if (negated)
      return !rez;

    return rez;
  }
};

#endif  // INCLUDES_EXCLUDER_HPP_
