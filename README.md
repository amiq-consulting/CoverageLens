# Coverage Lens
### Description
Coverage Lens is an utility (written in C++) that looks for a specified set of RTL code coverage items (statement, branch, condition, etc.) in UCIS compliant coverage databases and flags the ones that are not covered.
CL can be used to check that:
* Directed scenarios hit their target in the DUT
* Excluded code is not executed
* Bins of coverage are covered and assertions are failing or not

### Installation
CL currently works with NCSim and Questa. The executable will be different depending on the tool used to simulate, so you need to specify the vendor when compiling:

```sh
./compile.sh {cadence|mentor} {path_to_simulator}
```
  The path is necessary only for the first compilation, in order to link the UCIS implementation.
### Running CL
Run by using the coverage_lens.sh script.
CL supports a number of runtime parameters:
Passing files:
```sh
--file, -f # file with other arguments, one per line 
--check-file, -c  # check file containing code to be analyzed
--database, -d  # UCISDB (.ucdb or .ucd)
--plan, -p # vPlan
--refinement, -r # waiver file (.vpRefine or .do)
```

Filtering checks:
```sh
--strict-comment, -sc # consider only checks marked with an exact comment
--weak-comment, -wc # consider only checks that contain a string in comment 
--users, -u # consider only checks made by some user
```
Other:
```sh
--mail, -m  # send html report for a run
--verbose, -v # create debug files
--output, -o # change result file name
--list, -l  # find an instance in the DB. If no arg is given prints whole hierarchy
--testname, -t # pass testname 
--quiet, -q   # run in batch mode
--negate, -n  # switch all checks 
--coverage, -g # functional coverage information
```

### Creating a check file
Check files are the way CL knows what code you want to look at. These consist of several "check" commands that describe code coverage items (type, location) and their kind (per instance/ per type).
```
cl_check -k {inst|type} -s {hierarchical_location} -l {line_numeber} -t {code_type} [other descriptors]
```
By default, items pass the check if they were executed at least once.
Note: brackets mean mandatory and square parentheses mean optional;
Examples:

```sh
#Check a statement from line 42, in all "pkt_transmitter" instances
cl_check -k type -s pkt_transmitter -l 42 -t stmt
#Check that a statement from line 42, in all "pkt_transmitter" instances is not covered
cl_check -k type -s pkt_transmitter -l 42 -t stmt -n
#Check all configurations for an expression from line 42, in all "pkt_transmitter" instances
cl_check -k type -s pkt_transmitter -l 42 -t expr
#Check first 3 table rows for an expression from line 42, in all "pkt_transmitter" instances
cl_check -k type -s pkt_transmitter -l 42 -t expr 1 2 3
#Check state "IDLE" from the "trans_fsm" FSM, in all "pkt_transmitter" instances
cl_check -k type -s pkt_transmitter -t state trans_fsm IDLE
#Check transition "IDLE->RESET" from the "trans_fsm" FSM, in all "pkt_transmitter" instances
cl_check -k type -s pkt_transmitter -t state trans_fsm IDLE->RESET

#Check all statement from line 42 to 55, in all "pkt_transmitter" instances
cl_check -k type -s pkt_transmitter -l 42-55 -t stmt

#Check a statement from line 42, in two specific "pkt_transmitter" instances: pkt_transmitter{1,2}
cl_check -k inst -s top/tx1/pkt_transmitter1 -l 42 -t stmt
cl_check -k inst -s top/tx1/pkt_transmitter2 -l 42 -t stmt

#Check if an assert failed or not
cl_check -k inst -p pkg/monitor -t assert ASSERT_NAME

#Check if a coverage bin was covered
cl_check -k inst -p pkg/cov_collector -t cov /covergroup/coverpoint/bin_name
cl_check -k inst -p pkg/cov_collector -t cov /covergroup/coverpoint/array_bin 3
cl_check -f inst -p pkg/cov_collector -t cov /covergroup/cross_name 72
```

### Jumpstart

A simple use:
```sh
$ cat test_file.example
/* Simple check file. All checks are for the “pkt_trans” instance from the top module */

#check a statement from line 332
cl_check -k instance -p /top/pkt_trans -l 332 -t stmt   
# first row in an expression’s table 
cl_check -k instance -p /top/pkt_trans -l 333 -t expr 1  
# transition “IDLE->WAIT”  in a FSM named “state”
cl_check -k instance -p /top/pkt_trans/pkt2mac/mac_req_if -t trans state IDLE->WAIT 
# state “SENT” from a FSM named  “state”
cl_check -k instance -p /top/pkt_trans/pkt2mac/mac_req_if -t state state SENT 
$ ./run.sh --database my_ucisdb --check-file test_file.example -o example --quiet
$ cat example
*CL_ERR in instance top/pkt_trans,line 333: Condition hs_tx_ready_0 was hit 0 times!
*CL_ERR in instance top/pkt_trans/pkt2mac/mac_req_if/state/states/: State SENT was hit 0 times!
*CL_ERR in instance top/pkt_trans/pkt2mac/mac_req_if/state/trans/: Transition IDLE -> WAIT was hit 0 times!
*CL_ERR Total error count: 3!

$ ./run.sh --database my_ucisdb --check-file test_file.example -o example --quiet --negate
$ cat example
*CL_ERR in instance top/pkt_trans,line 333: Condition hs_tx_ready_0 was hit 0 times!
*CL_ERR in instance top/pkt_trans/pkt2mac/mac_req_if/state/states/: State SENT was hit 0 times!
*CL_ERR in instance top/pkt_trans/pkt2mac/mac_req_if/state/trans/: Transition IDLE -> WAIT was hit 0 times!
*CL_ERR Total error count: 3!

*CL_ERR in instance top/pkt_trans,line 332: Branch true_branch was hit 199964 times!
*CL_ERR Total error count: 1!

$ ./run.sh --list mem -d tests/mti/top.ucdb
UCISDB #0 @tests/mti/top.ucdb
/top/pkt_trans/hs_tx_data_fifo/memory
/top/pkt_trans/hs_tx_size_fifo/memory
```
### Functional coverage examples
./coverage_lens.sh -d <path_to_database> -g nof_cvps
This option will print out the total number of coverpoints from the verification environment.

./coverage_lens.sh -d <path_to_database> -g cvp_name <cvp_idx>
Based on the index of the covergroup (the index must be in the range [1, nof_cvps]), the path to the coverpoint 
will be printed out (the path includes the packages, the class name and the covergroup).

./coverage_lens.sh -d <path_to_database> -g nof_bins <cvp_idx>
Given the index of the coverpoint, this parameter will print out the number of bins which contributed to the coverpoint.

./coverage_lens.sh -d <path_to_database> -g bin_name <cvp_idx> <bin_idx>
This option will print out the full path to the bin (including the packages and the classes) that has the bin_idx index
from the coverpoint with the cvp_idx index.

./coverage_lens.sh <path_to_database> -g nof_hits <cvp_idx> <bin_idx>
This option will print out the number of hits of the bin with the bin_idx index from the coverpoint with the index cvp_idx.

./coverage_lens.sh <path_to_database> -g cvp_res <cvp_idx>
This option, with the index of the coverpoint as a parameter will show the percentage of the bins that have been
hit from the coverpoint.

./coverage_lens.sh <path_to_database> -g  nof_cvgs
This options takes no parameters and prints the total number of covergroups from the verification environment.

./coverage_lens.sh <path_to_database> -g cvg_name <cvg_idx>
Similar to the cvp_name option, this option will print the full path to the covergroup with the cvg_idx index.

./coverage_lens.sh <path_to_database> -g cvg_res <cvg_idx>
This option gets the index of the covergroup as a parameter and prints the percentage of hit bins from the covergroup.

