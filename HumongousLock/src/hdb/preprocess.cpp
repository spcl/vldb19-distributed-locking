#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>

#include <hdb/transactions/Transaction.h>

std::vector<std::string> transaction_constraints; 
uint32_t globalNumberOfLocks = 0;
std::string workloadLocation = "";

bool check_existance(uint32_t wh, uint32_t numberOfTransactions, uint32_t clientsPerWarehouse){
    std::ostringstream stringStream;
    stringStream << workloadLocation << "/wh" << wh + 1 << "_" << numberOfTransactions << "_" << clientsPerWarehouse-1 << "_" << clientsPerWarehouse;

    for(std::string str : transaction_constraints){
        stringStream << "_";
        stringStream << str;
    }
    stringStream << ".bin";

    std::string copyOfStr = stringStream.str();
    std::ifstream ifs(copyOfStr.c_str());
    return ifs.good(); 
}

void generate(uint32_t wh, uint32_t numberOfTransactions, uint32_t clientsPerWarehouse ){

    
    if(check_existance(wh,numberOfTransactions,clientsPerWarehouse) ){
        printf("bin files already exist\n");
        return; 
    }

    std::vector< std::vector<hdb::transactions::Transaction> * > all_transactions;
    for(uint32_t i= 0; i < clientsPerWarehouse; i++ ){
        all_transactions.push_back( new std::vector<hdb::transactions::Transaction>() );
        all_transactions[i]->reserve(numberOfTransactions);
    }

    
    char fileName[1024];
    memset(fileName, 0, 1024);
    sprintf(fileName, "%s/wh%06d.csv", workloadLocation.c_str(), wh + 1 );
    FILE *file = fopen(fileName, "r");
    if(file==NULL){
        printf ("%s is not found. Change the folder or unpack *.xz files with unxz\n",fileName);
        exit(EXIT_FAILURE);
    }

    hdb::transactions::Transaction *item = NULL;
    uint64_t currentTransactionId = 0;

    uint64_t transactioncounter = 0;
    uint64_t skip = 0; 
    uint64_t currentclient = 0;


    while (transactioncounter < numberOfTransactions) {

        uint64_t transactionId;
        uint64_t lockId;
        char lockMode[2];
        char queryType[16];

        char lineBuffer[128];
        bool validRead = (fgets(lineBuffer, sizeof(lineBuffer), file) != NULL);

        if(validRead){
            int readItems = sscanf(lineBuffer, "%lu,%lu,%2[^,],%s", &transactionId, &lockId, lockMode, queryType);
            validRead = (readItems == 4);
        }
        if (!validRead) {
            if (skip != transactionId && item != NULL) {
                all_transactions[currentclient]->push_back(*item);
                currentclient = (currentclient + 1) % clientsPerWarehouse;
                if(currentclient==0){
                    transactioncounter++;
                }
            }
            delete item;

            //stop processing
            break;
        }

 
        if(skip == transactionId){
            continue;
        }
 
    
        if (transactionId != currentTransactionId) { // new transaction

            
            if (item != NULL) {
                if(item->requests.size() < 750) {
                    all_transactions[currentclient]->push_back(*item);
                    currentclient = (currentclient + 1) % clientsPerWarehouse;
                    if(currentclient==0){
                        transactioncounter++;
                    }
                    delete item;
                    item = NULL;
                } else {
//                  DLOG_ALWAYS("ReplayClient", "Ignoring transaction that has %lu locks", item->requests.size());
                    delete item;
                    item = NULL;
                }
            }

            currentTransactionId = transactionId;

            // check constraints
            if( !transaction_constraints.empty() ){
                // if not in the list
                if (std::find(transaction_constraints.begin(), transaction_constraints.end(), queryType) ==  transaction_constraints.end()){
                    skip = transactionId;
                    continue;
                }
            }

            // the trace has only one type per transaction
            item = new hdb::transactions::Transaction(hdb::transactions::DATA_MODE_FROM_STRING(queryType));
        } 
        
        hdb::locktable::LockMode lm = hdb::locktable::LOCK_MODE_FROM_STRING(lockMode);
        if(lm == hdb::locktable::LockMode::NL){
            printf("Warning! Lock is unidentified %s from string %s\n",lockMode,lineBuffer);
        }
//      DLOG("ReplayClient", "Read workload item (%lu %lu %s %s).", transactionId, lockId, lockMode, queryType);
        item->requests.push_back(lockid_mode_pair_t(lockId, lm));
    }

    fclose(file);

    for(uint32_t cl =0 ; cl < clientsPerWarehouse; cl++){

        std::ostringstream stringStream;
        stringStream << workloadLocation << "/wh" << wh + 1 << "_" << numberOfTransactions << "_" << cl << "_" << clientsPerWarehouse;

        for(std::string str : transaction_constraints){
            stringStream << "_";
            stringStream << str;
        }
        stringStream << ".bin";
        std::string copyOfStr = stringStream.str();
        std::ofstream ofs(copyOfStr.c_str());
        {
            boost::archive::binary_oarchive oa(ofs);
            uint32_t size  = all_transactions[cl]->size();
            oa << size;
            oa << boost::serialization::make_array(all_transactions[cl]->data(), size);
     
            delete all_transactions[cl];
        }
    }
 
}

int main(int argc, char *argv[]) {

    boost::program_options::options_description programDescription("Preprocessing of traces.\nAllowed options");

    boost::program_options::options_description genericOpt("Generic options");
    genericOpt.add_options()
            ("help", "produce help message");

    boost::program_options::options_description testOpt("Configuration options");
    testOpt.add_options()
            ("processespernode", boost::program_options::value<uint32_t>()->default_value(1), "number of transaction processes per node")
            ("locksperwh", boost::program_options::value<uint32_t>()->default_value(1000), "number of locks per warehouse")
            ("warehouses", boost::program_options::value<uint32_t>()->default_value(1), "number of warehouses")
            ("numberofnodes", boost::program_options::value<uint32_t>()->default_value(1), "number of nodes");

    boost::program_options::options_description workloadOpt("Workloads");
    workloadOpt.add_options()
            ("localworkloadsize", boost::program_options::value<uint32_t>()->default_value(1000), "number of transactions per client")
            ("workloadlocation", boost::program_options::value<std::string>(), "workload location");
        
    boost::program_options::options_description traceOpt("Trace modifications");
    traceOpt.add_options()
            ("neworder", "filter transactions to keep only neworder transactions (can be combined with --payment)")
            ("payment", "filter transactions to keep only payment transactions (can be combined with --neworder)");
        
    programDescription.add(genericOpt).add(testOpt).add(workloadOpt).add(traceOpt);    

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    if(commandLineArgs.count("help")){
        std::cout<<programDescription<<"\n";
        return 1;
    }

    uint32_t NumberOfTransactionAgentsPerNode = commandLineArgs["processespernode"].as<uint32_t>();
    uint32_t numberOfNodes = commandLineArgs["numberofnodes"].as<uint32_t>();
    uint32_t globalNumberOfWarehouses = commandLineArgs["warehouses"].as<uint32_t>() ;
    uint32_t globalNumberOfTransactionAgents = NumberOfTransactionAgentsPerNode * numberOfNodes;
    
    
    uint32_t numberOfTransactions = commandLineArgs["localworkloadsize"].as<uint32_t>();

    if(commandLineArgs.count("workloadlocation")==0){
        std::cout<<"Error! workloadlocation is not specified."<<"\n";
        std::cout<<programDescription<<"\n";
        return 1;
    }


    workloadLocation = commandLineArgs["workloadlocation"].as<std::string>();
     
 
    uint32_t locksPerWarehouse = commandLineArgs["locksperwh"].as<uint32_t>();
    globalNumberOfLocks = globalNumberOfWarehouses * locksPerWarehouse;


    if( commandLineArgs.count("neworder") ){
        transaction_constraints.push_back("neword");
    };

    if( commandLineArgs.count("payment") ){
        transaction_constraints.push_back("payment");
    };


    uint32_t clientsPerWarehouse =  globalNumberOfTransactionAgents / globalNumberOfWarehouses;
    if(clientsPerWarehouse == 0){
        float delta = (float)globalNumberOfWarehouses / (float)globalNumberOfTransactionAgents;
        for(uint32_t clientId = 0; clientId < globalNumberOfTransactionAgents; clientId++){
            uint32_t wh = static_cast<uint32_t>(clientId * delta) ;
            uint32_t sharedby = 1;
            printf("Warehouse %d is shared by %d\n",wh,sharedby);
            generate(wh,numberOfTransactions,sharedby);
        }
    } else {
        if(numberOfNodes  / globalNumberOfWarehouses != 0){ // more nodes then wh 
            float step = ((float)numberOfNodes) / globalNumberOfWarehouses;
            for(uint32_t wh = 0; wh < globalNumberOfWarehouses; wh++){
                uint32_t NodesPerWarehouse = (uint32_t)((wh+1)*step)-(uint32_t)((wh)*step);
                uint32_t sharedby = NodesPerWarehouse*NumberOfTransactionAgentsPerNode;
                printf("Warehouse %d is shared by %d\n",wh,sharedby);
                generate(wh,numberOfTransactions,sharedby);
            }
        } 
        else { // if more wh then nodes
            float step = ((float)globalNumberOfWarehouses)/ numberOfNodes; // warehouses per node
            for(uint32_t nodeId = 0; nodeId < numberOfNodes; nodeId++){
                uint32_t WarehousesPerNode = (uint32_t)((nodeId+1)*step)-(uint32_t)((nodeId)*step);
                float step2 = ((float)NumberOfTransactionAgentsPerNode) / WarehousesPerNode;
                for(uint32_t wh = 0; wh < WarehousesPerNode; wh++){
                    uint32_t sharedby = (uint32_t)((wh+1)*step2)-(uint32_t)((wh)*step2);
                    printf("Warehouse %d is shared by %d\n",wh+(uint32_t)((nodeId)*step),sharedby);
                    generate(wh+(uint32_t)((nodeId)*step),numberOfTransactions,sharedby);
                }                                   
            }
        }      
    }

    return 0;
}
