/**
 * 
 *      Main test script for the pddl_planner component 
 * 
 * usage:
 * 
 *  ./pddl_planner [-p <planner-name>] [-t <timeout-seconds(float)>] <domain-description-file> <problem-file>
 * 
 * 
 * 
 *                                      OR:
 *                                      --
 * 
 * usage:
 * 
 *  ./pddl_planner [-l <# of planners> <planner-name> <planner-name> ... ] [-t <timeout-seconds(float)>] [-s] <domain-description-file> <problem-file>
 * 
 * 
 * 
 *          -s,  --sequential           run listed planners sequentially (no threads)
 * 
 * 
 *      Note: Parallel planners execution is assumed by default.
 *      ----
 * 
 * 
 *      Rationale for using threads:
 *      ---------------------------
 *  
 *  -> threads created in the main function have access to the common global string variable 'output'; 
 *      - it is more convenient to carefully append (!concurrency issues!) each individual planner execution result 
 *      to the 'output', rather than redirecting the standard output of the planners to a common location, later on
 *      reading it again
 *      - functionality to read the plan files is already implemented at the planners level and their common interface (PDDLPlannerInterface),
 *      the level where the filenames and locations of the result plans are known.
 *                                                                          -----
 * 
 * 
 *  -> synchronizing the waiting for results is simpler and more reliable when using the threads API (i.e. simply joining the individual threads)
 *  rather than sending signals around from forked child processes (the ones dealing with individual planners) back to the 
 *  main process (their common parent)
 * 
 * 
 *  -> threads are much more time and memory efficient (they share the same memory, switching between threads is much faster for the scheduler 
 *  than switching between processes and so)
 * 
 */



#include <iostream>
#include <stdio.h>
#include <map>
#include <set>
#include <errno.h>
#include <pddl_planner/Planning.hpp>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#define TIMEOUT 7.
// #define INPUT_VERIFICATION

double timeout = TIMEOUT;
bool seq = false, liste = false;
std::set<std::string> planners;
pddl_planner::Planning planning;
std::string problemDescription;

void usage(int argc, char** argv)
{
    printf("usage: %s [-p <planner-name>] [-t <timeout-seconds(float)>] <domain-description-file> <problem-file>\n",argv[0]);
    printf("or\n");
    printf("usage: %s [-l <# of planners> <planner-name> <planner-name> ... ] [-t <timeout-seconds(float)>]\n", argv[0]);
    printf("          [-s] <domain-description-file> <problem-file>\n");
    printf("DESCRIPTION OF OPTIONS\n");
    printf("      -s,--sequential    run listed planners sequentially (no threads)\n");
    std::set<std::string> availablePlanners = planning.getAvailablePlanners();
    std::set<std::string>::iterator it = availablePlanners.begin();
    printf("AVAILABLE PLANNERS\n");
    for(; it != availablePlanners.end(); ++it)
    {
        printf("    %s\n", it->c_str());
    }
}

int main(int argc, char** argv)
{
    using namespace pddl_planner;

    // defaultPlanner is LAMA
    if(argc < 2 )
    {
        usage(argc, argv);
        exit(0);
    }
    std::string firstArg = argv[1];
    if(firstArg == "-h" || firstArg == "--help")
    {
        usage(argc, argv);
        exit(0);
    }

    std::string plannerName = "LAMA";
    std::string domainFilename;
    std::string problemFilename; 

    if(firstArg == "-p" && (argc == 5 || 7 == argc))
    {
        if(5 == argc)
        {
            plannerName = argv[2];
            domainFilename = argv[3];
            problemFilename = argv[4];
        }
        else
        {
            if("-t" != std::string(argv[3]))
            {
                usage(argc, argv);
                exit(0);
            }
            plannerName = argv[2];
            timeout = atof(argv[4]);
            domainFilename = argv[5];
            problemFilename = argv[6];
        }
    } 
    else if("-t" == firstArg && 7 == argc)
    {
        if("-p" != std::string(argv[3]))
        {
            usage(argc, argv);
            exit(0);
        }
        plannerName = argv[4];
        timeout = atof(argv[2]);
        domainFilename = argv[5];
        problemFilename = argv[6];

    }
    else if(argc == 3)
    {
        domainFilename = argv[1];
        problemFilename = argv[2];
    } 
    else if("-l" == firstArg)
    {
        liste = true;
        if(argc < 3)
        {
            printf("Too few arguments were provided!\n");
            usage(argc, argv);
            exit(0);
        }
        int nplanners = atoi(argv[2]);
        if(nplanners < 1)
        {
            printf("In a list of planners, the number of planners has to be at least 1!\n");
            usage(argc, argv);
            exit(0);
        }
        if(argc < 5 + nplanners) // 3 + nplanners + 2
        {
            printf("Too few arguments were provided!\n");
            usage(argc, argv);
            exit(0);
        }
        for(int i = 0; i < nplanners; ++i)
        {
            planners.insert(std::string(argv[3 + i]));
        }
        if("-t" == std::string(argv[3 + nplanners]))
        {
            if(argc < 7 + nplanners) // 3 + nplanners + 2 + 2
            {
                printf("Too few arguments were provided!\n");
                usage(argc, argv);
                exit(0);
            }
            timeout = atof(argv[4 + nplanners]);
            if("--sequential" == std::string(argv[5 + nplanners]) || "-s" == std::string(argv[5 + nplanners]))
            {
                if(argc < 8 + nplanners) // 3 + nplanners + 2 + 2 + 1
                {
                    printf("Too few arguments were provided!\n");
                    usage(argc, argv);
                    exit(0);
                }
                domainFilename  = argv[6 + nplanners];
                problemFilename = argv[7 + nplanners];
                seq = true;
            }
            else
            {
                domainFilename  = argv[5 + nplanners];
                problemFilename = argv[6 + nplanners];
            }
        }
        else if("--sequential" == std::string(argv[3 + nplanners]) || "-s" == std::string(argv[3 + nplanners]))
        {
            if(argc < 6 + nplanners) // 3 + nplanners + 2 + 1
            {
                printf("Too few arguments were provided!\n");
                usage(argc, argv);
                exit(0);
            }
            seq = true;
            if("-t" == std::string(argv[4 + nplanners]))
            {
                if(argc < 8 + nplanners) // 3 + nplanners + 2 + 1 + 2
                {
                    printf("Too few arguments were provided!\n");
                    usage(argc, argv);
                    exit(0);
                }
                timeout = atof(argv[5 + nplanners]);
                domainFilename  = argv[6 + nplanners];
                problemFilename = argv[7 + nplanners];
            }
            else
            {
                domainFilename  = argv[4 + nplanners];
                problemFilename = argv[5 + nplanners];
            }
        }
        else
        {
            domainFilename  = argv[3 + nplanners];
            problemFilename = argv[4 + nplanners];
        }
    }
    else
    {
        usage(argc, argv);
        exit(0);
    }

    FILE* domainFile = fopen(domainFilename.c_str(),"r"); 
    if(!domainFile)
    {
        printf("Error opening file: '%s' -- %s", domainFilename.c_str(), strerror(errno));
        exit(-1);
    }

    char buffer[512];
    std::string domainDescription;

    while(fgets(buffer,512,domainFile) != NULL)
    {
        domainDescription += std::string(buffer);
    }
    fclose(domainFile);


    planning.setDomainDescription("test-domain", domainDescription);

    FILE* problemFile = fopen(problemFilename.c_str(), "r");
    if(!problemFile)
    {
        printf("Error opening file: '%s' -- %s", problemFilename.c_str(), strerror(errno));
        exit(-1);
    }

    while(fgets(buffer, 512, problemFile) != NULL)
    {
        problemDescription += std::string(buffer);
    }
    fclose(problemFile);

#ifdef INPUT_VERIFICATION
    printf("Input:\n    planner(s)Name  = ");
    for(std::set<std::string>::iterator it = planners.begin(); it != planners.end(); ++it)
    {
        printf("%s ", (*it).c_str());
    }
    printf("\n    domainFilename  = %s\n    problemFilename = %s\n    timeout         = %lf (sec)\n    list            = %s\n    sequential      = %s\n", domainFilename.c_str(), problemFilename.c_str(), timeout, liste ? "true" : "false", seq ? "true" : "false");
#endif

    if(!liste)
    {
        planners.insert(plannerName);
    }
    try
    {
        PlanResultList planResultList = planning.plan(problemDescription, planners, seq, timeout);
        PlanResultList::iterator it = planResultList.begin();
        for(; planResultList.end() != it; ++it)
        {
            PlanResult plan = (*it);
            printf("Planner %s:\n%s\n", plan.first.c_str(), plan.second.toString().c_str());
        }
    }
    catch(const std::runtime_error& e)
    {
        printf("Error: %s\n", e.what());
        if(!strncmp(e.what(),"pddl_planner::Planning: planner with name '", strlen("pddl_planner::Planning: planner with name '")))
        {
            printf("    Registered planners:\n");
            PlannerMap planners = planning.getPlanners();
            PlannerMap::iterator it = planners.begin();
            for(; it != planners.end(); ++it)
            {
                printf("%s ", it->first.c_str());
            }
            printf("\nFor a list of available planners (out of the registered ones) please use option \"--help\" alone!\n");
        }
    }



    return 0;
}
