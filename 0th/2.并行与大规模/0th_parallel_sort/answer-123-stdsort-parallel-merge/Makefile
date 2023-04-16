CXX = g++
CXXFLAGS = -fopenmp -D_GLIBCXX_PARALLEL -lm -O3 -static -std=c++20 -Wall
#CXXFLAGS = -fopenmp -pedantic -lm -ltbb -O3 -std=c++20 -Wall
	
answer: answer.cpp
	$(CXX) $< $(CXXFLAGS) -o $@