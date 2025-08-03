# CSOPESY - Process Multiplexer and CLI

Welcome to the CSOPESY project! This repository contains the code for a process multiplexer and CLI built using C++. 

## Members
- Bautista, Ralph Gabriel 
- Encarguez II, Jorenie
- Gan, Austin Philippe
- Gutierrez, Allen Andrei

## Features

* **Main Command Recognition:** The Main Console Accepts and acknowledges the following commands:
    * `initialize`: Initializes the console based on the config.txt file.
    * `screen -s <name>`: Creates a new process and puts the user in the process console.
    * `screen -r <name>`: Redirects the user to the process they want to go to.
    * `screen -c <name> "<instructions>"`: Creates a new process with user defined instructions.
    * `scheduler-start`: Starts the scheduling algorithm.
    * `scheduler-stop`: Stops the scheduling algorithm.
    * `report-util`: Generates a report of the process info shown by screen -ls
    * `clear`: Clears the terminal screen and reprints the header.
    * `process-smi`: Shows memory and paging status of processes.
    * `vmstat`: Shows information regarding memory, ticks, and paging.
    * `exit`: Closes the command-line emulator.

 **Process Command Recognition:** The Process Console Accepts and acknowledges the following commands:
   * `process -smi`: Clears the process screen and displays the most recent information about that process.
   * `exit`: Redirects the user back to the main console.
