#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "Compare.h"
#include "Compare_Func.c"

int DATA_BLOCK_NUM_PER_STRIPE = 0;
int PARITY_BLOCK_NUM_PER_STRIPE = 0;
int BLOCK_PER_STRIPE = 0;
int STRIPE_NUM = 0;
int NODE_NUM_PER_RACK = 0;
int RACK_NUM = 0;
int NODE_NUM = 0;
int BLOCK_NUM = 0;
int TOTAL_REPAIR_LINK_NUM_FIXED = 0;
int TOTAL_REPAIR_LINK_NUM_FLUCTUATE = 0;
int MAX_RACK_TIME_FIXED = 0;
float MAX_RACK_TIME_FLUCTUATE = 0.0f;
int **FixedRackTransfer = NULL;

int main(){
    printf("\n\x1b[32m------------------------------- READ CONFIG -------------------------------\x1b[0m\n");

    read_config("input/Compare_config.txt");

    printf("\n\x1b[32m------------------------ INITIALIZE REPAIR SOLUTION ------------------------\x1b[0m\n");

    srand((unsigned)time(NULL));
    struct timeval start, end;
    gettimeofday(&start, NULL);

    racks = (Rack *)malloc(RACK_NUM * sizeof(Rack));
    for(int i = 0; i < RACK_NUM; ++i){
        racks[i].stripeBlockCount = (int *)malloc(STRIPE_NUM * sizeof(int));
        for(int j = 0; j < STRIPE_NUM; ++j){
            racks[i].stripeBlockCount[j] = 0;
        }
        racks[i].Tu = 0;
        racks[i].Td = 0;
        racks[i].expTu = 0.0f;
        racks[i].expTd = 0.0f;
        racks[i].TuCount = 0;
        racks[i].TdCount = 0;
        racks[i].TuOptions = NULL;
        racks[i].TdOptions = NULL;
        racks[i].isMTRack = false;
        racks[i].isTuMax = false;
        racks[i].isTdMax = false;
    }

    nodes = (Node *)malloc(NODE_NUM * sizeof(Node));
    for(int i = 0; i < NODE_NUM; ++i){
        nodes[i].rackId = i / NODE_NUM_PER_RACK;
        nodes[i].isSet = (bool *)malloc(STRIPE_NUM * sizeof(bool));
        for(int j = 0; j < STRIPE_NUM; ++j){
            nodes[i].isSet[j] = false;
        }
    }

    blocks = (Block *)malloc(BLOCK_NUM * sizeof(Block));
    for(int i = 0; i < BLOCK_NUM; ++i){
        blocks[i].sourceStripe = i / BLOCK_PER_STRIPE;
        blocks[i].rackId = -1;
        blocks[i].nodeId = -1;
        blocks[i].isLost = false;
        blocks[i].previousNodeId = -1;
    }

    rackTransfer = (RackTransfer **)malloc(RACK_NUM * sizeof(RackTransfer *));
    for(int i = 0; i < RACK_NUM; ++i){
        rackTransfer[i] = (RackTransfer *)malloc(RACK_NUM * sizeof(RackTransfer));
        for(int j = 0; j < RACK_NUM; ++j){
            rackTransfer[i][j].count = 0;
            rackTransfer[i][j].options = NULL;
        }
    }

    FixedRackTransfer = (int **)malloc(RACK_NUM * sizeof(int *));
    for(int i = 0; i < RACK_NUM; ++i){
        FixedRackTransfer[i] = (int *)malloc(RACK_NUM * sizeof(int));
        for(int j = 0; j < RACK_NUM; ++j){
            FixedRackTransfer[i][j] = -1;
        }
    }

    read_fixed_rack_transfer("input/Fixed_RackTransfer-20.txt");
    printf("\x1b[34mfixed transmission cost between racks\x1b[0m\n");
    for(int i = 0; i < RACK_NUM; ++i){
        for(int j = 0; j < RACK_NUM; ++j){
            if(i != j){
                printf("R%d-R%d: %d\n", i, j, FixedRackTransfer[i][j]);
            }
        }
    }

    read_fluctuate_rack_transfer("input/Fluctuate_RackTransfer-20.txt");
    printf("\n\x1b[34mfluctuate transmission cost and prob between racks\x1b[0m\n");
    for(int i = 0; i < RACK_NUM; ++i){
        for(int j = 0; j < RACK_NUM; ++j){
            if(i != j){
                printf("R%d-R%d: %d options\n", i, j, rackTransfer[i][j].count);
                for(int k = 0; k < rackTransfer[i][j].count; ++k){
                    printf("  cost %d (prob %f)\n", rackTransfer[i][j].options[k].cost, rackTransfer[i][j].options[k].prob);
                }
            }
        }
    }

    fixedSolutions = (FixedSolution *)malloc(STRIPE_NUM * sizeof(FixedSolution));
    substituteFixedSolus = (FixedSolution *)malloc(STRIPE_NUM * sizeof(FixedSolution));
    for(int i = 0; i < STRIPE_NUM; ++i){
        fixedSolutions[i].requester = -1;
        fixedSolutions[i].fromRackSize = 0;
        fixedSolutions[i].toRack = -1;
        fixedSolutions[i].cost = 0;
        substituteFixedSolus[i].requester = -1;
        substituteFixedSolus[i].fromRackSize = 0;
        substituteFixedSolus[i].toRack = -1;
        substituteFixedSolus[i].cost = 0;
        fixedSolutions[i].helpers = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        fixedSolutions[i].fromRack = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        substituteFixedSolus[i].helpers = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        substituteFixedSolus[i].fromRack = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            fixedSolutions[i].helpers[j] = -1;
            fixedSolutions[i].fromRack[j] = -1;
            substituteFixedSolus[i].helpers[j] = -1;
            substituteFixedSolus[i].fromRack[j] = -1;
        }
    }

    fluctuateSolutions = (FluctuateSolution *)malloc(STRIPE_NUM * sizeof(FluctuateSolution));
    substituteFluctuateSolus = (FluctuateSolution *)malloc(STRIPE_NUM * sizeof(FluctuateSolution));
    for(int i = 0; i < STRIPE_NUM; ++i){
        fluctuateSolutions[i].requester = -1;
        fluctuateSolutions[i].fromRackSize = 0;
        fluctuateSolutions[i].toRack = -1;
        fluctuateSolutions[i].count = 1;
        fluctuateSolutions[i].options = NULL;
        substituteFluctuateSolus[i].requester = -1;
        substituteFluctuateSolus[i].fromRackSize = 0;
        substituteFluctuateSolus[i].toRack = -1;
        substituteFluctuateSolus[i].count = 1;
        substituteFluctuateSolus[i].options = NULL;
        fluctuateSolutions[i].helpers = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        fluctuateSolutions[i].fromRack = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        substituteFluctuateSolus[i].helpers = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        substituteFluctuateSolus[i].fromRack = (int *)malloc(DATA_BLOCK_NUM_PER_STRIPE * sizeof(int));
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            fluctuateSolutions[i].helpers[j] = -1;
            fluctuateSolutions[i].fromRack[j] = -1;
            substituteFluctuateSolus[i].helpers[j] = -1;
            substituteFluctuateSolus[i].fromRack[j] = -1;
        }
    }
    
    fixedLinks = (Link *)malloc((STRIPE_NUM * DATA_BLOCK_NUM_PER_STRIPE) * sizeof(Link));
    fluctuateLinks = (Link *)malloc((STRIPE_NUM * DATA_BLOCK_NUM_PER_STRIPE) * sizeof(Link));
    for(int i = 0; i < (STRIPE_NUM * DATA_BLOCK_NUM_PER_STRIPE); ++i){
        fixedLinks[i].fromRack = -1;
        fixedLinks[i].toRack = -1;
        fixedLinks[i].cost = 0;
        fixedLinks[i].startTime = -1;
        fixedLinks[i].endTime = -1;
        fixedLinks[i].isDispatched = false;
        fixedLinks[i].count = 0;
        fixedLinks[i].options = NULL;
        fluctuateLinks[i].fromRack = -1;
        fluctuateLinks[i].toRack = -1;
        fluctuateLinks[i].cost = 0;
        fluctuateLinks[i].startTime = -1;
        fluctuateLinks[i].endTime = -1;
        fluctuateLinks[i].isDispatched = false;
        fluctuateLinks[i].count = 0;
        fluctuateLinks[i].options = NULL;
    }

    int nodeRandom;
    for(int i = 0; i < BLOCK_NUM; ++i){
        do {
            nodeRandom = rand() % NODE_NUM;
        } while(nodes[nodeRandom].isSet[blocks[i].sourceStripe] || racks[nodes[nodeRandom].rackId].stripeBlockCount[blocks[i].sourceStripe] >= PARITY_BLOCK_NUM_PER_STRIPE); 
        blocks[i].rackId = nodes[nodeRandom].rackId;
        blocks[i].nodeId = nodeRandom;
        nodes[nodeRandom].isSet[blocks[i].sourceStripe] = true;
        racks[nodes[nodeRandom].rackId].stripeBlockCount[blocks[i].sourceStripe]++;
    }

    printf("\n\x1b[34minitial stripes\x1b[0m\n");
    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("S%d: [", i);
        for(int j = 0; j < BLOCK_PER_STRIPE; ++j){
            printf(" %d", blocks[i * BLOCK_PER_STRIPE + j].nodeId);
        }
        printf(" ]\n");
    }

    printf("\n\x1b[34minitial racks and nodes\x1b[0m\n");
    int nodeIndex = 0;
    for(int i = 0; i < RACK_NUM; ++i){
        printf("Rack %d: \n", i);
        for(int j = 0; j < NODE_NUM_PER_RACK; ++j){
            printf(" Node %d: \n", nodeIndex);
            for(int k = 0; k < BLOCK_NUM; ++k){
                if(blocks[k].nodeId == nodeIndex){
                    printf("  Block %d: S%d.B%d\n", k, blocks[k].sourceStripe, k % BLOCK_PER_STRIPE);
                }
            }
            nodeIndex++;
        }
    }

    for(int i = 0; i < STRIPE_NUM; ++i){
        int blockToLose = rand() % BLOCK_PER_STRIPE;
        blocks[blockToLose + i * BLOCK_PER_STRIPE].isLost = true;
        nodes[blocks[blockToLose + i * BLOCK_PER_STRIPE].nodeId].isSet[blocks[blockToLose + i * BLOCK_PER_STRIPE].sourceStripe] = false;
        racks[blocks[blockToLose + i * BLOCK_PER_STRIPE].rackId].stripeBlockCount[blocks[blockToLose + i * BLOCK_PER_STRIPE].sourceStripe]--;
        blocks[blockToLose + i * BLOCK_PER_STRIPE].previousNodeId = blocks[blockToLose + i * BLOCK_PER_STRIPE].nodeId;
        blocks[blockToLose + i * BLOCK_PER_STRIPE].rackId = -1;
        blocks[blockToLose + i * BLOCK_PER_STRIPE].nodeId = -1;
    }

    printf("\n\x1b[34mdata loss stripes\x1b[0m\n");
    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("S%d: [", i);
        for(int j = 0; j < BLOCK_PER_STRIPE; ++j){
            printf(" %d", blocks[i * BLOCK_PER_STRIPE + j].nodeId);
        }
        printf(" ]\n");
    }

    printf("\n\x1b[34mracks and nodes after data loss\x1b[0m\n");
    nodeIndex = 0;
    for(int i = 0; i < RACK_NUM; ++i){
        printf("Rack %d: \n", i);
        for(int j = 0; j < NODE_NUM_PER_RACK; ++j){
            printf(" Node %d: \n", nodeIndex);
            for(int k = 0; k < BLOCK_NUM; ++k){
                if(blocks[k].nodeId == nodeIndex){
                    printf("  Block %d: S%d.B%d\n", k, blocks[k].sourceStripe, k % BLOCK_PER_STRIPE);
                } else if(blocks[k].isLost && blocks[k].previousNodeId == nodeIndex){
                    printf("  Block %d: LOST\n", k);
                }
            }
            nodeIndex++;
        }
    }

    printf("\n\x1b[32m------------------------------ RANDOM SCHEME -------------------------------\x1b[0m\n");

    printf("\x1b[34mrandom choose helpers and requester\x1b[0m\n");

    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("Stripe %d: \n", i);
        int lostBlock = -1;
        for(int j = 0; j < BLOCK_PER_STRIPE; ++j){
            if(blocks[i * BLOCK_PER_STRIPE + j].isLost){
                lostBlock = i * BLOCK_PER_STRIPE + j;
                break;
            }
        }

        int *validHelpers = (int *)malloc((BLOCK_PER_STRIPE - 1) * sizeof(int));
        int validHelperCount = 0;
        for(int j = 0; j < BLOCK_PER_STRIPE; ++j){
            if(!blocks[i * BLOCK_PER_STRIPE + j].isLost){
                validHelpers[validHelperCount] = i * BLOCK_PER_STRIPE + j;
                printf("  validHelpers[%d] = %d\n", validHelperCount, validHelpers[validHelperCount]);
                validHelperCount++;
            }
        }
        random_choose_helpers(validHelpers, validHelperCount, i);
        printf("  helpers: blocks = [");
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            printf(" %d", fixedSolutions[i].helpers[j]);
        }
        printf(" ]\n");

        fixedSolutions[i].requester = blocks[lostBlock].previousNodeId;
        fluctuateSolutions[i].requester = blocks[lostBlock].previousNodeId;
        printf("  requester: Node = %d\n", fixedSolutions[i].requester);

        solution_repair_cost(fixedSolutions, i);
        printf("  \x1b[30mFIXED:\x1b[0m\n");
        printf("    fixedSolutions[%d].cost = %d\n", i, fixedSolutions[i].cost);

        solution_repair_options(fluctuateSolutions, i);
        printf("  \x1b[30mFLUCTUATE:\x1b[0m\n");
        printf("  fluctuateSolutions[%d] has %d possibilities\n", i, fluctuateSolutions[i].count);
        for(int j = 0; j < fluctuateSolutions[i].count; ++j){
            printf("    options[%d]: cost %d (prob %f)\n", j, fluctuateSolutions[i].options[j].cost, fluctuateSolutions[i].options[j].prob);
        }
        
        free(validHelpers);
    }

    printf("\n\x1b[34mcompute MT\x1b[0m\n");
    printf("  \x1b[30mFIXED:\x1b[0m\n");
    compute_TuTd_fixed(fixedSolutions);
    MAX_RACK_TIME_FIXED = find_max_rack_time_fixed();
    printf("  fixed random MT = %d\n", MAX_RACK_TIME_FIXED);
    printf("  \x1b[30mFLUCTUATE:\x1b[0m\n");
    compute_TuTd_fluctuate(fluctuateSolutions);
    MAX_RACK_TIME_FLUCTUATE = find_max_rack_time_fluctuate();
    printf("  fluctuate random MT = %f\n", MAX_RACK_TIME_FLUCTUATE);
    // print_MT_to_file("output/MT_Results.txt", "Random-FIXED", MAX_RACK_TIME_FIXED, "Random-FLUCTUATE", MAX_RACK_TIME_FLUCTUATE);
    
    reset_fluctuate_links_without_print(fluctuateSolutions);
    TOTAL_REPAIR_LINK_NUM_FIXED = TOTAL_REPAIR_LINK_NUM_FLUCTUATE;

    printf("\n\x1b[34mrandomly shuffle the repair links\x1b[0m\n");
    shuffle(fluctuateLinks, TOTAL_REPAIR_LINK_NUM_FLUCTUATE);
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i){
        fixedLinks[i].fromRack = fluctuateLinks[i].fromRack;
        fixedLinks[i].toRack = fluctuateLinks[i].toRack;
        fixedLinks[i].cost = FixedRackTransfer[fixedLinks[i].fromRack][fixedLinks[i].toRack];
        printf("  Link %d: fromRack=%d, toRack=%d\n", i, fluctuateLinks[i].fromRack, fluctuateLinks[i].toRack);
        printf("          \x1b[30mFIXED:\x1b[0m\n          cost: %d\n", fixedLinks[i].cost);
        printf("          \x1b[30mFLUCTUATE:\x1b[0m\n          %d cost possibilities\n", fluctuateLinks[i].count);
        for(int j = 0; j < fluctuateLinks[i].count; ++j){
            printf("          option[%d]: cost %d (prob %f)\n", j, fluctuateLinks[i].options[j].cost, fluctuateLinks[i].options[j].prob);
        }
    }
    printf("  total repair links: %d\n", TOTAL_REPAIR_LINK_NUM_FLUCTUATE);

    printf("\n\x1b[34mschedule repair links\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    schedule_fixed_repair_links();
    int RandomTotalRepairCost = 0;
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
        if(fixedLinks[i].endTime > RandomTotalRepairCost){
            RandomTotalRepairCost = fixedLinks[i].endTime;
        }
    }
    printf("  random total repair cost: %d\n", RandomTotalRepairCost);

    printf("\n\x1b[34manalyze all possible scheduling results\x1b[0m\n");
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    int RandomResultCount = 0;
    Result* RandomResults = analyze_all_possibilities(fluctuateLinks, &RandomResultCount);
    sort_results_by_time(RandomResults, RandomResultCount);
    if(RandomResultCount == 0){
        printf("\nNo valid results were found. There might be scheduling conflicts or logical errors.\n");
    } else {
        printf("All possible total repair times and probabilities:\n");
        print_prob(RandomResults, RandomResultCount);
    }

    printf("\n\x1b[32m-------------------------------- AZ SCHEME ---------------------------------\x1b[0m\n");

    for(int i = 0; i < STRIPE_NUM; ++i){
        fixedSolutions[i].requester = -1;
        fixedSolutions[i].fromRackSize = 0;
        fixedSolutions[i].toRack = -1;
        fixedSolutions[i].cost = 0;
        fluctuateSolutions[i].requester = -1;
        fluctuateSolutions[i].fromRackSize = 0;
        fluctuateSolutions[i].toRack = -1;
        fluctuateSolutions[i].count = 0;
        if(fluctuateSolutions[i].options != NULL){
            free(fluctuateSolutions[i].options);
            fluctuateSolutions[i].options = NULL;
        }
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            fixedSolutions[i].helpers[j] = -1;
            fixedSolutions[i].fromRack[j] = -1;
            fluctuateSolutions[i].helpers[j] = -1;
            fluctuateSolutions[i].fromRack[j] = -1;
        }
    }

    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("\x1b[34mAZ choose helpers and requester for stripe %d\x1b[0m\n", i);

        int validHelpersNum = 0;
        int validRequestersNum = 0;
        int *validHelpers = (int *)calloc((BLOCK_PER_STRIPE - 1), sizeof(int));
        int *validRequesters = (int *)calloc((NODE_NUM - BLOCK_PER_STRIPE + 1), sizeof(int));

        for(int j = 0; j < BLOCK_PER_STRIPE; ++j){
            if(!blocks[i * BLOCK_PER_STRIPE + j].isLost && nodes[blocks[i * BLOCK_PER_STRIPE + j].nodeId].isSet[i]){
                validHelpers[validHelpersNum] = i * BLOCK_PER_STRIPE + j;
                validHelpersNum++;
            }
        }
        
        for(int j = 0; j < NODE_NUM; ++j){
            if(racks[nodes[j].rackId].stripeBlockCount[i] < PARITY_BLOCK_NUM_PER_STRIPE && !nodes[j].isSet[i]){
                validRequesters[validRequestersNum] = j;
                validRequestersNum++;
            }
        }

        printf("  validHelpers: ");
        for(int j = 0; j < validHelpersNum; ++j){
            printf("Block[%d] ", validHelpers[j]);
        }
        printf("\n");
        printf("  validRequesters: ");
        for(int j = 0; j < validRequestersNum; ++j){
            printf("Node[%d] ", validRequesters[j]);
        }
        printf("\n");

        printf("\x1b[34mAZ solution for stripe %d\x1b[0m\n", i);
        int *currentHelpers = (int *)calloc(DATA_BLOCK_NUM_PER_STRIPE, sizeof(int));
        printf("  \x1b[30mFIXED:\x1b[0m\n");
        generate_AZ_one_stripe_combinations_fixed(i, validHelpers, validHelpersNum, validRequesters, validRequestersNum, 0, currentHelpers, 0);
        printf("  Helpers: ");
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            printf("Block[%d] ", fixedSolutions[i].helpers[j]);
        }
        printf("\n");
        printf("  Requester: Node[%d]\n", fixedSolutions[i].requester);
        printf("  From Rack Size: %d\n", fixedSolutions[i].fromRackSize);
        printf("  From Racks: ");
        for(int j = 0; j < fixedSolutions[i].fromRackSize; ++j){
            printf("Rack[%d] ", fixedSolutions[i].fromRack[j]);
        }
        printf("\n");
        printf("  To Rack: Rack[%d]\n", fixedSolutions[i].toRack);
        printf("  Cost: %d\n", fixedSolutions[i].cost);

        printf("  \x1b[30mFLUCTUATE:\x1b[0m\n");
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            currentHelpers[j] = 0;
        }
        generate_AZ_one_stripe_combinations(i, validHelpers, validHelpersNum, validRequesters, validRequestersNum, 0, currentHelpers, 0);
        if(fluctuateSolutions[i].count == 0){
            printf("  No valid AZ solution found for stripe %d.\n", i);
        } else {
            printf("  Helpers: ");
            for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
                printf("Block[%d] ", fluctuateSolutions[i].helpers[j]);
            }
            printf("\n");
            printf("  Requester: Node[%d]\n", fluctuateSolutions[i].requester);
            printf("  From Rack Size: %d\n", fluctuateSolutions[i].fromRackSize);
            printf("  From Racks: ");
            for(int j = 0; j < fluctuateSolutions[i].fromRackSize; ++j){
                printf("Rack[%d] ", fluctuateSolutions[i].fromRack[j]);
            }
            printf("\n");
            printf("  To Rack: Rack[%d]\n", fluctuateSolutions[i].toRack);
            printf("  Count: %d\n", fluctuateSolutions[i].count);
            for(int j = 0; j < fluctuateSolutions[i].count; ++j){
                printf("  options[%d]: cost %d (prob %f)\n", j, fluctuateSolutions[i].options[j].cost, fluctuateSolutions[i].options[j].prob);
            }
        }
        printf("\n");
        
        free(validHelpers);
        free(validRequesters);
        free(currentHelpers);
    }

    printf("\x1b[34mcompute MT\x1b[0m\n");
    printf("  \x1b[30mFIXED:\x1b[0m\n");
    compute_TuTd_fixed(fixedSolutions);
    MAX_RACK_TIME_FIXED = find_max_rack_time_fixed();
    printf("  fixed AZ MT = %d\n", MAX_RACK_TIME_FIXED);
    printf("  \x1b[30mFLUCTUATE:\x1b[0m\n");
    compute_TuTd_fluctuate(fluctuateSolutions);
    MAX_RACK_TIME_FLUCTUATE = find_max_rack_time_fluctuate();
    printf("  fluctuate AZ MT = %f\n", MAX_RACK_TIME_FLUCTUATE);
    // print_MT_to_file("output/MT_Results.txt", "AZ-FIXED", MAX_RACK_TIME_FIXED, "AZ-FLUCTUATE", MAX_RACK_TIME_FLUCTUATE);

    printf("\n\x1b[34mAZ scheme repair links\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    reset_fixed_links();
    printf("  fixed solution total repair links: %d\n", TOTAL_REPAIR_LINK_NUM_FIXED);
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    reset_fluctuate_links();
    printf("  fluctuate solution total repair links: %d\n", TOTAL_REPAIR_LINK_NUM_FLUCTUATE);

    printf("\n\x1b[34mrandomly shuffle the repair links\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    shuffle(fixedLinks, TOTAL_REPAIR_LINK_NUM_FIXED);
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
        printf("  fixedLink %d: fromRack=%d, toRack=%d, cost=%d\n", i, fixedLinks[i].fromRack, fixedLinks[i].toRack, fixedLinks[i].cost);
    }
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    shuffle(fluctuateLinks, TOTAL_REPAIR_LINK_NUM_FLUCTUATE);
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i){
        printf("  fluctuateLink %d: fromRack=%d, toRack=%d\n", i, fluctuateLinks[i].fromRack, fluctuateLinks[i].toRack);
        printf("                   %d cost possibilities\n", fluctuateLinks[i].count);
        for(int j = 0; j < fluctuateLinks[i].count; ++j){
            printf("                   option[%d]: cost %d (prob %f)\n", j, fluctuateLinks[i].options[j].cost, fluctuateLinks[i].options[j].prob);
        }
    }

    printf("\n\x1b[34mschedule repair links\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    schedule_fixed_repair_links();
    int AZTotalRepairCost = 0;
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
        if(fixedLinks[i].endTime > AZTotalRepairCost){
            AZTotalRepairCost = fixedLinks[i].endTime;
        }
    }
    printf("  AZ total repair cost: %d\n", AZTotalRepairCost);

    printf("\n\x1b[34manalyze all possible scheduling results\x1b[0m\n");
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    int AZResultCount = 0;
    Result* AZResults = analyze_all_possibilities(fluctuateLinks, &AZResultCount);
    sort_results_by_time(AZResults, AZResultCount);
    if(AZResultCount == 0){
        printf("\nNo valid results were found. There might be scheduling conflicts or logical errors.\n");
    } else {
        printf("All possible total repair times and probabilities:\n");
        print_prob(AZResults, AZResultCount);
    }

    printf("\n\x1b[32m-------------------------------- CTP SCHEME --------------------------------\x1b[0m\n");
    for(int i = 0; i < STRIPE_NUM; ++i){
        substituteFixedSolus[i].requester = fixedSolutions[i].requester;
        substituteFixedSolus[i].fromRackSize = fixedSolutions[i].fromRackSize;
        substituteFixedSolus[i].toRack = fixedSolutions[i].toRack;
        substituteFixedSolus[i].cost = fixedSolutions[i].cost;
        substituteFluctuateSolus[i].requester = fluctuateSolutions[i].requester;
        substituteFluctuateSolus[i].fromRackSize = fluctuateSolutions[i].fromRackSize;
        substituteFluctuateSolus[i].toRack = fluctuateSolutions[i].toRack;
        substituteFluctuateSolus[i].count = fluctuateSolutions[i].count;
        substituteFluctuateSolus[i].options = (CostProbPair *)calloc(fluctuateSolutions[i].count, sizeof(CostProbPair));
        for(int j = 0; j < fluctuateSolutions[i].count; ++j){
            substituteFluctuateSolus[i].options[j].cost = fluctuateSolutions[i].options[j].cost;
            substituteFluctuateSolus[i].options[j].prob = fluctuateSolutions[i].options[j].prob;
        }
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            substituteFixedSolus[i].helpers[j] = fixedSolutions[i].helpers[j];
            substituteFluctuateSolus[i].helpers[j] = fluctuateSolutions[i].helpers[j];
        }
        for(int j = 0; j < fixedSolutions[i].fromRackSize; ++j){
            substituteFixedSolus[i].fromRack[j] = fixedSolutions[i].fromRack[j];
        }
        for(int j = 0; j < fluctuateSolutions[i].fromRackSize; ++j){
            substituteFluctuateSolus[i].fromRack[j] = fluctuateSolutions[i].fromRack[j];
        }
    }

    printf("\x1b[34mCTP scheme\x1b[0m");
    printf("\n\x1b[30mFIXED:\x1b[0m");
    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("\n\x1b[34mCTP optimizing stripe %d\x1b[0m\n", i);
        printf("\x1b[34mTuTd\x1b[0m\n");
        printf("solution[%d]: fromRackSize=%d, toRack=%d\n", i, fixedSolutions[i].fromRackSize, fixedSolutions[i].toRack);
        for(int j = 0; j < fixedSolutions[i].fromRackSize; ++j){
            printf("  fromRack[%d]=%d\n", j, fixedSolutions[i].fromRack[j]);
        }
        compute_TuTd_fixed(fixedSolutions);
        for(int i = 0; i < RACK_NUM; ++i){
            printf("Rack %d: Tu = %d, Td = %d\n", i, racks[i].Tu, racks[i].Td);
        }
        printf("\x1b[34mMT\x1b[0m\n");
        MAX_RACK_TIME_FIXED = find_max_rack_time_fixed();
        printf("MAX_RACK_TIME_FIXED = %d\n", MAX_RACK_TIME_FIXED);
        mark_max_rack_time_rack_fixed();

        for(int j = 0; j < RACK_NUM; ++j){
            if(racks[j].isMTRack && racks[j].isTuMax){
                bool found = 0;
                for(int k = 0; k < DATA_BLOCK_NUM_PER_STRIPE && !found; ++k){
                    if(blocks[fixedSolutions[i].helpers[k]].rackId == j){
                        found = 1;
                        printf("optimize helpers in rack %d\n", j);
                        bool optimized = false;
                        find_new_helpers(i, j, &optimized, true);
                        if(optimized){
                            printf("A substitute solution with a smaller MT has been found\n");
                            break;
                        } else {
                            printf("No substitute solutions found\n");
                        }
                    }
                }
                if(!found){
                    printf("Stripe %d has no helpers in Rack %d\n", i, j);
                }
            }
            if(racks[j].isMTRack && racks[j].isTdMax){
                if(nodes[fixedSolutions[i].requester].rackId == j){
                    printf("optimize requester in rack %d\n", j);
                    bool optimized = false;
                    find_new_requester_fixed(i, j, &optimized);
                    if(optimized){
                        printf("A substitute solution with a smaller MT has been found\n");
                        break;
                    }
                } else {
                    printf("Stripe %d has no requester in Rack %d\n", i, j);
                }
            }
        }
    }

    printf("\n\x1b[34mfinal fixed solutions after CTP optimization\x1b[0m\n");
    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("Stripe %d:\n", i);
        printf("  Helpers: ");
        for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
            printf("Block[%d] ", fixedSolutions[i].helpers[j]);
        }
        printf("\n");
        printf("  Requester: Node[%d]\n", fixedSolutions[i].requester);
        printf("  From Rack Size: %d\n", fixedSolutions[i].fromRackSize);
        printf("  From Racks: ");
        for(int j = 0; j < fixedSolutions[i].fromRackSize; ++j){
            printf("Rack[%d] ", fixedSolutions[i].fromRack[j]);
        }
        printf("\n");
        printf("  To Rack: Rack[%d]\n", fixedSolutions[i].toRack);
        printf("  Cost: %d\n", fixedSolutions[i].cost);
    }

    printf("\n\x1b[34mcompute MT\x1b[0m\n");
    printf("  \x1b[30mFIXED:\x1b[0m\n");
    printf("  fixed CTP MT = %d\n", MAX_RACK_TIME_FIXED);

    printf("\n\x1b[30mFLUCTUATE:\x1b[0m");
    clear_max_rack_time_rack_marks();
    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("\n\x1b[34mCTP optimizing stripe %d\x1b[0m\n", i);
        printf("\x1b[34mTuTd\x1b[0m\n");
        printf("solution[%d]: fromRackSize=%d, toRack=%d\n", i, fluctuateSolutions[i].fromRackSize, fluctuateSolutions[i].toRack);
        for(int j = 0; j < fluctuateSolutions[i].fromRackSize; ++j){
            printf("  fromRack[%d]=%d\n", j, fluctuateSolutions[i].fromRack[j]);
        }
        compute_TuTd_fluctuate(fluctuateSolutions);
        for(int i = 0; i < RACK_NUM; ++i){
            printf("Rack %d: TuCount = %d, TdCount = %d\n", i, racks[i].TuCount, racks[i].TdCount);
            for(int j = 0; j < racks[i].TuCount; ++j){
                printf("  TuOptions[%d]: cost %d (prob %f)\n", j, racks[i].TuOptions[j].cost, racks[i].TuOptions[j].prob);
            }
            for(int j = 0; j < racks[i].TdCount; ++j){
                printf("  TdOptions[%d]: cost %d (prob %f)\n", j, racks[i].TdOptions[j].cost, racks[i].TdOptions[j].prob);
            }
        }
        printf("\x1b[34mMT\x1b[0m\n");
        MAX_RACK_TIME_FLUCTUATE = find_max_rack_time_fluctuate();
        printf("MAX_RACK_TIME_FLUCTUATE = %f\n", MAX_RACK_TIME_FLUCTUATE);
        mark_max_rack_time_rack_fluctuate();

        for(int j = 0; j < RACK_NUM; ++j){
            if(racks[j].isMTRack && racks[j].isTuMax){
                bool found = 0;
                for(int k = 0; k < DATA_BLOCK_NUM_PER_STRIPE && !found; ++k){
                    if(blocks[fluctuateSolutions[i].helpers[k]].rackId == j){
                        found = 1;
                        printf("optimize helpers in rack %d\n", j);
                        bool optimized = false;
                        find_new_helpers(i, j, &optimized, false);
                        if(optimized){
                            printf("A substitute solution with a smaller MT has been found\n");
                            break;
                        } else {
                            printf("No substitute solutions found\n");
                        }
                    }
                }
                if(!found){
                    printf("Stripe %d has no helpers in Rack %d\n", i, j);
                }
            }
            if(racks[j].isMTRack && racks[j].isTdMax){
                if(nodes[fluctuateSolutions[i].requester].rackId == j){
                    printf("optimize requester in rack %d\n", j);
                    bool optimized = false;
                    find_new_requester_fluctuate(i, j, &optimized);
                    if(optimized){
                        printf("A substitute solution with a smaller MT has been found\n");
                        break;
                    }
                } else {
                    printf("Stripe %d has no requester in Rack %d\n", i, j);
                }
            }
        }
    }

    printf("\n\x1b[34mfinal fluctuate solution after CTP optimization\x1b[0m\n");
    for(int i = 0; i < STRIPE_NUM; ++i){
        printf("Stripe %d:\n", i);
        if(fluctuateSolutions[i].count == 0){
            printf("  No valid CTP solution found for stripe %d.\n", i);
        } else {
            printf("  Helpers: ");
            for(int j = 0; j < DATA_BLOCK_NUM_PER_STRIPE; ++j){
                printf("Block[%d] ", fluctuateSolutions[i].helpers[j]);
            }
            printf("\n");
            printf("  Requester: Node[%d]\n", fluctuateSolutions[i].requester);
            printf("  From Rack Size: %d\n", fluctuateSolutions[i].fromRackSize);
            printf("  From Racks: ");
            for(int j = 0; j < fluctuateSolutions[i].fromRackSize; ++j){
                printf("Rack[%d] ", fluctuateSolutions[i].fromRack[j]);
            }
            printf("\n");
            printf("  To Rack: Rack[%d]\n", fluctuateSolutions[i].toRack);
            printf("  Count: %d\n", fluctuateSolutions[i].count);
            for(int j = 0; j < fluctuateSolutions[i].count; ++j){
                printf("  options[%d]: cost %d (prob %f)\n", j, fluctuateSolutions[i].options[j].cost, fluctuateSolutions[i].options[j].prob);
            }
        }
    }

    printf("\n\x1b[34mcompute MT\x1b[0m\n");
    printf("  \x1b[30mFLUCTUATE:\x1b[0m\n");
    printf("  fluctuate CTP MT = %f\n", MAX_RACK_TIME_FLUCTUATE);
    // print_MT_to_file("output/MT_Results.txt", "CTP-FIXED", MAX_RACK_TIME_FIXED, "CTP-FLUCTUATE", MAX_RACK_TIME_FLUCTUATE);
    // print_empty_line_to_file("output/MT_Results.txt");

    printf("\n\x1b[34mCTP scheme repair links\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    reset_fixed_links();
    printf("  fixed solution total repair links: %d\n", TOTAL_REPAIR_LINK_NUM_FIXED);
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    reset_fluctuate_links();
    printf("  fluctuate solution total repair links: %d\n", TOTAL_REPAIR_LINK_NUM_FLUCTUATE);

    printf("\n\x1b[34mrandomly shuffle the repair links\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    shuffle(fixedLinks, TOTAL_REPAIR_LINK_NUM_FIXED);
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
        printf("  fixedLink %d: fromRack=%d, toRack=%d, cost=%d\n", i, fixedLinks[i].fromRack, fixedLinks[i].toRack, fixedLinks[i].cost);
    }
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    shuffle(fluctuateLinks, TOTAL_REPAIR_LINK_NUM_FLUCTUATE);
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i){
        printf("  fluctuateLink %d: fromRack=%d, toRack=%d\n", i, fluctuateLinks[i].fromRack, fluctuateLinks[i].toRack);
        printf("                   %d cost possibilities\n", fluctuateLinks[i].count);
        for(int j = 0; j < fluctuateLinks[i].count; ++j){
            printf("                   option[%d]: cost %d (prob %f)\n", j, fluctuateLinks[i].options[j].cost, fluctuateLinks[i].options[j].prob);
        }
    }

    printf("\n\x1b[34mschedule repair links\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    schedule_fixed_repair_links();
    int CTPTotalRepairCost = 0;
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FIXED; ++i){
        if(fixedLinks[i].endTime > CTPTotalRepairCost){
            CTPTotalRepairCost = fixedLinks[i].endTime;
        }
    }
    printf("  CTP total repair cost: %d\n", CTPTotalRepairCost);

    printf("\n\x1b[34manalyze all possible scheduling results\x1b[0m\n");
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    int CTPResultCount = 0;
    Result* CTPResults = analyze_all_possibilities(fluctuateLinks, &CTPResultCount);
    sort_results_by_time(CTPResults, CTPResultCount);
    if(CTPResultCount == 0){
        printf("\nNo valid results were found. There might be scheduling conflicts or logical errors.\n");
    } else {
        printf("All possible total repair times and probabilities:\n");
        print_prob(CTPResults, CTPResultCount);
    }

    printf("\n\x1b[32m---------------------------- COMPARE ALL SCHEMES ----------------------------\x1b[0m\n");
    printf("\x1b[30mFIXED:\x1b[0m\n");
    printf("  Random Scheme Total Repair Cost: %d\n", RandomTotalRepairCost);
    printf("  AZ Scheme Total Repair Cost: %d\n", AZTotalRepairCost);
    printf("  CTP Scheme Total Repair Cost: %d\n", CTPTotalRepairCost);
    printf("\x1b[30mFLUCTUATE:\x1b[0m\n");
    printf("  Random Scheme Possible Results:\n");
    print_prob(RandomResults, RandomResultCount);
    print_expected_highest_scheme(RandomResults, RandomResultCount);
    int RR_maxIndex = expected_highest_cost_index(RandomResults, RandomResultCount);
    printf("  Random Scheme Expected Highest Cost: %d\n", RandomResults[RR_maxIndex].totalTime);
    printf("  AZ Scheme Possible Results:\n");
    print_prob(AZResults, AZResultCount);
    print_expected_highest_scheme(AZResults, AZResultCount);
    int AZ_maxIndex = expected_highest_cost_index(AZResults, AZResultCount);
    printf("  AZ Scheme Expected Highest Cost: %d\n", AZResults[AZ_maxIndex].totalTime);
    printf("  CTP Scheme Possible Results:\n");
    print_prob(CTPResults, CTPResultCount);
    print_expected_highest_scheme(CTPResults, CTPResultCount);
    int CTP_maxIndex = expected_highest_cost_index(CTPResults, CTPResultCount);
    printf("  CTP Scheme Expected Highest Cost: %d\n", CTPResults[CTP_maxIndex].totalTime);
    // print_exp_results_to_file("output/Compare_Results_exp.txt", RandomTotalRepairCost, RandomResults, RandomResultCount, AZTotalRepairCost, AZResults, AZResultCount, CTPTotalRepairCost, CTPResults, CTPResultCount);
    // print_results_to_file("output/Compare_Results.txt", RandomTotalRepairCost, RandomResults, RandomResultCount, AZTotalRepairCost, AZResults, AZResultCount, CTPTotalRepairCost, CTPResults, CTPResultCount);

    for(int i = 0; i < RACK_NUM; ++i){
        for(int j = 0; j < RACK_NUM; ++j){
            free(rackTransfer[i][j].options);
        }
    }    
    for(int i = 0; i < RACK_NUM; ++i){
        free(racks[i].stripeBlockCount);
        free(rackTransfer[i]);
        free(racks[i].TuOptions);
        free(racks[i].TdOptions);
    }
    for(int i = 0; i < NODE_NUM; ++i){
        free(nodes[i].isSet);
    }
    for(int i = 0; i < STRIPE_NUM; ++i){
        free(fixedSolutions[i].helpers);
        free(fixedSolutions[i].fromRack);
        free(substituteFixedSolus[i].helpers);
        free(substituteFixedSolus[i].fromRack);
        free(fluctuateSolutions[i].helpers);
        free(fluctuateSolutions[i].fromRack);
        free(fluctuateSolutions[i].options);
        free(substituteFluctuateSolus[i].helpers);
        free(substituteFluctuateSolus[i].fromRack);
        free(substituteFluctuateSolus[i].options);
    }
    for(int i = 0; i < TOTAL_REPAIR_LINK_NUM_FLUCTUATE; ++i){
        free(fluctuateLinks[i].options);
    }
    free(racks);
    free(nodes);
    free(blocks);
    free(fixedSolutions);
    free(substituteFixedSolus);
    free(fluctuateSolutions);
    free(substituteFluctuateSolus);
    free(fixedLinks);
    free(fluctuateLinks);
    free(rackTransfer);
    free(RandomResults);
    free(AZResults);
    free(CTPResults);

    printf("\n\x1b[34mtotal time\x1b[0m\n");
    gettimeofday(&end, NULL);
    printf("time = %lds + %ldus\n", end.tv_sec - start.tv_sec, end.tv_usec - start.tv_usec);

    return 0;
}
