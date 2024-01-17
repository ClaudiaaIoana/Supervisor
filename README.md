![Image Alt text](/logo.png "Diagram")

**Dodot Andrei & Dascalescu Claudia**  
*C113E, Academia Tehnica Militara "Ferdinand I"*

## Table of Contents

1. [Introduction](#introduction)
   - 1.1 [Project Purpose](#project-purpose)
   - 1.2 [Key Definitions](#key-definitions)
2. [General Software Product Description](#general-software-product-description)
   - 2.1 [Software Overview](#software-overview)
   - 2.2 [HW/SW Platform Details](#platform-details)
   - 2.3 [Constraints](#constraints)
3. [Detailed Specific Requirements](#detailed-specific-requirements)
   - 3.1 [Functional Requirements](#functional-requirements)
   - 3.2 [Non-functional Requirements](#non-functional-requirements)
4. [Key Implementation Points](#key-implementation-points)
   - 4.1 [Fsup](#fsup)
   - 4.2 [Backsup](#backsup)
   - 4.3 [Manager](#manager)
5. [Architecture](#architecture)
   - 5.1 [Diagram](#diagram)

## Introduction

### 1.1 Project Purpose
The SuperManager project aims to develop a transparent Linux process management and monitoring application, facilitating both local and remote user interactions to ensure ease of use.

### 1.2 Key Definitions
- **fsup**: Command-line interface program for user interaction.
- **backsup**: systemd service managing processes, a vital component of Supervisor.
- **POSIX**: IEEE computer standards for OS compatibility.
- **Client**: Entity using Supervisor to manage and monitor processes.
- **Server (Manager)**: Program enabling remote management and monitoring.

## General Software Product Description

### 2.1 Software Overview
Supervisor comprises two main entities:
- **Server (Manager)**: A remote process manager with advanced control and configuration features.
- **Client (Supervisor)**: Runs on a station, executing programs based on specified criteria. Comprises `fsup` and `Backsup` components.

### 2.2 HW/SW Platform Details
- Linux-compatible OS
- POSIX standards adherence
- C Standard Library for portable code
- pthread.h for thread management
- fcntl library for file manipulation

### 2.3 Constraints
- **fsup Constraints**:
  - Runs with current user permissions.
- **backsup Constraints**:
  - Requires a correct config file for proper execution.
  - Must run with root permissions for process management.
- **manager Constraints**:
  - Dependent on the existence of connected clients.

### Detailed Specific Requirements

#### 3.1 Functional Requirements
- **User Communication**:
  - `fsup <program> <args>`: Launches processes with a specified executable path and arguments.
  - `fsup <options> <pid>`: Performs operations on a process using options like "kill."

- **Manager (Server)**:
  - Manages clients and the processes they launch.
  - Decision-making for process control and file modification.

- **Operation Logging**:
  - `Backsup` logs all actions for tracking and analysis.
  - `Manager` maintains its own journal, helpful for the administrator.

#### 3.2 Non-functional Requirements
- **Error Handling Mechanism**: Robust error handling for unforeseen situations.
- **Local User Interaction**: `fsup` locates and searches for config files, providing proper error messages.
- **Communication**: UnixSocket-based communication between `fsup` and `backsup`.
  InetSocket-based communication between `backsup` and `Manager`.
- **Config File Processing**: `Backsup` processes config files, ensuring conditions for program execution.
- **Manager-Supervisor Communication**: Using JSON to simplify the parse process.

## Key Implementation Points

### 4.1 Fsup
- **Single-threaded**: Uses a single thread to connect to backsup and send a request.

### 4.2 Backsup
- **Multithreaded**:
  - A thread listens for incoming connections from fsup.
  - For every connection, a thread is created to handle the request, be it the launch of a process and waiting for it to end or a list of pid redirected in the fsups terminal.
  - A thread handles the connection with the Manager.
- **Fsup Wait**:
  - Backsup has the responsibility to make the fsup enter a waiting state while it handles the request.
  - After it makes sure the fsup has no more input to get, it lets the process rerun its course and end the execution normally.
- **Settings Structure**:
  - A specially defined structure that gradually updates the parameters for executing the program in the desirable way.
  - If the .conf file does not provide enough details, the default behavior is to take the basic parameters of the .fsup (owner, gowner, stdin, stdout, stderr).
- **Process List**:
  - A list created to keep track of the processes currently running under it and their status.
  - It is updated at every create_process and every time a child process ends its execution.
- **Log**:
  - Made with some thread-safe implemented functions that write in a local file "activity.log".

### 4.3 Manager
- **Multithreading**: Handles multiple connected clients.
- **Facile Communication**: Uses .json to send and receive data from backsup.

## Architecture

### 5.1 Diagram
![Image Alt text](/Diagram.png "Diagram"))


