test-write:
	sysbench memory --memory-block-size=2G --memory-total-size=1000G --memory-oper=write --threads=12 run

test-read:
	sysbench memory --memory-block-size=2G --memory-total-size=1000G --memory-oper=read --threads=12 run

example-%: main%.cpp
	$(CXX) -o $@ $^ -Wall -Wextra -O0 -MMD

opt-example-%: main%.cpp
	$(CXX) -o $@ $^ -Wall -Wextra -O2 -MMD

omp-example-%: main%.cpp
	$(CXX) -o $@ $^ -Wall -Wextra -O0 -MMD -fopenmp

opt-omp-example-%: main%.cpp
	$(CXX) -o $@ $^ -Wall -Wextra -O2 -MMD -fopenmp

clear:
	$(RM) -rf example-* opt-example-* omp-example-* opt-omp-example-*
