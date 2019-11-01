#include <stdio.h>
#include <iostream>
#include <mpi.h>
#include <boost/program_options.hpp>

#include <hdb/configuration/SystemConfig.h>
#include <hdb/locktable/LockTableAgent.h>
#include <hdb/transactions/TransactionAgent.h>
#include <hdb/transactions/SimpleClient.h>
#include <hdb/transactions/ReplayClient.h>
#include <hdb/stats/ClientStats.h>
#include <hdb/utils/Debug.h>

#ifdef USE_FOMPI
#include <fompi.h>
#endif

int main(int argc, char *argv[]) {

    boost::program_options::options_description programDescription("Distributed locking.\nAllowed options");

    boost::program_options::options_description genericOpt("Generic options");
    genericOpt.add_options()
            ("help","produce help message")
            ("timelimit", boost::program_options::value<uint32_t>()->default_value(120), "runtime limit in seconds")
            ("logname", boost::program_options::value<std::string>(), "output file name (default: PWD/TIME.exp)");

    boost::program_options::options_description testOpt("Configuration options");
    testOpt.add_options()
            ("processespernode", boost::program_options::value<uint32_t>()->default_value(1), "number of transaction processes per node")
            ("locksperwh", boost::program_options::value<uint32_t>()->default_value(1000), "number of locks per warehouse")
            ("warehouses", boost::program_options::value<uint32_t>()->default_value(1), "number of warehouses")
            ("socketspernode", boost::program_options::value<uint32_t>()->default_value(1), "number of sockets per node. (It helps to colocate transaction agents with their home lock agents to the same socket)");

 
    boost::program_options::options_description transactionOpt("Transaction algorithm");
    transactionOpt.add_options()
            ("simple2pl","simple 2PL (default)")
            ("waitdie", "wait-die strategy")
            ("nowait", "nowait lock strategy") 
            ("timestamp", "timestamp lock strategy"); 
    
    boost::program_options::options_description workloadOpt("Workloads");
    workloadOpt.add_options()
            ("localworkloadsize", boost::program_options::value<uint32_t>()->default_value(1000), "number of transactions per client")
            ("workloadlocation", boost::program_options::value<std::string>(), "workload location")
            ("synthetic", "run synthetic workload")
            ("remotelockprob", boost::program_options::value<uint32_t>()->default_value(0), "probability of remote lock (synthetic workload only)");
            
    boost::program_options::options_description traceOpt("Trace modifications");
    traceOpt.add_options()
            ("neworder", "filter transactions to keep only neworder transactions (can be combined with --payment)")
            ("payment", "filter transactions to keep only payment transactions (can be combined with --neworder)");
        
    boost::program_options::options_description othersOpt("Others modifications"); 
    othersOpt.add_options()
            ("comdelay", boost::program_options::value<uint32_t>()->default_value(0), "add a delay in nanosec before each communication")
            ("hashlock", "randomly assign locks to lock agents");  

    programDescription.add(genericOpt).add(testOpt).add(transactionOpt).add(workloadOpt).add(traceOpt).add(othersOpt);


    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    hdb::configuration::SystemConfig config;


    if(commandLineArgs.count("help")){
       std::cout<<programDescription<<"\n";
       return 1;
    }

    uint32_t localWorkloadSize = commandLineArgs["localworkloadsize"].as<uint32_t>();
    std::string workloadLocation = commandLineArgs.count("workloadlocation") > 0 ? commandLineArgs["workloadlocation"].as<std::string>() : "";

    if(commandLineArgs.count("workloadlocation")==0 && !commandLineArgs.count("synthetic")){
        std::cout<<"Error! workloadlocation is not specified."<<"\n";
        std::cout<<programDescription<<"\n";
        return 1;
    }

 
#ifdef USE_FOMPI
    foMPI_Init(NULL, NULL);
#else 
    MPI_Init(NULL, NULL);
#endif

    MPI_Comm_rank(MPI_COMM_WORLD, &(config.globalRank));
    MPI_Comm_size(MPI_COMM_WORLD, &(config.globalNumberOfProcesses));

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &(config.localCommunicator));
    MPI_Comm_rank(config.localCommunicator, &(config.localRank));
    MPI_Comm_size(config.localCommunicator, &(config.localNumberOfProcesses));


#ifdef USE_FOMPI
    if (config.globalRank == 0) {
        printf("Using foMPI\n");
        fflush(stdout);
    }
#endif  

    config.numberOfNodes = config.globalNumberOfProcesses / config.localNumberOfProcesses;

    /*
     * Compute assignment of roles to processes
     */

    uint32_t localNumberOfTransactionAgents = commandLineArgs["processespernode"].as<uint32_t>();
    uint32_t localNumberOfLockTableAgents = config.localNumberOfProcesses - localNumberOfTransactionAgents;
 
    uint32_t numberOfSocketsPerNode = commandLineArgs["socketspernode"].as<uint32_t>();



    uint32_t numberOfLockServerPerSocket = localNumberOfLockTableAgents / numberOfSocketsPerNode;
    uint32_t numberOfProcessesPerSocket = config.localNumberOfProcesses / numberOfSocketsPerNode;

    config.globalNumberOfLockTableAgents = localNumberOfLockTableAgents * config.numberOfNodes;
    
    config.lockServerGlobalRanks = new uint32_t[config.globalNumberOfLockTableAgents];
    config.globalNumberOfWarehouses = commandLineArgs["warehouses"].as<uint32_t>() ;
    


    config.globalNumberOfTransactionAgents =  config.globalNumberOfProcesses - config.globalNumberOfLockTableAgents;
       
 
    config.syntheticmode = commandLineArgs.count("synthetic") ? true : false;

    if( commandLineArgs.count("neworder") ){
        if (config.globalRank == 0) {
            DLOG_ALWAYS("Main", "neworder filter is enabled\n");
        }
        config.transaction_constraints.push_back("neword");
    };

    if( commandLineArgs.count("payment") ){
        if (config.globalRank == 0) {
            DLOG_ALWAYS("Main", "payment filter is enabled\n");
        }
        config.transaction_constraints.push_back("payment");
    };

    
    config.comdelay = commandLineArgs["comdelay"].as<uint32_t>();
    if (config.globalRank == 0) {
        DLOG_ALWAYS("Main", "Communication delay is %d ns\n", config.comdelay);
    }

    config.waitdie = false;
    if( commandLineArgs.count("waitdie") ){
        config.waitdie = true;
    };

  
    config.nowait = false;
    if( commandLineArgs.count("nowait") ){
        config.nowait = true;
    };

    config.timestamp = false;
    if( commandLineArgs.count("timestamp") ){
        config.timestamp = true;
    };

    config.hashlock = false;
    if( commandLineArgs.count("hashlock") ){
        if (config.globalRank == 0) {
            DLOG_ALWAYS("Main", "hashlock assignment is enabled\n");
        }
        config.hashlock = true;
    };


    config.timelimit = commandLineArgs["timelimit"].as<uint32_t>();
     
    config.locksPerWarehouse = commandLineArgs["locksperwh"].as<uint32_t>();
     

    config.globalNumberOfLocks = config.globalNumberOfWarehouses * config.locksPerWarehouse;
    if (config.globalRank == 0) {

        DLOG_ALWAYS("Main", "The total number of locks %d", config.globalNumberOfLocks);
        DLOG_ALWAYS("Main", "The total number of warehouses %d", config.globalNumberOfWarehouses );
        DLOG_ALWAYS("Main", "The total number of transaction agents %d", config.globalNumberOfTransactionAgents );
        DLOG_ALWAYS("Main", "The total number of lock agents %d", config.globalNumberOfLockTableAgents );

        DLOG_ALWAYS("Main", "The number of transaction agents per node %d", localNumberOfTransactionAgents );
        DLOG_ALWAYS("Main", "The number of lock agents per node %d", localNumberOfLockTableAgents );
    }
 
    uint32_t c = 0;
    config.isLockTableAgent = false;
    config.internalRank = 0;
    config.locksOnThisAgent = 0;

    // Compute lock server ranks
    for (uint32_t n = 0; n < config.numberOfNodes; ++n) {
        uint32_t machineStartId = n * config.localNumberOfProcesses;
        for (uint32_t s = 0; s < numberOfSocketsPerNode; ++s) {
            uint32_t socketStartId = machineStartId + s * numberOfProcessesPerSocket;
            for (uint32_t l = 0; l < numberOfLockServerPerSocket; ++l) {
                config.lockServerGlobalRanks[c] = socketStartId + l;
                if (socketStartId + l == config.globalRank) {
                    config.isLockTableAgent = true;
                    config.internalRank = c;
                }
                ++c;
            }
        }

    }

    // Compute transaction processing ranks
    c = 0;
    for (uint32_t n = 0; n < config.numberOfNodes; ++n) {
        uint32_t machineStartId = n * config.localNumberOfProcesses;
        for (uint32_t s = 0; s < numberOfSocketsPerNode; ++s) {
            uint32_t socketStartId = machineStartId + s * numberOfProcessesPerSocket + numberOfLockServerPerSocket;
            for (uint32_t t = 0; t < (numberOfProcessesPerSocket - numberOfLockServerPerSocket); ++t) {
                if (socketStartId + t == config.globalRank) {
                    config.internalRank = c;
                }
                ++c;
            }
        }
    }

    
    config.AllLocksPerAgent.resize(config.globalNumberOfLockTableAgents); 
 

    // create mapping from warehouse to lockserver
    {
        uint32_t  wareHousesPerLockServer = config.globalNumberOfWarehouses / config.globalNumberOfLockTableAgents;
            
        config.warehouseToLockServer.resize(config.globalNumberOfWarehouses + 1);
        config.warehouseToLockServer[0] = 0;

        // normal case. when a warehouse is managed by a single lockagent
        if(wareHousesPerLockServer != 0){

            float step = ((float)config.globalNumberOfWarehouses) / config.globalNumberOfLockTableAgents;
            uint32_t indx = 0;
            for(uint32_t i=0 ; i < config.globalNumberOfLockTableAgents; i++){
                uint32_t currentwh = (uint32_t)((i+1)*step)-(uint32_t)((i)*step);
                config.AllLocksPerAgent[i] =  currentwh*config.locksPerWarehouse;
                for(uint32_t j=0; j<currentwh; j++){
                    config.warehouseToLockServer[indx] = i; 
                    indx++;         
                }
            }
            config.warehouseToLockServer[indx] = config.warehouseToLockServer[indx-1] + 1;

        } // distributed case. when a warehouse is managed by many lockagents
        else {
            if(config.numberOfNodes / config.globalNumberOfWarehouses !=0 ){
                float step = ((float)config.numberOfNodes) / config.globalNumberOfWarehouses;

                config.warehouseToLockServer[0] = 0;
                for(uint32_t i = 1 ; i <= config.globalNumberOfWarehouses; i++){
                    uint32_t NodesPerWarehouse = (uint32_t)((i)*step)-(uint32_t)((i-1)*step);
                    uint32_t LockServersPerwareHouse = NodesPerWarehouse * localNumberOfLockTableAgents;
                    config.warehouseToLockServer[i] = config.warehouseToLockServer[i-1] + LockServersPerwareHouse;

                    uint32_t j =0;
                    for(uint32_t lockAgentid = config.warehouseToLockServer[i-1]; lockAgentid < config.warehouseToLockServer[i] ; lockAgentid++ ){

                        config.AllLocksPerAgent[lockAgentid] = config.locksPerWarehouse / LockServersPerwareHouse + 
                                                             ((config.locksPerWarehouse % LockServersPerwareHouse > j) ? 1 : 0 );
                        j++;
                    }
                }
            }else { // if more wh then nodes
                float step = ((float) config.globalNumberOfWarehouses)/ config.numberOfNodes; // warehouses per node
                config.warehouseToLockServer[0] = 0; 
                uint32_t idx = 0;
                for(uint32_t nodeId = 0; nodeId < config.numberOfNodes; nodeId++){
                    uint32_t WarehousesPerNode = (uint32_t)((nodeId+1)*step)-(uint32_t)((nodeId)*step);
                    float step2 = ((float)localNumberOfLockTableAgents) / WarehousesPerNode;
                    for(uint32_t wh = 0; wh < WarehousesPerNode; wh++){
                        uint32_t LockServersPerwareHouse = (uint32_t)((wh+1)*step2)-(uint32_t)((wh)*step2);
                        config.warehouseToLockServer[idx+1] = config.warehouseToLockServer[idx] + LockServersPerwareHouse;

                        uint32_t j=0;
                        for(uint32_t lockAgentid = config.warehouseToLockServer[idx]; lockAgentid < config.warehouseToLockServer[idx+1] ; lockAgentid++ ){
                            config.AllLocksPerAgent[lockAgentid] = config.locksPerWarehouse / LockServersPerwareHouse+ 
                                                             ((config.locksPerWarehouse % LockServersPerwareHouse > j) ? 1 : 0 );
                            j++;
                        }
 
                        idx++;
                    }                                   
                }
            }
        }
    }   


    if(config.isLockTableAgent){
        config.locksOnThisAgent = config.AllLocksPerAgent[config.internalRank];
    }

    if (config.globalRank == 0) {
        printf("[Main] The number of locks for each lock agent: ");
        for(uint32_t x : config.AllLocksPerAgent){
            printf("%d ",x);
        }
        printf("\n");
    }
    /*
     * Setup log output
     */

    FILE *logOutput = NULL;
    if (config.globalRank == 0) {
        char fileName[1024];
        if (commandLineArgs.count("logname") == 0) {
            memset(fileName, 0, 1024);
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            uint64_t experimentId = currentTime.tv_sec * 1000000L + currentTime.tv_usec;
            sprintf(fileName, "%lu.exp", experimentId);
        } else {
            
            sprintf(fileName, "%s.exp", commandLineArgs["logname"].as<std::string>().c_str());
        }
        DLOG_ALWAYS("Main", "Output file: %s\n",fileName);
        logOutput = fopen(fileName, "w");
    }

    /*
     * Start processes
     */


    if (config.isLockTableAgent) {

        hdb::locktable::LockTableAgent *lockTableAgent = new hdb::locktable::LockTableAgent(&config);
        MPI_Barrier(MPI_COMM_WORLD);
        DLOG("Main", "Process %d starting", config.globalRank);
        lockTableAgent->execute();
        DLOG("Main", "Process %d finished", config.globalRank);
        MPI_Barrier(MPI_COMM_WORLD);
        DLOG("Main", "Process %d passed final barrier", config.globalRank);
        delete lockTableAgent;

    } else {

        hdb::transactions::TransactionAgent *transactionAgent;
        if(config.syntheticmode)
            transactionAgent = new hdb::transactions::SimpleClient(&config, localWorkloadSize, 10, commandLineArgs["remotelockprob"].as<uint32_t>());
        else{
            transactionAgent = new hdb::transactions::ReplayClient(&config, workloadLocation, localWorkloadSize);
        }
        transactionAgent->generate();

        MPI_Barrier(MPI_COMM_WORLD);
 
        transactionAgent->execute();
        transactionAgent->shutdown();
        MPI_Barrier(MPI_COMM_WORLD);
        DLOG("Main", "Process %d passed final barrier", config.globalRank);

        if (config.globalRank == 0) {
            transactionAgent->stats.dumpToFile(logOutput);
        } else {
            transactionAgent->stats.sendDataToRoot();
        }
        delete transactionAgent;
    }
    /*
     * Output log data
     */
    if (config.globalRank == 0) {
        uint32_t clientsToReceiveFrom = config.globalNumberOfTransactionAgents;
        if(!config.isLockTableAgent) --clientsToReceiveFrom;
        for (uint32_t i = 0; i < clientsToReceiveFrom; ++i) {
            hdb::stats::ClientStats statsBuffer;
            MPI_Recv(&statsBuffer, sizeof(hdb::stats::ClientStats), MPI_BYTE, MPI_ANY_SOURCE, CLIENT_STAT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            statsBuffer.dumpToFile(logOutput);
        }
        fclose(logOutput);
        DLOG_ALWAYS("Main", "The experiment has been finished");
    }
 
#ifdef USE_FOMPI
    foMPI_Finalize();
#else
    MPI_Finalize();
#endif
    
    return 0;

}
