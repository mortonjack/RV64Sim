# RV64Sim

A 64-bit RISC-V simulator for my ELEC ENG Computer Architecture course at the University of Adelaide. This assignment was split into two parts, non-privileged and privileged architecture. I recieved 970/1000 marks for the non-privileged architecture part (Losing 30 marks for efficiency) and 1000/1000 marks for the privileged architecture part.

See tests for examples of capabilities. I implemeneted processor.cpp and memory.cpp, commands.cpp and rv64sim.cpp were given to me (although I may have made slight modifications, check commit history on them). If you have any questions, feel free to reach out to me, I kept this repository private whilst the course was active and didn't intend to make it public. 

# Test
Go to any of the provided test folders and run ./run_all_tests, it will output a file containing the diff between expected and actual output. This should be almost entirely empty, except the instruction tests (there was some undefined behaviour allowable for part 1). 
