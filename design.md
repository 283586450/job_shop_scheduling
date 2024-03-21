# target
Solve Job Shop Problem with multi-algorithms 

# language： C++23

# design
1. When start the program, and pass the command line args, there are two option:
    - specify the scale (job number, machine number), the program will generate the test data and create a test instance.
    - specify the data file, the program will load the test data, and create a test instance .
    - some other parameters could be specified: num_thread, solving time,
    - the instance obj cannot be moved, it can only be copied.

2. After the instance is generated, a number of algorithm is launched:
    - A construct algorithm is used to generate the initial feasible solutions (for other algorithms)

3. There are 3 format of solution in this program:
    - S0: a list of each task, where each task has its start/end time.
    - S1: a vector of task for each machine. 
    - S2: a disjunctive graph to represent the solution.
    - S3: a chromosome to be used in GA and other search algorithms. 

    S2, and S3 could translate to S1, and we can use S1 to get S0;

4. We have the fowlling algrithms to be used in the program:
    - GRASP construct: generate init feasible solution (be called when other algrithm needed)
    - GRASP local search: use S2 to perform local search (2 threads)
    - LNS: use S1 to perfrom LNS with different dispatching rules, or with NEH algorithm. (4 threads)
    - GA + PSO: use S3 to perfrom GA. (4 threads)
    - Tabu Search: use S2 to perform Tabu Search(1 thread)
    - CP-SAT solver: create a cp-sat model and solve it with cp-sat solver(8 threads)

5. Each algorithm are Multi-threads safe, and:
    - they store a list of local best solutions, they could be updated by each threads of the algorithm.
    - and the porgram store a global best solution, can be updated by each thread of each algorithm.

根据上面的描述，请给我一个整体的设计，包括怎么构建这个软件，怎么设计多线程安全，应该用什么样的设计模式，数据的存储，复制，