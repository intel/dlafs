.PHONY: all clean

SRC_FILES := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SRC_FILES))
TARGETS := hddlspipes

all: $(TARGETS)

$(TARGETS): $(OBJECTS)
	@$(CXX) $^ -o $@ $(LDFLAGS) -Wl,-rpath-link
$(OBJECTS): %.o: %.c
	@$(CXX) $< -o $@ -c $(CXXFLAGS) $(DEFINES)
	
clean:
	@$(RM) *.o  $(TARGETS)