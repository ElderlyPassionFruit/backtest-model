**GaraCapital homework**

1. For installing:

	1.1. mkdir build

	1.2. cd build

	1.3. cmake ..

	1.4. make

2. All binary files will be in the bin folder
3. Main class - BackTests
4. All unit tests are in the file Application/unit_tests.cpp, compiled into the binary file bin/unit-tests

**Explanation how my model works**

I had quite a few attempts to use some one parameter: various averages, percentiles, and so on, however, they almost did not beat the random (which brings a small profit, since the data is for a small period of time, and market grows on this period). 

Therefore, I decided to try to combine several maximally simple signs (they look at the market volume recently from different points of view) into 1 powerful one (in a sense, I stole the idea of catboost). The final solution is in the file Application/main.cpp, is compiled into a binary file bin/hft-simulator (run it, it outputs a certain amount of logs about the operation of the model).