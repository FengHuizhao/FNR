#ifndef _COMPARE_H
#define _COMPARE_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>

extern int DATA_BLOCK_NUM_PER_STRIPE; // Number of data blocks per stripe
extern int PARITY_BLOCK_NUM_PER_STRIPE; // Number of parity blocks per stripe
extern int BLOCK_PER_STRIPE; // Total number of blocks per stripe
extern int STRIPE_NUM; // Number of stripes
extern int NODE_NUM_PER_RACK; // Number of nodes per rack
extern int RACK_NUM; // Number of racks
extern int NODE_NUM; // Total number of nodes
extern int BLOCK_NUM; // Total number of blocks
extern int TOTAL_REPAIR_LINK_NUM_FIXED; // Total number of repair links for fixed cost scheme
extern int TOTAL_REPAIR_LINK_NUM_FLUCTUATE; // Total number of repair links for fluctuating cost scheme
extern int MAX_RACK_TIME_FIXED; // Maximum rack time for fixed scheme
extern float MAX_RACK_TIME_FLUCTUATE; // Maximum rack time for fluctuating scheme
extern int **FixedRackTransfer; // Matrix of transmission costs between racks

typedef struct {
    int cost; // Transmission cost
    float prob; // Repair probability for this cost
} CostProbPair;

typedef struct {
    int *stripeBlockCount; // Number of blocks from each stripe in this rack
    int Tu; // Rack's upload time
    int Td; // Rack's download time
    float expTu; // Mathematical expectation of rack's upload time
    float expTd; // Mathematical expectation of rack's download time
    int TuCount; // Number of possible "cost-probability" pairs for rack's upload time
    int TdCount; // Number of possible "cost-probability" pairs for rack's download time
    CostProbPair *TuOptions; // Pointer to array of "cost-probability" pairs for rack's upload time
    CostProbPair *TdOptions; // Pointer to array of "cost-probability" pairs for rack's download time
    bool isMTRack; // Is it the rack that causes the bottleneck（Tu or Td equals to MAX_RACK_TIME）
    bool isTuMax; // Is the bottleneck caused by uploading（Tu equals to MAX_RACK_TIME）
    bool isTdMax; // Is the bottleneck caused by downloading（Td equals to MAX_RACK_TIME）
} Rack;

typedef struct {
    int rackId; // Node's rack ID
    bool *isSet; // Whether the node has a block from a particular stripe
} Node;

typedef struct {
    int sourceStripe; // Block's source stripe
    int rackId; // Block's rack ID
    int nodeId; // Block's node ID
    bool isLost; // Whether the block is lost
    int previousNodeId; // Node ID where the block was previously located
} Block;

typedef struct {
    int *helpers; // IDs of helpers in the solution
    int requester; // ID of the requester in the solution
    int fromRackSize; // Number of source racks
    int *fromRack; // Source racks for repair (racks where helpers are located after removing duplicates)
    int toRack; // Target rack for repair (rack where the requester is located)
    int cost; // Cost of the solution
} FixedSolution;

typedef struct {
    int *helpers; // IDs of helpers in the solution
    int requester; // ID of the requester in the solution
    int fromRackSize; // Number of source racks
    int *fromRack; // Source racks for repair (racks where helpers are located after removing duplicates)
    int toRack; // Target rack for repair (rack where the requester is located)
    int count; // Number of possible "cost-probability" pairs
    CostProbPair *options; // Pointer to array of "cost-probability" pairs
} FluctuateSolution;

typedef struct {
    int fromRack; // Source rack
    int toRack; // Target rack
    int cost; // Repair cost
    int count; // Number of possible "cost-probability" pairs
    CostProbPair *options; // Pointer to array of "cost-probability" pairs
    int startTime; // Repair start time
    int endTime; // Repair end time
    bool isDispatched; // Whether the repair is completed
} Link;

typedef struct {
    int count; // Number of possible "cost-probability" pairs
    CostProbPair *options; // Pointer to array of "cost-probability" pairs
} RackTransfer;

typedef struct{
    int totalTime; // Total time
    float prob; // Probability of this time
} Result;

Rack *racks;
Node *nodes;
Block *blocks;
FixedSolution *fixedSolutions;
FixedSolution *substituteFixedSolus;
FluctuateSolution *fluctuateSolutions;
FluctuateSolution *substituteFluctuateSolus;
Link *fixedLinks;
Link *fluctuateLinks;
RackTransfer **rackTransfer;

// Read the current configuration settings from the file
void read_config(const char *filename);

// Checking duplicate costs (fluctuate)
int duplicate_check(int val, int *arr, int count);

// Read inter-rack transfer costs from file (fixed)
void read_fixed_rack_transfer(const char *filename);

// Read inter-rack transfer costs and probabilities from file (fluctuate)
void read_fluctuate_rack_transfer(const char *filename);

// Randomly shuffle the order of links
void shuffle(Link *links, int totalRepairLinksNum);

// Check memory allocation for arrays
void check_allocation(void *ptr);

// Print probability and cumulative probability of schemes
void print_prob(Result *results, int resultCount);

// Print the values and cumulative probability of the scheme with the highest expected value
void print_expected_highest_scheme(Result *results, int resultCount);

// Return the index of the scheme with the highest expected cost
int expected_highest_cost_index(Result *results, int resultCount);

// Write MT values to file
void print_MT_to_file(const char *filename, const char *scheme_fixed, int MT_fixed, const char *scheme_float, float MT_float);

// Write a blank line to file
void print_empty_line_to_file(const char *filename);

// Write final results to file
void print_results_to_file(const char *filename, int RandomTotalRepairCost, Result* RandomResults, int RandomResultCount, int AZTotalRepairCost, Result* AZResults, int AZResultCount, int CTPTotalRepairCost, Result* CTPResults, int CTPResultCount);

// Write final expected-value results to file
void print_exp_results_to_file(const char *filename, int RandomTotalRepairCost, Result* RandomResults, int RandomResultCount, int AZTotalRepairCost, Result* AZResults, int AZResultCount, int CTPTotalRepairCost, Result* CTPResults, int CTPResultCount);

// Randomly select helpers
void random_choose_helpers(int *validHelpers, int validHelpersNum, int stripe);

// Calculate repair cost (fixed)
void solution_repair_cost(FixedSolution *solus, int stripe);

// Check whether a new link conflicts with scheduled links (fixed)
bool check_conflict_fixed(int newLinkId, int currentTime);

// Schedule repair links for fixed-cost schemes (fixed)
void schedule_fixed_repair_links();

// Generate all possible (cost-probability) pair combinations for one stripe scheme (fluctuate)
void generate_costprob_combinations(FluctuateSolution *solus, int stripe, int *indices, int currentIndex, int *optionIndex);

// Merge entries with identical costs in (cost-probability) combinations (fluctuate)
void merge_duplicate(FluctuateSolution *solus, int stripe);

// Generate all possible repair cost options (fluctuate)
void solution_repair_options(FluctuateSolution *solus, int stripe);

// Reset links without printing (fluctuate)
void reset_fluctuate_links_without_print(FluctuateSolution *solus);

// Copy link structure (deep copy of all fields)
Link* copy_links(Link* original, int totalRepairLinksNum);

// Check link conflicts (fluctuate)
bool check_conflict(Link* links, int currentIdx, int startTime, int endTime);

// Randomly sample an option index according to probability distribution
int sample_option_index(CostProbPair *options, int count);

// Analyze all possiblilities (fluctuate)
Result* analyze_all_possibilities(Link* originalLinks, int* resultCount);

// sorting results by totalTime (fluctuate)
int compare_results(const void* a, const void* b);

// Sort results array by time (fluctuate)
void sort_results_by_time(Result* results, int count);

// Reset links (fixed)
void reset_fixed_links();

// Reset links (fluctuate)
void reset_fluctuate_links();

// Calculate single-stripe cost of AZ scheme (fixed)
int compute_AZ_solution_fixed_cost(int *currentHelpers, int *validRequesters, int validRequestersIndex, int *fromRackSize, int *fromRack);

// Recursively generate all candidate combinations for one stripe (fixed)
void generate_AZ_one_stripe_combinations_fixed(int stripe, int *validHelpers, int validHelpersNum, int *validRequesters, int validRequestersNum, int start, int *currentHelpers, int current_pos);

// Generate all possible (cost-probability) pair combinations for one AZ stripe scheme (fluctuate)
void generate_AZ_one_stripe_costprob_combinations(FluctuateSolution *NewSolution, int *indices, int currentIndex, int *optionIndex);

// Merge entries with identical costs in AZ (cost-probability) combinations (fluctuate)
void merge_AZ_duplicate(FluctuateSolution *NewSolution);

// Calculate single-stripe cost of AZ scheme (fluctuate)
void compute_AZ_one_stripe_solution(int *currentHelpers, int *validRequesters, int validRequestersIndex, int *fromRackSize, int *fromRack, FluctuateSolution *NewSolution);

// Calculate maximum expected cost of one stripe under AZ scheme (fluctuate)
int compute_AZ_one_stripe_solution_maxExp_cost(FluctuateSolution *solution);

// Recursively generate all candidate combinations for one stripe (fluctuate)
void generate_AZ_one_stripe_combinations(int stripe, int *validHelpers, int validHelpersNum, int *validRequesters, int validRequestersNum, int start, int *currentHelpers, int current_pos);

// Compute Tu and Td (fixed)
void compute_TuTd_fixed(FixedSolution *solus);

// Find MT for current scheme (fixed)
int find_max_rack_time_fixed();

// Mark racks that have reached MAX_RACK_TIME (fixed)
void mark_max_rack_time_rack_fixed();

// Merge a new link's probability distribution with the current rack's total distribution
void dp_merge_link(CostProbPair **current_options, int *current_count, CostProbPair *link_options, int link_option_count);

// Compute Tu and Td (fluctuate)
void compute_TuTd_fluctuate(FluctuateSolution *solus);

// Find MT for current scheme (fluctuate)
float find_max_rack_time_fluctuate();

// Mark racks that have reached MAX_RACK_TIME (fluctuate)
void mark_max_rack_time_rack_fluctuate();

// Clear MAX_RACK_TIME marks
void clear_max_rack_time_rack_marks();

// Copy solutions (fixed)
void copy_fixed_solution(FixedSolution *solusFrom, FixedSolution *solusTo, int stripe);

// Copy solutions (fluctuate)
void copy_fluctuate_solution(FluctuateSolution *solusFrom, FluctuateSolution *solusTo, int stripe);

// Calculate single-stripe cost of CTP scheme (fixed)
void compute_CTP_solution_fixed_cost(FixedSolution *solus, int stripe);

// Generate all valid substitute helper combinations (fixed)
void generate_substitute_helpers_fixed(int stripe, int *validHelpers, int validHelpersNum, int start, int *currentHelpers, int current_pos, bool *optimized);

// Generate all valid substitute helper combinations (fluctuate)
void generate_substitute_helpers_fluctuate(int stripe, int *validHelpers, int validHelpersNum, int start, int *currentHelpers, int current_pos, bool *optimized);

// Search new helpers while keeping requesters of current stripe unchanged
void find_new_helpers(int stripe, int MTRack, bool *optimized, bool isFixed);

// Search new requesters while keeping helpers of current stripe unchanged (fixed)
void find_new_requester_fixed(int stripe, int MTRack, bool *optimized);

// Search new requesters while keeping helpers of current stripe unchanged (fluctuate)
void find_new_requester_fluctuate(int stripe, int MTRack, bool *optimized);

#endif