CXX      ?= c++
CRITERION := $(shell brew --prefix criterion 2>/dev/null)
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2
CXXFLAGS += -I$(CRITERION)/include
LDFLAGS  += -L$(CRITERION)/lib
LDLIBS   := -lcriterion

TEST_BIN := vle_test
TEST_SRC := vle_test.cc

.PHONY: test clean

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) vle.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(TEST_SRC) $(LDLIBS)

clean:
	$(RM) $(TEST_BIN)
