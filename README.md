# FSM-aware-compression
A simple implementation of FSM-aware data acquisition and storage.



- input file path in /script/run.py, `INPUT_DIR_PATH`

- The command to run is as follows, Configure your `make`

   - ```shell
     mingw32-make run
     ```

- file tree

  - ```shell
    |-input   # temp file
    |-script
    	|-run.py
    |-src
    	|-Huffman.c
    	|-Huffman.h
    	|-main.c
    	|-main.h
    Makefile
    |-result  # generate when make run
    	|-{now_time}_result.xlsx		  # all statistics
    	|-{now_time}_result_avg_final.xlsx # average with same bit and size in statistics
    	|-{now_time}.txt         		  # main.exe output	
    	|-{now_time}_short.txt	           # main.exe reduced output
    ```

    
    

