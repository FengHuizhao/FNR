#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include "Compare.h"

void read_config(const char *filename){
    FILE *file = fopen(filename, "r");
    if(file == NULL){
        fprintf(stderr, "Failed to open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int ret = fscanf(file, "DATA_BLOCK_NUM_PER_STRIPE:%d PARITY_BLOCK_NUM_PER_STRIPE:%d STRIPE_NUM:%d NODE_NUM_PER_RACK:%d RACK_NUM:%d", &DATA_BLOCK_NUM_PER_STRIPE, &PARITY_BLOCK_NUM_PER_STRIPE, &STRIPE_NUM, &NODE_NUM_PER_RACK, &RACK_NUM);
    if (ret != 5) {
        fprintf(stderr, "Error: failed to parse config file, expected 5 parameters\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);

    BLOCK_PER_STRIPE = DATA_BLOCK_NUM_PER_STRIPE + PARITY_BLOCK_NUM_PER_STRIPE;
    NODE_NUM = RACK_NUM * NODE_NUM_PER_RACK;
    BLOCK_NUM = STRIPE_NUM * BLOCK_PER_STRIPE;

    printf("DATA_BLOCK_NUM_PER_STRIPE: %d\n", DATA_BLOCK_NUM_PER_STRIPE);
    printf("PARITY_BLOCK_NUM_PER_STRIPE: %d\n", PARITY_BLOCK_NUM_PER_STRIPE);
    printf("BLOCK_PER_STRIPE: %d\n", BLOCK_PER_STRIPE);
    printf("STRIPE_NUM: %d\n", STRIPE_NUM);
    printf("NODE_NUM_PER_RACK: %d\n", NODE_NUM_PER_RACK);
    printf("RACK_NUM: %d\n", RACK_NUM);
    printf("NODE_NUM: %d\n", NODE_NUM);
    printf("BLOCK_NUM: %d\n", BLOCK_NUM);
}

int duplicate_check(int val, int *arr, int count){
    for(int i = 0; i < count; i++){
        if(arr[i] == val){
            return 1;
        }
    }
    return 0;
}

void read_fixed_rack_transfer(const char *filename){
    FILE *file = fopen(filename, "r");
    if(file == NULL){
        fprintf(stderr, "Failed to open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int fromRack, toRack, cost;
    int line_count = 0;

    while(fscanf(file, "%d %d %d", &fromRack, &toRack, &cost) == 3){
        if(fromRack >= 0 && fromRack < RACK_NUM && toRack >= 0 && toRack < RACK_NUM){
            FixedRackTransfer[fromRack][toRack] = cost;
            FixedRackTransfer[toRack][fromRack] = cost;
            line_count++;
        } else {
            fprintf(stderr, "error: invalid rack number in line %d (fromRack=%d, toRack=%d)\n", line_count + 1, fromRack, toRack);
        }
    }

    fclose(file);
}

void read_fluctuate_rack_transfer(const char *filename){
    FILE *file = fopen(filename, "r");
    if(file == NULL){
        fprintf(stderr, "Failed to open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int fromRack, toRack, count, cost;
    float prob;
    int line_count = 0;

    while(1){
        int ret = fscanf(file, "%d-%d %d:", &fromRack, &toRack, &count);
        if(ret == EOF){
            break;
        }
        if(fromRack >= 0 && fromRack < RACK_NUM && toRack >= 0 && toRack < RACK_NUM){
            rackTransfer[fromRack][toRack].count = count;
            rackTransfer[fromRack][toRack].options = (CostProbPair *)calloc(count, sizeof(CostProbPair));
            rackTransfer[toRack][fromRack].count = count;
            rackTransfer[toRack][fromRack].options = (CostProbPair *)calloc(count, sizeof(CostProbPair));
            for(int i = 0; i < count; ++i){
                int ret = fscanf(file, "[%d-%f]", &cost, &prob);
                if (ret != 2) {
                    fprintf(stderr, "Warning: failed to parse line, skip\n");
                    break;
                }
                rackTransfer[fromRack][toRack].options[i].cost = cost;
                rackTransfer[fromRack][toRack].options[i].prob = prob;
                rackTransfer[toRack][fromRack].options[i].cost = cost;
                rackTransfer[toRack][fromRack].options[i].prob = prob;
            }
            line_count++;
        } else {
            fprintf(stderr, "error: invalid rack number in line %d (fromRack=%d, toRack=%d)\n", line_count + 1, fromRack, toRack);
        }
    }

    fclose(file);
}

void shuffle(Link *links, int totalRepairLinksNum){
    for(int i = totalRepairLinksNum - 1; i > 0; --i){
        int j = rand() % (i + 1);
        Link tmp = links[i];
        links[i] = links[j];
        links[j] = tmp;
    }
}

void check_allocation(void *ptr){
    if(ptr == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
}

void print_prob(Result *results, int resultCount){
    float cumulativeProb = 0.0f;
    for(int i = 0; i < resultCount; ++i){
        cumulativeProb += results[i].prob;
        printf("  Total Time: %d, Prob: %.4f%%, CumProb: %.4f%%\n", results[i].totalTime, results[i].prob * 100, cumulativeProb * 100);
    }
}

void print_expected_highest_scheme(Result *results, int resultCount){
    float maxExpectedValue = -1.0f;
    int maxIndex = -1;
    float cumulativeProb[resultCount];
    memset(cumulativeProb, 0, sizeof(float) * resultCount);
    for(int i = 0; i < resultCount; ++i){
        cumulativeProb[i] = (i > 0) ? cumulativeProb[i - 1] + results[i].prob : results[i].prob;
        float expectedValue = results[i].totalTime * results[i].prob;
        if(expectedValue > maxExpectedValue){
            maxExpectedValue = expectedValue;
            maxIndex = i;
        }
    }
    if(maxIndex != -1){
        printf("  Expected Highest Scheme cost: %d, Prob: %.4f%%, CumProb: %.4f%%, maxExpectedValue: %.4f\n", results[maxIndex].totalTime, results[maxIndex].prob * 100, cumulativeProb[maxIndex] * 100, maxExpectedValue);
    }
}

int expected_highest_cost_index(Result *results, int resultCount){
    float maxExpectedValue = -1.0f;
    int maxIndex = -1;
    for(int i = 0; i < resultCount; ++i){
        float expectedValue = results[i].totalTime * results[i].prob;
        if(expectedValue > maxExpectedValue){
            maxExpectedValue = expectedValue;
            maxIndex = i;
        }
    }
    return maxIndex;
}

void print_MT_to_file(const char *filename, const char *scheme_fixed, int MT_fixed, const char *scheme_float, float MT_float){
    FILE *file = fopen(filename, "a");
    if(file == NULL){
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }

    fprintf(file, "%s: %d\n%s: %f\n", scheme_fixed, MT_fixed, scheme_float, MT_float);

    fclose(file);
}

void print_empty_line_to_file(const char *filename){
    FILE *file = fopen(filename, "a");
    if(file == NULL){
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }

    fprintf(file, "\n");

    fclose(file);
}

void print_results_to_file(const char *filename, int RandomTotalRepairCost, Result* RandomResults, int RandomResultCount, int AZTotalRepairCost, Result* AZResults, int AZResultCount, int CTPTotalRepairCost, Result* CTPResults, int CTPResultCount){
    FILE *file = fopen(filename, "a");
    if(file == NULL){
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }

    fprintf(file, "FIXED:\n");
    fprintf(file, "  Random Scheme Total Repair Cost: %d\n", RandomTotalRepairCost);
    fprintf(file, "  AZ Scheme Total Repair Cost: %d\n", AZTotalRepairCost);
    fprintf(file, "  CTP Scheme Total Repair Cost: %d\n", CTPTotalRepairCost);
    fprintf(file, "FLUCTUATE:\n");
    fprintf(file, "  Random Scheme Possible Results:\n");
    float cumulativeProb = 0.0f;
    for(int i = 0; i < RandomResultCount; ++i){
        cumulativeProb += RandomResults[i].prob;
        fprintf(file, "  Total Time: %d, Prob: %.4f%%, CumProb: %.4f%%\n", RandomResults[i].totalTime, RandomResults[i].prob * 100, cumulativeProb * 100);
    }
    fprintf(file, "  AZ Scheme Possible Results:\n");
    cumulativeProb = 0.0f;
    for(int i = 0; i < AZResultCount; ++i){
        cumulativeProb += AZResults[i].prob;
        fprintf(file, "  Total Time: %d, Prob: %.4f%%, CumProb: %.4f%%\n", AZResults[i].totalTime, AZResults[i].prob * 100, cumulativeProb * 100);
    }
    fprintf(file, "  CTP Scheme Possible Results:\n");
    cumulativeProb = 0.0f;
    for(int i = 0; i < CTPResultCount; ++i){
        cumulativeProb += CTPResults[i].prob;
        fprintf(file, "  Total Time: %d, Prob: %.4f%%, CumProb: %.4f%%\n", CTPResults[i].totalTime, CTPResults[i].prob * 100, cumulativeProb * 100);
    }
    fprintf(file, "\n");

    fclose(file);
}

void print_exp_results_to_file(const char *filename, int RandomTotalRepairCost, Result* RandomResults, int RandomResultCount, int AZTotalRepairCost, Result* AZResults, int AZResultCount, int CTPTotalRepairCost, Result* CTPResults, int CTPResultCount){
    FILE *file = fopen(filename, "a");
    if(file == NULL){
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }

    fprintf(file, "Random-FIXED: %d\n", RandomTotalRepairCost);
    fprintf(file, "AZ-FIXED: %d\n", AZTotalRepairCost);
    fprintf(file, "CTP-FIXED: %d\n", CTPTotalRepairCost);
    fprintf(file, "Random-FLUCTUATE: ");
    int RR_maxIndex = expected_highest_cost_index(RandomResults, RandomResultCount);
    float cumulativeProb = 0.0f;
    for(int i = 0; i <= RR_maxIndex; ++i){
        cumulativeProb += RandomResults[i].prob;
    }
    fprintf(file, "%d, Prob: %.4f%%, CumProb: %.4f%%, exp: %.4f\n", RandomResults[RR_maxIndex].totalTime, RandomResults[RR_maxIndex].prob * 100, cumulativeProb * 100, RandomResults[RR_maxIndex].totalTime * RandomResults[RR_maxIndex].prob);
    fprintf(file, "AZ-FLUCTUATE: ");
    int AZ_maxIndex = expected_highest_cost_index(AZResults, AZResultCount);
    cumulativeProb = 0.0f;
    for(int i = 0; i <= AZ_maxIndex; ++i){
        cumulativeProb += AZResults[i].prob;
    }
    fprintf(file, "%d, Prob: %.4f%%, CumProb: %.4f%%, exp: %.4f\n", AZResults[AZ_maxIndex].totalTime, AZResults[AZ_maxIndex].prob * 100, cumulativeProb * 100, AZResults[AZ_maxIndex].totalTime * AZResults[AZ_maxIndex].prob);
    fprintf(file, "CTP-FLUCTUATE: ");
    int CTP_maxIndex = expected_highest_cost_index(CTPResults, CTPResultCount);
    cumulativeProb = 0.0f;
    for(int i = 0; i <= CTP_maxIndex; ++i){
        cumulativeProb += CTPResults[i].prob;
    }
    fprintf(file, "%d, Prob: %.4f%%, CumProb: %.4f%%, exp: %.4f\n", CTPResults[CTP_maxIndex].totalTime, CTPResults[CTP_maxIndex].prob * 100, cumulativeProb * 100, CTPResults[CTP_maxIndex].totalTime * CTPResults[CTP_maxIndex].prob);
    fprintf(file, "\n");

    fclose(file);
}

void random_choose_helpers(int *validHelpers, int validHelpersNum, int stripe){
    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        int helperIndex = rand() % validHelpersNum;
        fixedSolutions[stripe].helpers[i] = validHelpers[helperIndex];
        fluctuateSolutions[stripe].helpers[i] = validHelpers[helperIndex];
        validHelpers[helperIndex] = validHelpers[validHelpersNum - 1];
        validHelpersNum--;
    }
}

void solution_repair_cost(FixedSolution *solus, int stripe){
    solus[stripe].toRack = nodes[solus[stripe].requester].rackId;
    printf("  toRack %d\n", solus[stripe].toRack);
    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        solus[stripe].fromRack[i] = blocks[solus[stripe].helpers[i]].rackId;
    }

    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        int is_duplicate = 0;
        if(solus[stripe].fromRack[i] == solus[stripe].toRack){
            is_duplicate = 1;
        }
        if(!is_duplicate){
            for(int j = 0; j < solus[stripe].fromRackSize; ++j){
                if(solus[stripe].fromRack[i] == solus[stripe].fromRack[j]){
                    is_duplicate = 1;
                    break;
                }
            }
        }
        if(!is_duplicate){
            solus[stripe].fromRack[solus[stripe].fromRackSize++] = solus[stripe].fromRack[i];
        }
    }

    for(int i = 0; i < solus[stripe].fromRackSize; ++i){
        printf("  fromRack[%d] = %d\n", i, solus[stripe].fromRack[i]);
        if(solus[stripe].fromRack[i] != solus[stripe].toRack){
            solus[stripe].cost += FixedRackTransfer[solus[stripe].fromRack[i]][solus[stripe].toRack];
        }
    }
}

bool check_conflict_fixed(int newLinkId, int currentTime){
    for(int i = 0; i < newLinkId; ++i){
        if(!fixedLinks[i].isDispatched){
            return true;
        }
    }
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; i++){
        if(fixedLinks[i].startTime > currentTime || fixedLinks[i].endTime <= currentTime){
            continue;
        }
        if(fixedLinks[i].fromRack == fixedLinks[newLinkId].fromRack){
            return true;
        }
        if(fixedLinks[i].toRack == fixedLinks[newLinkId].toRack){
            return true;
        }
    }
    return false;
}

void schedule_fixed_repair_links(){
    int currentTime = 0;
    int completedLinksNum = 0;
    while(completedLinksNum < TOTAL_REPAIR_LINK_NUM_FIXED){
        bool newScheduled = false;
        for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
            if(!fixedLinks[i].isDispatched){
                if(!check_conflict_fixed(i, currentTime)){
                    fixedLinks[i].startTime = currentTime;
                    fixedLinks[i].endTime = currentTime + fixedLinks[i].cost;
                    fixedLinks[i].isDispatched = true;
                    newScheduled = true;
                    printf("  time %d: start to repair link[%d] R%d-R%d (cost %d)\n", currentTime, i, fixedLinks[i].fromRack, fixedLinks[i].toRack, fixedLinks[i].cost);
                }
            }
        }
        if(!newScheduled){
            int nextTime = INT_MAX;
            for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
                if(fixedLinks[i].isDispatched && fixedLinks[i].endTime > currentTime && fixedLinks[i].endTime < nextTime){
                    nextTime = fixedLinks[i].endTime;
                }
            }
            if(nextTime == INT_MAX){
                break;
            }
            currentTime = nextTime;
            printf("  time %d: ", currentTime);
            bool has_completed = false;
            for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
                if(fixedLinks[i].endTime == currentTime){
                    if(!has_completed){
                        has_completed = true;
                    } else {
                        printf(", ");
                    }
                    printf("Link[%d] R%d-R%d", i, fixedLinks[i].fromRack, fixedLinks[i].toRack);
                    completedLinksNum++;
                }
            }
            if(has_completed){
                printf(" completed the repair\n");
            }
        }
    }
}

void generate_costprob_combinations(FluctuateSolution *solus, int stripe, int *indices, int currentIndex, int *optionIndex){
    if(currentIndex == solus[stripe].fromRackSize){
        int totalCost = 0;
        float totalProb = 1.0f;
        for(int i = 0; i < solus[stripe].fromRackSize; ++i){
            int idx = indices[i];
            totalCost += rackTransfer[solus[stripe].fromRack[i]][solus[stripe].toRack].options[idx].cost;
            totalProb *= rackTransfer[solus[stripe].fromRack[i]][solus[stripe].toRack].options[idx].prob;
        }
        solus[stripe].options[*optionIndex].cost = totalCost;
        solus[stripe].options[*optionIndex].prob = totalProb;
        (*optionIndex)++;
        return;
    }
    
    for(int i = 0; i < rackTransfer[solus[stripe].fromRack[currentIndex]][solus[stripe].toRack].count; i++){
        indices[currentIndex] = i;
        generate_costprob_combinations(solus, stripe, indices, currentIndex + 1, optionIndex);
    }
}

void merge_duplicate(FluctuateSolution *solus, int stripe){
    int maxCost = 0;
    for(int i = 0; i < solus[stripe].count; ++i){
        if(solus[stripe].options[i].cost > maxCost){
            maxCost = solus[stripe].options[i].cost;
        }
    }

    float *tempProbs = (float *)calloc(maxCost + 1, sizeof(float));

    for(int i = 0; i < solus[stripe].count; ++i){
        int cost = solus[stripe].options[i].cost;
        tempProbs[cost] += solus[stripe].options[i].prob;
    }

    int newCount = 0;
    for(int cost = 0; cost <= maxCost; ++cost){
        if(tempProbs[cost] > 1e-9){
            newCount++;
        }
    }

    solus[stripe].count = newCount;
    solus[stripe].options = (CostProbPair *)realloc(solus[stripe].options, newCount * sizeof(CostProbPair));
    if(solus[stripe].options == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        free(tempProbs);
        return;
    }
    int index = 0;
    for(int cost = 0; cost <= maxCost; ++cost){
        if(tempProbs[cost] > 1e-9){
            solus[stripe].options[index].cost = cost;
            solus[stripe].options[index].prob = tempProbs[cost];
            index++;
        }
    }

    free(tempProbs);
}

void solution_repair_options(FluctuateSolution *solus, int stripe){
    solus[stripe].toRack = nodes[solus[stripe].requester].rackId;
    solus[stripe].fromRackSize = 0;
    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        solus[stripe].fromRack[i] = blocks[solus[stripe].helpers[i]].rackId;
    }

    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        int is_duplicate = 0;
        if(solus[stripe].fromRack[i] == solus[stripe].toRack){
            is_duplicate = 1;
        }
        if(!is_duplicate){
            for(int j = 0; j < solus[stripe].fromRackSize; ++j){
                if(solus[stripe].fromRack[i] == solus[stripe].fromRack[j]){
                    is_duplicate = 1;
                    break;
                }
            }
        }
        if(!is_duplicate){
            solus[stripe].fromRack[solus[stripe].fromRackSize++] = solus[stripe].fromRack[i];
        }
    }

    solus[stripe].count = 1;
    for(int i = 0; i < solus[stripe].fromRackSize; ++i){
        solus[stripe].count *= rackTransfer[solus[stripe].fromRack[i]][solus[stripe].toRack].count;
    }
    solus[stripe].options = (CostProbPair *)realloc(solus[stripe].options, solus[stripe].count * sizeof(CostProbPair));
    int *indices = (int *)calloc(solus[stripe].fromRackSize, sizeof(int));

    int optionIndex = 0;
    generate_costprob_combinations(solus, stripe, indices, 0, &optionIndex);
    merge_duplicate(solus, stripe);

    free(indices);
}

void reset_fluctuate_links_without_print(FluctuateSolution *solus){
    TOTAL_REPAIR_LINK_NUM_FLUCTUATE = 0;
    for(int i = 0; i < STRIPE_NUM; ++i){
        for(int j = 0; j < solus[i].fromRackSize; ++j){
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack = solus[i].fromRack[j];
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack = solus[i].toRack;
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].count = rackTransfer[fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack][fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack].count;       
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options = (CostProbPair *)malloc(fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].count * sizeof(CostProbPair));
            for(int k = 0; k < fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].count; ++k){
                fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options[k].cost = rackTransfer[fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack][fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack].options[k].cost;
                fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options[k].prob = rackTransfer[fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack][fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack].options[k].prob;
            }
            TOTAL_REPAIR_LINK_NUM_FLUCTUATE++;
        }
    }
}

Link* copy_links(Link* original, int totalRepairLinksNum){
    Link* copy = (Link*)calloc(totalRepairLinksNum, sizeof(Link));
    if(!copy){
        fprintf(stderr, "Memory allocation failed (copy_links)\n");
        exit(EXIT_FAILURE);
    }
    
    for(int i = 0; i < totalRepairLinksNum; ++i){
        copy[i].fromRack = original[i].fromRack;
        copy[i].toRack = original[i].toRack;
        copy[i].cost = original[i].cost;
        copy[i].options = original[i].options;
        copy[i].count = original[i].count;
        copy[i].startTime = original[i].startTime;
        copy[i].endTime = original[i].endTime;
        copy[i].isDispatched = original[i].isDispatched;
    }
    return copy;
}

bool check_conflict(Link* links, int currentIdx, int startTime, int endTime){
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i){
        if(i == currentIdx || !links[i].isDispatched)
            continue;
        bool timeOverlap = (startTime < links[i].endTime) && (links[i].startTime < endTime);
        bool rackConflict = (links[i].fromRack == links[currentIdx].fromRack) || (links[i].toRack == links[currentIdx].toRack);
        if(timeOverlap && rackConflict){
            return true;
        }
    }
    return false;
}

int sample_option_index(CostProbPair *options, int count){
    double r = (double)rand() / RAND_MAX;
    double cumProb = 0.0;
    for(int i = 0; i < count; i++){
        cumProb += options[i].prob;
        if(r <= cumProb){
            return i;
        }
    }
    return count - 1;
}

Result* analyze_all_possibilities(Link* originalLinks, int* resultCount) {
    const int SIMULATION_ROUNDS = 20000; 
    int max_possible_time = 10000; 
    
    int *timeFrequency = (int *)calloc(max_possible_time, sizeof(int));
    if (!timeFrequency) {
        fprintf(stderr, "Memory allocation failed for timeFrequency\n");
        return NULL;
    }

    int maxObservedTime = 0;
    int minObservedTime = max_possible_time;

    Link *simLinks = copy_links(originalLinks, TOTAL_REPAIR_LINK_NUM_FLUCTUATE);

    printf("Starting Aligned Monte Carlo Simulation (%d rounds)...\n", SIMULATION_ROUNDS);

    for (int round = 0; round < SIMULATION_ROUNDS; ++round) {
        int sampledCosts[TOTAL_REPAIR_LINK_NUM_FLUCTUATE];
        for (int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i) {
            simLinks[i] = originalLinks[i];
            simLinks[i].isDispatched = false;
            simLinks[i].startTime = -1;
            simLinks[i].endTime = -1;
            
            int pickedIdx = sample_option_index(originalLinks[i].options, originalLinks[i].count);
            sampledCosts[i] = originalLinks[i].options[pickedIdx].cost;
        }

        int currentTime = 0;
        int completedLinksNum = 0;

        while (completedLinksNum < TOTAL_REPAIR_LINK_NUM_FLUCTUATE) {
            bool canSchedule = false;

            int minUnDispatchedIndex = -1;
            for (int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i) {
                if (!simLinks[i].isDispatched) {
                    minUnDispatchedIndex = i;
                    break;
                }
            }

            if (minUnDispatchedIndex != -1) {
                int i = minUnDispatchedIndex;
                int cost = sampledCosts[i];
                int startTime = currentTime;
                int endTime = startTime + cost;

                if (!check_conflict(simLinks, i, startTime, endTime)) {
                    canSchedule = true;
                    simLinks[i].isDispatched = true;
                    simLinks[i].startTime = startTime;
                    simLinks[i].endTime = endTime;
                }
            }

            if (!canSchedule) {
                int nextTime = INT_MAX;
                bool hasDispatched = false;
                
                for (int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i) {
                    if (simLinks[i].isDispatched && simLinks[i].endTime > currentTime) {
                        hasDispatched = true;
                        if (simLinks[i].endTime < nextTime) {
                            nextTime = simLinks[i].endTime;
                        }
                    }
                }

                if (!hasDispatched) {
                    int i = minUnDispatchedIndex;
                    if (i != -1) {
                        int cost = originalLinks[i].options[0].cost;
                        int startTime = currentTime;
                        int endTime = startTime + cost;

                        simLinks[i].isDispatched = true;
                        simLinks[i].startTime = startTime;
                        simLinks[i].endTime = endTime;
                    } else {
                        break;
                    }
                } else {
                    currentTime = nextTime;
                    int newCompleted = 0;
                    for (int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i) {
                        if (simLinks[i].endTime == nextTime) {
                            newCompleted++;
                        }
                    }
                    completedLinksNum += newCompleted;
                }
            }
        }

        if (currentTime < max_possible_time) {
            timeFrequency[currentTime]++;
            if (currentTime > maxObservedTime) maxObservedTime = currentTime;
            if (currentTime < minObservedTime) minObservedTime = currentTime;
        }
    }

    *resultCount = 0;
    for (int t = minObservedTime; t <= maxObservedTime; ++t) {
        if (timeFrequency[t] > 0) {
            (*resultCount)++;
        }
    }

    Result* results = (Result*)malloc((*resultCount) * sizeof(Result));
    int idx = 0;
    for (int t = minObservedTime; t <= maxObservedTime; ++t) {
        if (timeFrequency[t] > 0) {
            results[idx].totalTime = t;
            results[idx].prob = (float)timeFrequency[t] / SIMULATION_ROUNDS;
            idx++;
        }
    }

    free(timeFrequency);
    free(simLinks);
    
    return results;
}

int compare_results(const void* a, const void* b){
    int timeA = (*(Result*)a).totalTime;
    int timeB = (*(Result*)b).totalTime;

    if(timeA < timeB) return -1;
    if(timeA > timeB) return 1;
    return 0;
}

void sort_results_by_time(Result* results, int count){
    if(results == NULL || count <= 1){
        return;
    }

    qsort(results, count, sizeof(Result), compare_results);
}

void reset_fixed_links(){
    TOTAL_REPAIR_LINK_NUM_FIXED = 0;
    for(int i = 0; i < STRIPE_NUM; ++i){
        for(int j = 0; j < fixedSolutions[i].fromRackSize; ++j){
            fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].fromRack = fixedSolutions[i].fromRack[j];
            fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].toRack = fixedSolutions[i].toRack;
            fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].cost = FixedRackTransfer[fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].fromRack][fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].toRack];
            printf("  fixedLink[%d]: fromRack=%d, toRack=%d, cost=%d\n", TOTAL_REPAIR_LINK_NUM_FIXED, fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].fromRack, fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].toRack, fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].cost);
            fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].startTime = -1;
            fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].endTime = -1;
            fixedLinks[TOTAL_REPAIR_LINK_NUM_FIXED].isDispatched = false;
            TOTAL_REPAIR_LINK_NUM_FIXED++;
        }
    }
}

void reset_fluctuate_links(){
    TOTAL_REPAIR_LINK_NUM_FLUCTUATE = 0;
    for(int i = 0; i < STRIPE_NUM; ++i){
        for(int j = 0; j < fluctuateSolutions[i].fromRackSize; ++j){
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack = fluctuateSolutions[i].fromRack[j];
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack = fluctuateSolutions[i].toRack;
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].count = rackTransfer[fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack][fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack].count;
            printf("  fluctuateLink[%d]: fromRack=%d, toRack=%d\n", TOTAL_REPAIR_LINK_NUM_FLUCTUATE, fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack, fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack);
            printf("                    %d cost possibilities\n", fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].count);
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options = (CostProbPair *)calloc(fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].count, sizeof(CostProbPair));
            for(int k = 0; k < fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].count; ++k){
                fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options[k].cost = rackTransfer[fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack][fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack].options[k].cost;
                fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options[k].prob = rackTransfer[fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].fromRack][fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].toRack].options[k].prob;
                printf("                    option[%d]: cost %d (prob %f)\n", k, fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options[k].cost, fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].options[k].prob);
            }
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].startTime = -1;
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].endTime = -1;
            fluctuateLinks[TOTAL_REPAIR_LINK_NUM_FLUCTUATE].isDispatched = false;
            TOTAL_REPAIR_LINK_NUM_FLUCTUATE++;
        }
    }
}

int compute_AZ_solution_fixed_cost(int *currentHelpers, int *validRequesters, int validRequestersIndex, int *fromRackSize, int *fromRack){
    int toRack = nodes[validRequesters[validRequestersIndex]].rackId;
    int cost = 0;

    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        int is_duplicate = 0;
        if(blocks[currentHelpers[i]].rackId == toRack){
            is_duplicate = 1;
        }
        if(!is_duplicate){
            for(int j = 0; j < *fromRackSize; ++j){
                if(blocks[currentHelpers[i]].rackId == fromRack[j]){
                    is_duplicate = 1;
                    break;
                }
            }
        }
        if(!is_duplicate){
            fromRack[(*fromRackSize)++] = blocks[currentHelpers[i]].rackId;
        }
    }

    for(int i = 0; i < *fromRackSize; ++i){
        if(fromRack[i] != toRack){
            cost += FixedRackTransfer[fromRack[i]][toRack];
        }
    }

    return cost;
}

void generate_AZ_one_stripe_combinations_fixed(int stripe, int *validHelpers, int validHelpersNum, int *validRequesters, int validRequestersNum, int start, int *currentHelpers, int current_pos){
    if(current_pos == DATA_BLOCK_NUM_PER_STRIPE){
        for(int i = 0; i < validRequestersNum; i++){
            int fromRackSize = 0;
            int *fromRack = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
            for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; j++){
                fromRack[j] = blocks[currentHelpers[j]].rackId;
            }
            int cost = compute_AZ_solution_fixed_cost(currentHelpers, validRequesters, i, &fromRackSize, fromRack);

            if(cost < fixedSolutions[stripe].cost || fixedSolutions[stripe].cost == 0){                
                fixedSolutions[stripe].fromRackSize = fromRackSize;
                fixedSolutions[stripe].requester = validRequesters[i];
                fixedSolutions[stripe].toRack = nodes[fixedSolutions[stripe].requester].rackId;
                fixedSolutions[stripe].cost = cost;
                for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; j++){
                    fixedSolutions[stripe].helpers[j] = currentHelpers[j];
                }                
                for(int j = 0; j < fromRackSize; j++){
                    fixedSolutions[stripe].fromRack[j] = fromRack[j];
                }
            }
        }
        return;
    }
    
    for(int i = start; i <= validHelpersNum - DATA_BLOCK_NUM_PER_STRIPE + current_pos; i++){
        currentHelpers[current_pos] = validHelpers[i];
        generate_AZ_one_stripe_combinations_fixed(stripe, validHelpers, validHelpersNum, validRequesters, validRequestersNum, i + 1, currentHelpers, current_pos + 1);
    }
}

void generate_AZ_one_stripe_costprob_combinations(FluctuateSolution *NewSolution, int *indices, int currentIndex, int *optionIndex){
    if(currentIndex == (*NewSolution).fromRackSize){
        int totalCost = 0;
        float totalProb = 1.0f;
        for(int i = 0; i < (*NewSolution).fromRackSize; ++i){
            int idx = indices[i];
            totalCost += rackTransfer[(*NewSolution).fromRack[i]][(*NewSolution).toRack].options[idx].cost;
            totalProb *= rackTransfer[(*NewSolution).fromRack[i]][(*NewSolution).toRack].options[idx].prob;
        }
        (*NewSolution).options[*optionIndex].cost = totalCost;
        (*NewSolution).options[*optionIndex].prob = totalProb;
        if(totalProb >= 1.0f || totalProb <= 0.0f){
            printf("    WARNING! INVALID PROBABILITY %f\n", totalProb);
        }
        (*optionIndex)++;
        return;
    }
    
    for(int i = 0; i < rackTransfer[(*NewSolution).fromRack[currentIndex]][(*NewSolution).toRack].count; i++){
        indices[currentIndex] = i;
        generate_AZ_one_stripe_costprob_combinations(NewSolution, indices, currentIndex + 1, optionIndex);
    }
}

void merge_AZ_duplicate(FluctuateSolution *NewSolution){
    int maxCost = 0;
    for(int i = 0; i < (*NewSolution).count; ++i){
        if((*NewSolution).options[i].cost > maxCost){
            maxCost = (*NewSolution).options[i].cost;
        }
    }

    float *tempProbs = (float *)calloc(maxCost + 1, sizeof(float));
    check_allocation(tempProbs);

    for(int i = 0; i < (*NewSolution).count; ++i){
        int cost = (*NewSolution).options[i].cost;
        tempProbs[cost] += (*NewSolution).options[i].prob;
    }

    int newCount = 0;
    for(int cost = 0; cost <= maxCost; ++cost){
        if(tempProbs[cost] > 1e-9){
            newCount++;
        }
    }

    (*NewSolution).count = newCount;
    (*NewSolution).options = (CostProbPair *)realloc((*NewSolution).options, newCount * sizeof(CostProbPair));
    if((*NewSolution).options == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        free(tempProbs);
        return;
    }
    int index = 0;
    for(int cost = 0; cost <= maxCost; ++cost){
        if(tempProbs[cost] > 1e-9){
            (*NewSolution).options[index].cost = cost;
            (*NewSolution).options[index].prob = tempProbs[cost];
            index++;
        }
    }

    free(tempProbs);
}

void compute_AZ_one_stripe_solution(int *currentHelpers, int *validRequesters, int validRequestersIndex, int *fromRackSize, int *fromRack, FluctuateSolution *NewSolution){
    int toRack = nodes[validRequesters[validRequestersIndex]].rackId;

    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        int is_duplicate = 0;
        if(blocks[currentHelpers[i]].rackId == toRack){
            is_duplicate = 1;
        }
        if(!is_duplicate){
            for(int j = 0; j < *fromRackSize; ++j){
                if(blocks[currentHelpers[i]].rackId == fromRack[j]){
                    is_duplicate = 1;
                    break;
                }
            }
        }
        if(!is_duplicate){
            fromRack[(*fromRackSize)++] = blocks[currentHelpers[i]].rackId;
        }
    }

    (*NewSolution).fromRackSize = *fromRackSize;
    (*NewSolution).toRack = toRack;
    (*NewSolution).requester = validRequesters[validRequestersIndex];
    (*NewSolution).count = 1;
    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        (*NewSolution).helpers[i] = currentHelpers[i];
    }
    for(int i = 0; i < *fromRackSize; ++i){
        (*NewSolution).fromRack[i] = fromRack[i];
    }
    for(int i = 0; i < *fromRackSize; ++i){
        if(fromRack[i] != toRack){
            (*NewSolution).count *= rackTransfer[fromRack[i]][toRack].count;
        }
    }
    (*NewSolution).options = (CostProbPair *)realloc((*NewSolution).options, (*NewSolution).count * sizeof(CostProbPair));
    check_allocation((*NewSolution).options);
    for(int i = 0; i < (*NewSolution).count; ++i){
        (*NewSolution).options[i].cost = 0;
        (*NewSolution).options[i].prob = 1.0f;
    }
    
    int *indices = (int *)calloc(*fromRackSize, sizeof(int));
    check_allocation(indices);
    int optionIndex = 0;
    generate_AZ_one_stripe_costprob_combinations(NewSolution, indices, 0, &optionIndex);
    merge_AZ_duplicate(NewSolution);

    free(indices);
}

int compute_AZ_one_stripe_solution_maxExp_cost(FluctuateSolution *solution){
    float cost = 0.0f;
    if((*solution).count == 0 || (*solution).options == NULL){
        return 0;
    }
    for(int i = 0; i < (*solution).count; ++i){
        cost += (*solution).options[i].cost * (*solution).options[i].prob;
    }
    return cost;
}

void generate_AZ_one_stripe_combinations(int stripe, int *validHelpers, int validHelpersNum, int *validRequesters, int validRequestersNum, int start, int *currentHelpers, int current_pos){
    if(current_pos == DATA_BLOCK_NUM_PER_STRIPE){
        for(int i = 0; i < validRequestersNum; i++){
            int fromRackSize = 0;
            int *fromRack = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
            check_allocation(fromRack);
            for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; j++){
                fromRack[j] = blocks[currentHelpers[j]].rackId;
            }
            FluctuateSolution *NewSolution;
            NewSolution = (FluctuateSolution *)malloc(sizeof(FluctuateSolution));
            check_allocation(NewSolution);
            (*NewSolution).options = (CostProbPair *)calloc(1000, sizeof(CostProbPair));
            check_allocation((*NewSolution).options);
            (*NewSolution).helpers = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
            check_allocation((*NewSolution).helpers);
            (*NewSolution).fromRack = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
            check_allocation((*NewSolution).fromRack);
            for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
                (*NewSolution).helpers[j] = -1;
                (*NewSolution).fromRack[j] = -1;
            }
            compute_AZ_one_stripe_solution(currentHelpers, validRequesters, i, &fromRackSize, fromRack, NewSolution);

            float newCost = compute_AZ_one_stripe_solution_maxExp_cost(NewSolution);
            float cost;
            if(fluctuateSolutions[stripe].options == NULL){
                cost = 0;
            } else {
                cost = compute_AZ_one_stripe_solution_maxExp_cost(&fluctuateSolutions[stripe]);
            }

            if((newCost < cost && newCost > 0) || cost == 0){
                fluctuateSolutions[stripe].fromRackSize = fromRackSize;
                fluctuateSolutions[stripe].requester = validRequesters[i];
                fluctuateSolutions[stripe].toRack = nodes[fluctuateSolutions[stripe].requester].rackId;
                fluctuateSolutions[stripe].count = (*NewSolution).count;
                for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; j++){
                    fluctuateSolutions[stripe].helpers[j] = currentHelpers[j];
                }
                for(int j = 0; j < fromRackSize; j++){
                    fluctuateSolutions[stripe].fromRack[j] = fromRack[j];
                }
                fluctuateSolutions[stripe].options = (CostProbPair *)realloc(fluctuateSolutions[stripe].options, (*NewSolution).count * sizeof(CostProbPair));
                check_allocation(fluctuateSolutions[stripe].options);
                for(int j = 0; j < (*NewSolution).count; j++){
                    fluctuateSolutions[stripe].options[j] = (*NewSolution).options[j];
                }
            }

            free(fromRack);
            free((*NewSolution).helpers);
            free((*NewSolution).fromRack);
            free((*NewSolution).options);
            free(NewSolution);
        }
        return;
    }
    
    for(int i = start; i <= validHelpersNum - DATA_BLOCK_NUM_PER_STRIPE + current_pos; i++){
        currentHelpers[current_pos] = validHelpers[i];
        generate_AZ_one_stripe_combinations(stripe, validHelpers, validHelpersNum, validRequesters, validRequestersNum, i + 1, currentHelpers, current_pos + 1);
    }
}

void compute_TuTd_fixed(FixedSolution *solus){
    for(int i = 0; i < RACK_NUM; ++i){
        racks[i].Tu = 0;
        racks[i].Td = 0;
    }
    for(int i = 0; i < STRIPE_NUM; ++i){
        for(int j = 0; j < solus[i].fromRackSize; ++j){
            if(solus[i].fromRack[j] != solus[i].toRack){
                racks[solus[i].fromRack[j]].Tu += FixedRackTransfer[solus[i].fromRack[j]][solus[i].toRack];
                racks[solus[i].toRack].Td += FixedRackTransfer[solus[i].fromRack[j]][solus[i].toRack];
            }
        }
    }
}

int find_max_rack_time_fixed(){
    int MT = 0;
    for(int i = 0; i < RACK_NUM; ++i){
        if(racks[i].Tu > MT){
            MT = racks[i].Tu;
        }
        if(racks[i].Td > MT){
            MT = racks[i].Td;
        }
    }
    return MT;
}

void mark_max_rack_time_rack_fixed(){
    for(int i = 0; i < RACK_NUM; ++i){
        if(racks[i].Tu == MAX_RACK_TIME_FIXED){
            racks[i].isMTRack = true;
            racks[i].isTuMax = true;
            printf("Rack %d Tu is a bottleneck\n", i);
        }
        if(racks[i].Td == MAX_RACK_TIME_FIXED){
            racks[i].isMTRack = true;
            racks[i].isTdMax = true;
            printf("Rack %d Td is a bottleneck\n", i);
        }
    }
}

void dp_merge_link(CostProbPair **current_options, int *current_count, CostProbPair *link_options, int link_option_count) {
    if (*current_count == 0 || link_option_count == 0) return;

    int max_curr_cost = 0;
    for (int i = 0; i < *current_count; i++) {
        if ((*current_options)[i].cost > max_curr_cost) {
            max_curr_cost = (*current_options)[i].cost;
        }
    }

    int max_link_cost = 0;
    for (int i = 0; i < link_option_count; i++) {
        if (link_options[i].cost > max_link_cost) {
            max_link_cost = link_options[i].cost;
        }
    }

    int max_new_cost = max_curr_cost + max_link_cost;

    float *dp = (float*)calloc(max_new_cost + 1, sizeof(float));
    check_allocation(dp);

    for (int i = 0; i < *current_count; i++) {
        for (int j = 0; j < link_option_count; j++) {
            int new_cost = (*current_options)[i].cost + link_options[j].cost;
            float new_prob = (*current_options)[i].prob * link_options[j].prob;
            dp[new_cost] += new_prob;
        }
    }

    int new_count = 0;
    for (int c = 0; c <= max_new_cost; c++) {
        if (dp[c] > 1e-9) {
            new_count++;
        }
    }

    free(*current_options); 
    *current_options = (CostProbPair*)malloc(new_count * sizeof(CostProbPair));
    check_allocation(*current_options);

    int idx = 0;
    for (int c = 0; c <= max_new_cost; c++) {
        if (dp[c] > 1e-9) {
            (*current_options)[idx].cost = c;
            (*current_options)[idx].prob = dp[c];
            idx++;
        }
    }

    *current_count = new_count;
    free(dp);
}

void compute_TuTd_fluctuate(FluctuateSolution *solus) {
    for(int i = 0; i < RACK_NUM; ++i){
        racks[i].TuCount = 0;
        racks[i].TdCount = 0;
        racks[i].TuOptions = NULL;
        racks[i].TdOptions = NULL; 
    }

    reset_fluctuate_links_without_print(solus);

    for (int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i) {
        int fromRack = fluctuateLinks[i].fromRack;
        int toRack = fluctuateLinks[i].toRack;

        if (fromRack != toRack && fluctuateLinks[i].count > 0) {
            
            if (fluctuateLinks[i].count == 1 && 
                fluctuateLinks[i].options[0].cost == 0 && 
                fluctuateLinks[i].options[0].prob >= 0.999f) {
                continue; 
            }

            if (racks[fromRack].TuCount == 0) {
                racks[fromRack].TuCount = fluctuateLinks[i].count;
                racks[fromRack].TuOptions = (CostProbPair *)malloc(sizeof(CostProbPair) * fluctuateLinks[i].count);
                check_allocation(racks[fromRack].TuOptions);
                memcpy(racks[fromRack].TuOptions, fluctuateLinks[i].options, sizeof(CostProbPair) * fluctuateLinks[i].count);
            } else {
                dp_merge_link(&(racks[fromRack].TuOptions), &(racks[fromRack].TuCount), 
                              fluctuateLinks[i].options, fluctuateLinks[i].count);
            }

            if (racks[toRack].TdCount == 0) {
                racks[toRack].TdCount = fluctuateLinks[i].count;
                racks[toRack].TdOptions = (CostProbPair *)malloc(sizeof(CostProbPair) * fluctuateLinks[i].count);
                check_allocation(racks[toRack].TdOptions);
                memcpy(racks[toRack].TdOptions, fluctuateLinks[i].options, sizeof(CostProbPair) * fluctuateLinks[i].count);
            } else {
                dp_merge_link(&(racks[toRack].TdOptions), &(racks[toRack].TdCount), 
                              fluctuateLinks[i].options, fluctuateLinks[i].count);
            }
        }
    }
}

float find_max_rack_time_fluctuate(){
    float MT = 0;

    for(int i = 0; i < RACK_NUM; ++i){
        racks[i].expTu = 0.0f;
        racks[i].expTd = 0.0f;
        for(int j = 0; j < racks[i].TuCount; ++j){
            racks[i].expTu += racks[i].TuOptions[j].cost * racks[i].TuOptions[j].prob;
        }
        for(int j = 0; j < racks[i].TdCount; ++j){
            racks[i].expTd += racks[i].TdOptions[j].cost * racks[i].TdOptions[j].prob;
        }
        if(racks[i].expTu > MT){
            MT = racks[i].expTu;

        }
        if(racks[i].expTd > MT){
            MT = racks[i].expTd;
        }
    }
    return MT;
}

void mark_max_rack_time_rack_fluctuate(){
    for(int i = 0; i < RACK_NUM; ++i){
        if(racks[i].expTu == MAX_RACK_TIME_FLUCTUATE){
            racks[i].isMTRack = true;
            racks[i].isTuMax = true;
            printf("Rack %d Tu is a bottleneck\n", i);
        }
        if(racks[i].expTd == MAX_RACK_TIME_FLUCTUATE){
            racks[i].isMTRack = true;
            racks[i].isTdMax = true;
            printf("Rack %d Td is a bottleneck\n", i);
        }
    }
}

void clear_max_rack_time_rack_marks(){
    for(int i = 0; i < RACK_NUM; ++i){
        racks[i].isMTRack = false;
        racks[i].isTuMax = false;
        racks[i].isTdMax = false;
    }
}

void copy_fixed_solution(FixedSolution *solusFrom, FixedSolution *solusTo, int stripe){
    solusTo[stripe].toRack = solusFrom[stripe].toRack;
    solusTo[stripe].requester = solusFrom[stripe].requester;
    solusTo[stripe].fromRackSize = solusFrom[stripe].fromRackSize;
    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        solusTo[stripe].helpers[i] = solusFrom[stripe].helpers[i];
    }
    for(int i = 0; i < solusTo[stripe].fromRackSize; ++i){
        solusTo[stripe].fromRack[i] = solusFrom[stripe].fromRack[i];
    }
    solusTo[stripe].cost = solusFrom[stripe].cost;
}

void copy_fluctuate_solution(FluctuateSolution *solusFrom, FluctuateSolution *solusTo, int stripe){
    solusTo[stripe].toRack = solusFrom[stripe].toRack;
    solusTo[stripe].requester = solusFrom[stripe].requester;
    solusTo[stripe].fromRackSize = solusFrom[stripe].fromRackSize;
    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        solusTo[stripe].helpers[i] = solusFrom[stripe].helpers[i];
    }
    for(int i = 0; i < solusTo[stripe].fromRackSize; ++i){
        solusTo[stripe].fromRack[i] = solusFrom[stripe].fromRack[i];
    }
    solusTo[stripe].count = solusFrom[stripe].count;
    solusTo[stripe].options = (CostProbPair *)realloc(solusTo[stripe].options, solusTo[stripe].count * sizeof(CostProbPair));
    for(int i = 0; i < solusTo[stripe].count; ++i){
        solusTo[stripe].options[i].cost = solusFrom[stripe].options[i].cost;
        solusTo[stripe].options[i].prob = solusFrom[stripe].options[i].prob;
    }
}

void compute_CTP_solution_fixed_cost(FixedSolution *solus, int stripe){
    solus[stripe].toRack = nodes[solus[stripe].requester].rackId;
    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        solus[stripe].fromRack[i] = blocks[solus[stripe].helpers[i]].rackId;
    }
    solus[stripe].fromRackSize = 0;
    solus[stripe].cost = 0;

    for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
        int is_duplicate = 0;
        if(solus[stripe].fromRack[i] == solus[stripe].toRack){
            is_duplicate = 1;
        }
        if(!is_duplicate){
            for(int j = 0; j < solus[stripe].fromRackSize; ++j){
                if(solus[stripe].fromRack[i] == solus[stripe].fromRack[j]){
                    is_duplicate = 1;
                    break;
                }
            }
        }
        if(!is_duplicate){
            solus[stripe].fromRack[solus[stripe].fromRackSize++] = solus[stripe].fromRack[i];
        }
    }

    for(int i = 0; i < solus[stripe].fromRackSize; ++i){
        if(solus[stripe].fromRack[i] != solus[stripe].toRack){
            solus[stripe].cost += FixedRackTransfer[solus[stripe].fromRack[i]][solus[stripe].toRack];
        }
    }
}

void generate_substitute_helpers_fixed(int stripe, int *validHelpers, int validHelpersNum, int start, int *currentHelpers, int current_pos, bool *optimized){
    if(current_pos == DATA_BLOCK_NUM_PER_STRIPE){
        for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
            substituteFixedSolus[stripe].helpers[i] = currentHelpers[i];
            printf("  Trying helper: Block %d (rack %d)\n", currentHelpers[i], blocks[currentHelpers[i]].rackId);
        }
        compute_CTP_solution_fixed_cost(substituteFixedSolus, stripe);
        compute_TuTd_fixed(substituteFixedSolus);
        int MT = find_max_rack_time_fixed();
        printf("  Substitute MT=%d\n", MT);

        if(MT < MAX_RACK_TIME_FIXED){
            MAX_RACK_TIME_FIXED = MT;
            clear_max_rack_time_rack_marks();
            mark_max_rack_time_rack_fixed();
            copy_fixed_solution(substituteFixedSolus, fixedSolutions, stripe);
            *optimized = 1;
        } else {
            printf("substitute MT %d not better than current MT %d\n", MT, MAX_RACK_TIME_FIXED);
            for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
                substituteFixedSolus[stripe].helpers[i] = fixedSolutions[stripe].helpers[i];
            }
        }
        return;
    }
    
    for(int i = start; i <= validHelpersNum - DATA_BLOCK_NUM_PER_STRIPE + current_pos; i++){
        currentHelpers[current_pos] = validHelpers[i];
        generate_substitute_helpers_fixed(stripe, validHelpers, validHelpersNum, i + 1, currentHelpers, current_pos + 1, optimized);
    }
}

void generate_substitute_helpers_fluctuate(int stripe, int *validHelpers, int validHelpersNum, int start, int *currentHelpers, int current_pos, bool *optimized){
    if(current_pos == DATA_BLOCK_NUM_PER_STRIPE){
        for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
            substituteFluctuateSolus[stripe].helpers[i] = currentHelpers[i];
            printf("  Trying helper: Block %d (rack %d)\n", currentHelpers[i], blocks[currentHelpers[i]].rackId);
        }
        solution_repair_options(substituteFluctuateSolus, stripe);
        compute_TuTd_fluctuate(substituteFluctuateSolus);
        float MT = find_max_rack_time_fluctuate();
        printf("  Substitute MT=%f\n", MT);

        if(MT < MAX_RACK_TIME_FLUCTUATE){
            MAX_RACK_TIME_FLUCTUATE = MT;
            clear_max_rack_time_rack_marks();
            mark_max_rack_time_rack_fluctuate();
            copy_fluctuate_solution(substituteFluctuateSolus, fluctuateSolutions, stripe);
            *optimized = 1;
        } else {
            printf("substitute MT %f not better than current MT %f\n", MT, MAX_RACK_TIME_FLUCTUATE);
            for(int i = 0; i < DATA_BLOCK_NUM_PER_STRIPE; ++i){
                substituteFluctuateSolus[stripe].helpers[i] = fluctuateSolutions[stripe].helpers[i];
            }
        }
        return;
    }
    
    for(int i = start; i <= validHelpersNum - DATA_BLOCK_NUM_PER_STRIPE + current_pos; i++){
        currentHelpers[current_pos] = validHelpers[i];
        generate_substitute_helpers_fluctuate(stripe, validHelpers, validHelpersNum, i + 1, currentHelpers, current_pos + 1, optimized);
    }
}

void find_new_helpers(int stripe, int MTRack, bool *optimized, bool isFixed){
    int validHelpersNum = 0;
    int *validHelpers = (int *)calloc(BLOCK_PER_STRIPE - 1, sizeof(int));

    for(int i = 0; i < BLOCK_PER_STRIPE; ++i){
        if(!blocks[stripe * BLOCK_PER_STRIPE + i].isLost && nodes[blocks[stripe * BLOCK_PER_STRIPE + i].nodeId].isSet[stripe] && blocks[stripe * BLOCK_PER_STRIPE + i].rackId != MTRack){
            validHelpers[validHelpersNum++] = stripe * BLOCK_PER_STRIPE + i;
        }
    }

    if(validHelpersNum < DATA_BLOCK_NUM_PER_STRIPE){
        printf("Not enough substitute valid helpers\n");
        free(validHelpers);
        return;
    }

    int *currentHelpers = (int *)calloc(DATA_BLOCK_NUM_PER_STRIPE, sizeof(int));
    if(isFixed){
        generate_substitute_helpers_fixed(stripe, validHelpers, validHelpersNum, 0, currentHelpers, 0, optimized);
    } else{
        generate_substitute_helpers_fluctuate(stripe, validHelpers, validHelpersNum, 0, currentHelpers, 0, optimized);
    }
    if(!optimized){
        printf("No substitute solutions found\n");
    }

    free(validHelpers);
    free(currentHelpers);
}

void find_new_requester_fixed(int stripe, int MTRack, bool *optimized){
    int validRequestersNum = 0;
    int *validRequesters = (int *)calloc(NODE_NUM, sizeof(int));

    for(int i = 0; i < NODE_NUM; ++i){
        if(racks[nodes[i].rackId].stripeBlockCount[stripe] < PARITY_BLOCK_NUM_PER_STRIPE && !nodes[i].isSet[stripe] && nodes[i].rackId != MTRack){
            validRequesters[validRequestersNum++] = i;
        }
    }

    if(validRequestersNum == 0){
        printf("Not enough substitute valid requesters\n");
        free(validRequesters);
        return;
    } else {
        validRequesters = (int *)realloc(validRequesters, validRequestersNum * sizeof(int));
    }

    for(int i = 0; i < validRequestersNum; ++i){
        substituteFixedSolus[stripe].requester = validRequesters[i];
        substituteFixedSolus[stripe].toRack = nodes[validRequesters[i]].rackId;
        printf("  Trying requester: Node %d (rack %d)\n", validRequesters[i], substituteFixedSolus[stripe].toRack);
        compute_CTP_solution_fixed_cost(substituteFixedSolus, stripe);
        compute_TuTd_fixed(substituteFixedSolus);
        int MT = find_max_rack_time_fixed();
        printf("  Substitute MT=%d\n", MT);

        if(MT < MAX_RACK_TIME_FIXED){
            MAX_RACK_TIME_FIXED = MT;
            clear_max_rack_time_rack_marks();
            mark_max_rack_time_rack_fixed();
            copy_fixed_solution(substituteFixedSolus, fixedSolutions, stripe);
            *optimized = 1;
        } else {
            printf("substitute MT %d not better than current MT %d\n", MT, MAX_RACK_TIME_FIXED);
            substituteFixedSolus[stripe].requester = fixedSolutions[stripe].requester;
            substituteFixedSolus[stripe].toRack = fixedSolutions[stripe].toRack;
        }
    }

    if(!*optimized){
        printf("No substitute solutions found\n");
    }

    free(validRequesters);
}

void find_new_requester_fluctuate(int stripe, int MTRack, bool *optimized){
    int validRequestersNum = 0;
    int *validRequesters = (int *)calloc(NODE_NUM, sizeof(int));

    for(int i = 0; i < NODE_NUM; ++i){
        if(racks[nodes[i].rackId].stripeBlockCount[stripe] < PARITY_BLOCK_NUM_PER_STRIPE && !nodes[i].isSet[stripe] && nodes[i].rackId != MTRack){
            validRequesters[validRequestersNum++] = i;
        }
    }

    if(validRequestersNum == 0){
        printf("Not enough substitute valid requesters\n");
        free(validRequesters);
        return;
    } else {
        validRequesters = (int *)realloc(validRequesters, validRequestersNum * sizeof(int));
    }

    for(int i = 0; i < validRequestersNum; ++i){
        substituteFluctuateSolus[stripe].requester = validRequesters[i];
        substituteFluctuateSolus[stripe].toRack = nodes[validRequesters[i]].rackId;
        printf("  Trying requester: Node %d (rack %d)\n", validRequesters[i], substituteFluctuateSolus[stripe].toRack);
        solution_repair_options(substituteFluctuateSolus, stripe);
        compute_TuTd_fluctuate(substituteFluctuateSolus);
        float MT = find_max_rack_time_fluctuate();
        printf("  Substitute MT=%f\n", MT);

        if(MT < MAX_RACK_TIME_FLUCTUATE){
            MAX_RACK_TIME_FLUCTUATE = MT;
            clear_max_rack_time_rack_marks();
            mark_max_rack_time_rack_fluctuate();
            copy_fluctuate_solution(substituteFluctuateSolus, fluctuateSolutions, stripe);
            *optimized = 1;
        } else {
            printf("substitute MT %f not better than current MT %f\n", MT, MAX_RACK_TIME_FLUCTUATE);
            substituteFluctuateSolus[stripe].requester = fluctuateSolutions[stripe].requester;
            substituteFluctuateSolus[stripe].toRack = fluctuateSolutions[stripe].toRack;
        }
    }

    if(!*optimized){
        printf("No substitute solutions found\n");
    }

    free(validRequesters);
}
