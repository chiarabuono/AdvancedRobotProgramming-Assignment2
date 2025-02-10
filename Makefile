CC = gcc
CXX = g++
AR = ar
CFLAGS = -I./include -I./src/Generated -I/usr/include/fastdds -I/usr/include/fastcdr -I./src -Wall -Wextra -Wpedantic -g
CXXFLAGS = -I./include -I./src/Generated -I/usr/include/fastdds -I/usr/include/fastcdr -I./src -Wall -Wextra -Wpedantic -g -std=c++11
LDFLAGS = -L$(BUILD_DIR)
LIBS = -lauxfunc -ltargsubscriber -ltargpublisher -lobstsubscriber -lobstpublisher -lncurses -ltinfo -lm -lcjson -lstdc++ -lfastcdr -lfastdds

# Directory
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build
LOG_DIR = log

# Source and object files
SRC_FILES_C = $(wildcard $(SRC_DIR)/*.c)
SRC_FILES_CPP = $(wildcard $(SRC_DIR)/*.cpp)
AUX_OBJ = $(BUILD_DIR)/auxfunc.o
TARG_SUBSCRIBER_OBJS = $(BUILD_DIR)/targ_subscriber.o
TARG_PUBLISHER_OBJS = $(BUILD_DIR)/targ_publisher.o
OBST_SUBSCRIBER_OBJS = $(BUILD_DIR)/obst_subscriber.o
OBST_PUBLISHER_OBJS = $(BUILD_DIR)/obst_publisher.o
OBJS_C = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter-out $(SRC_DIR)/auxfunc.c,$(SRC_FILES_C)))
OBJS_CPP = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter-out $(SRC_DIR)/targ_subscriber.cpp $(SRC_DIR)/targ_publisher.cpp $(SRC_DIR)/obst_subscriber.cpp $(SRC_DIR)/obst_publisher.cpp,$(SRC_FILES_CPP)))

# Static libraries
LIBAUX = $(BUILD_DIR)/libauxfunc.a
LIBTARG_SUBSCRIBER = $(BUILD_DIR)/libtargsubscriber.a
LIBTARG_PUBLISHER = $(BUILD_DIR)/libtargpublisher.a
LIBOBST_SUBSCRIBER = $(BUILD_DIR)/libobstsubscriber.a
LIBOBST_PUBLISHER = $(BUILD_DIR)/libobstpublisher.a

# Executables
EXECUTABLES = $(BIN_DIR)/main $(BIN_DIR)/drone $(BIN_DIR)/input $(BIN_DIR)/watchdog $(BIN_DIR)/blackBoard $(BIN_DIR)/target $(BIN_DIR)/obstacle

# Default target
all: directories $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER) $(EXECUTABLES)

# Compile static libraries
$(LIBAUX): $(AUX_OBJ)
	$(AR) rcs $@ $^

$(LIBTARG_SUBSCRIBER): $(TARG_SUBSCRIBER_OBJS)
	$(AR) rcs $@ $^

$(LIBTARG_PUBLISHER): $(TARG_PUBLISHER_OBJS)
	$(AR) rcs $@ $^

$(LIBOBST_SUBSCRIBER): $(OBST_SUBSCRIBER_OBJS)
	$(AR) rcs $@ $^

$(LIBOBST_PUBLISHER): $(OBST_PUBLISHER_OBJS)
	$(AR) rcs $@ $^

# Compile executables
$(BIN_DIR)/main: $(BUILD_DIR)/main.o $(OBJS_C) $(OBJS_CPP) $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/drone: $(BUILD_DIR)/drone.o $(OBJS_C) $(OBJS_CPP) $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/input: $(BUILD_DIR)/input.o $(OBJS_C) $(OBJS_CPP) $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/watchdog: $(BUILD_DIR)/watchdog.o $(OBJS_C) $(OBJS_CPP) $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/blackBoard: $(BUILD_DIR)/blackBoard.o $(OBJS_C) $(OBJS_CPP) $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/target: $(BUILD_DIR)/target.o $(OBJS_C) $(OBJS_CPP) $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BIN_DIR)/obstacle: $(BUILD_DIR)/obstacle.o $(OBJS_C) $(OBJS_CPP) $(LIBAUX) $(LIBTARG_SUBSCRIBER) $(LIBTARG_PUBLISHER) $(LIBOBST_SUBSCRIBER) $(LIBOBST_PUBLISHER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

# Compile object files (including library files)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Manage directories
.PHONY: directories clean clean-logs
directories:
	mkdir -p $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR)
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log

# Clean logs only
clean-logs:
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log

# Clean all
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR)
