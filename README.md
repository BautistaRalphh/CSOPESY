# CSOPESY - Process Multiplexer and CLI

Welcome to the CSOPESY project! This repository contains the code for a process multiplexer and CLI built using C++. 

## Members
- Bautista, Ralph Gabriel 
- Encarguez II, Jorenie
- Gan, Austin Philippe
- Gutierrez, Allen Andrei

## Features

* **Main Command Recognition:** The Main Console Accepts and acknowledges the following commands:
    * `initialize`: Not yet implemented
    * `screen -s <name>`: Creates a new process and puts the user in the process console.
    * `screen -r <name>`: Redirects the user to the process they want to go to.
    * `scheduler-test`: Not yet implemented
    * `scheduler-stop`: Not yet implemented
    * `report-util`: Not yet implemented 
    * `clear`: Clears the terminal screen and reprints the header.
    * `exit`: Closes the command-line emulator.

 **Process Command Recognition:** The Process Console Accepts and acknowledges the following commands:
   * `process -smi`: Clears the process screen and displays the most recent information about that process.
   * `exit`: Redirects the user back to the main console.