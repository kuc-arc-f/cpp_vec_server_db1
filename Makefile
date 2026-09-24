CXX      = clang++
CXXFLAGS = -std=c++17 -pthread -I./include
#CXXFLAGS = -Wall -O2 -I./include
LIBS     =  -lsqlite3 -luuid -lspdlog -lfmt
TARGET   = vec_server

.PHONY: all clean install

#clang++ -std=c++11 -pthread server.cpp -o vec_server -lsqlite3 -luuid -lspdlog -lfmt
all: $(TARGET)

$(TARGET): server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)
#	$(CXX) $(CXXFLAGS) -o $@ $<

install: $(TARGET)
	install -m 755 $(TARGET) $(HOME)/.local/bin/

clean:
	rm -f $(TARGET)