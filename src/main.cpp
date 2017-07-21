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

#include <string>

#include "iterator.hpp"

/* A custom checker that highlights items hit more than 1k times */
string custom_checker(node_info_t inf) {

  if (!inf.found)
    return "missing";

  if (inf.hit_count > 1000)
    return "more_than_1000";

  return "default";

}

int main(int argc, char** argv) {

// Uncomment to register the check defined above
//  register_check(custom_checker);

// Pass it to the ucis iterator to populate it
  return it_main(argc, argv);
}
