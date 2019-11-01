# Load local config, if it exists (needs to be the first command in the makefile)
SCRIPT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
# To configure this Makefile, define variables in Makefile.local in the same directory
-include $(SCRIPT_DIR)/Makefile.local

########################################

DEBUG ?= 0   # Set to 0 if unset
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g3 -DDEBUG
    LDFLAGS += -g
else
    CXXFLAGS += -DNDEBUG -O3 -std=c++0x -march=native -mtune=native
endif


ifneq ($(USE_LOGGING),)
        CXXFLAGS+=-D USE_LOGGING
endif

# Set to 0 if not overwritten
USE_CRAY ?= 0

ifeq ($(USE_CRAY), 1)
    COMPILER                    =       CC
    CXXFLAGS+=-D USE_FOMPI

    ifeq ($(FOMPI_LOCATION),)
        $(error FOMPI_LOCATION is not specified)
    endif

   CXXFLAGS+=-I $(FOMPI_LOCATION) -I /opt/cray/dmapp/default/include/ -I $(FOMPI_LOCATION)/libtopodisc -I $(FOMPI_LOCATION)/mpitypes/install/include  -DXPMEM -DUGNI
   FOMPI_LIBS                   =  -L $(FOMPI_LOCATION) -lfompi -L $(FOMPI_LOCATION)/mpitypes/install/lib -lmpitypes  -L /opt/cray/dmapp/default/include/lib64 -ldmapp -DXPMEM -DUGNI
   LDFLAGS+=$(FOMPI_LIBS)
else
    COMPILER                    =       mpic++
endif

BOOSTLD=
ifneq ($(BOOST_LIB),)
        BOOSTLD+=-Wl,-rpath=$(BOOST_LIB) -L ${BOOST_LIB}
endif

BOOSTLD+=-lboost_program_options -lboost_serialization


########################################

SOURCE_FILES		= 	src/hdb/main.cpp \
						src/hdb/locktable/LockTableAgent.cpp \
						src/hdb/transactions/TransactionAgent.cpp \
						src/hdb/communication/VoteCommunicator.cpp \
						src/hdb/communication/DataLayer.cpp \
						src/hdb/communication/Communicator.cpp \
						src/hdb/communication/NotifiedCommunicator.cpp \
						src/hdb/communication/SendReceiveCommunicator.cpp \
						src/hdb/communication/HybridCommunicator.cpp \
						src/hdb/messages/MessageSlot.cpp \
						src/hdb/messages/Message.cpp \
						src/hdb/messages/LockRequest.cpp \
						src/hdb/messages/LockGrant.cpp \
						src/hdb/messages/LockRelease.cpp \
						src/hdb/messages/VoteRequest.cpp \
						src/hdb/messages/TransactionEnd.cpp \
						src/hdb/messages/AbortTransaction.cpp \
						src/hdb/messages/Shutdown.cpp \
						src/hdb/locktable/Lock.cpp \
						src/hdb/locktable/Table.cpp \
						src/hdb/transactions/Transaction.cpp \
						src/hdb/transactions/TransactionManager.cpp \
						src/hdb/transactions/SimpleClient.cpp \
						src/hdb/transactions/ReplayClient.cpp \
						src/hdb/stats/ClientStats.cpp

HEADER_FILES		= 	src/hdb/configuration/SystemConfig.h \
						src/hdb/locktable/LockTableAgent.h \
						src/hdb/transactions/TransactionAgent.h \
						src/hdb/communication/VoteCommunicator.h \
						src/hdb/communication/DataLayer.h \
						src/hdb/communication/NotifiedCommunicator.h \
						src/hdb/communication/SendReceiveCommunicator.h \
						src/hdb/communication/Communicator.h \
						src/hdb/communication/HybridCommunicator.h \
						src/hdb/messages/MessageUtils.h \
						src/hdb/messages/MessageSlot.h \
						src/hdb/messages/Message.h \
						src/hdb/messages/LockRequest.h \
						src/hdb/messages/LockGrant.h \
						src/hdb/messages/LockRelease.h \
						src/hdb/messages/VoteRequest.h \
						src/hdb/messages/TransactionEnd.h \
						src/hdb/messages/AbortTransaction.h \
						src/hdb/messages/Shutdown.h \
						src/hdb/locktable/Lock.h \
						src/hdb/locktable/Table.h \
						src/hdb/transactions/Transaction.h \
						src/hdb/transactions/TransactionManager.h \
						src/hdb/transactions/SimpleClient.h \
						src/hdb/transactions/ReplayClient.h \
						src/hdb/stats/ClientStats.h \
						src/hdb/utils/Debug.h
				
########################################

PROJECT_NAME		= 	hdb-lock

########################################

SOURCE_FOLDER		= 	src
BUILD_FOLDER		= 	build
RELEASE_FOLDER		= 	release

########################################
			
OBJECT_FILES		= 	$(patsubst $(SOURCE_FOLDER)/%.cpp,$(BUILD_FOLDER)/%.o,$(SOURCE_FILES))
SOURCE_DIRECTORIES	= 	$(dir $(HEADER_FILES))
BUILD_DIRECTORIES	= 	$(patsubst $(SOURCE_FOLDER)/%,$(BUILD_FOLDER)/%,$(SOURCE_DIRECTORIES))

########################################

all: program preprocess

########################################

$(BUILD_FOLDER)/%.o:  $(SOURCE_FILES) $(HEADER_FILES)
	mkdir -p $(BUILD_FOLDER)
	mkdir -p $(BUILD_DIRECTORIES)
	$(COMPILER) $(CXXFLAGS) $(BOOST_INCLUDE) -c $(SOURCE_FOLDER)/$*.cpp -I $(SOURCE_FOLDER) $(LDFLAGS) -o $(BUILD_FOLDER)/$*.o

########################################

program: $(OBJECT_FILES)
	mkdir -p $(RELEASE_FOLDER)
	$(COMPILER) $(OBJECT_FILES) $(CXXFLAGS)  $(BOOST_INCLUDE)  $(LDFLAGS) $(BOOSTLD) -o $(RELEASE_FOLDER)/$(PROJECT_NAME)

#######################################


preprocess:
	mkdir -p $(RELEASE_FOLDER)
	$(COMPILER) src/hdb/preprocess.cpp src/hdb/transactions/Transaction.cpp -DNDEBUG -O3 -std=c++0x -march=native -mtune=native -I src/ $(BOOST_INCLUDE) $(BOOSTLD) -o $(RELEASE_FOLDER)/preprocess

########################################
clean:
	rm -rf $(BUILD_FOLDER)
	rm -rf $(RELEASE_FOLDER)

########################################

