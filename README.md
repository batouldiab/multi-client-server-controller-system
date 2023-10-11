# Multi-Client Multi-Server Controller

## Description

This project implements a multi-client multi-server controller system. The system consists of a controller, multiple clients, and a server. Clients send requests to the controller, which manages the incoming requests and distributes them to the server for processing. Once the server processes the requests, the results are sent back to the respective clients.

## Features

- **Dynamic Client Creation:** The number of clients can be dynamically set by the user during runtime.
- **Request Management:** The controller manages incoming client requests and assigns them to the server for processing.
- **Concurrency:** The system supports multiple clients and processes requests concurrently.
- **Error Handling:** Handles various error scenarios such as no available server slots, request timeouts, and more.

## Prerequisites

- **Operating System:** This project is designed for Unix-like operating systems.
- **Compiler:** The code is written in C and requires a C compiler (e.g., GCC).
- **Dependencies:** Standard C libraries are used, no external dependencies required.

## How to Run

1. **Compilation:** Compile the `project.c` file using a C compiler. For example, using GCC:
  ```bash
  gcc -o controller project.c
  ```

2. **Execution:** Run the compiled binary:
  ```bash
  ./controller
  ```

3. **Input:** Enter the number of clients to create when prompted.

## Notes

- The system uses signals for inter-process communication and synchronization.
- Clients generate random requests to simulate real-world scenarios.
- The project utilizes forked processes for implementing clients, controller, and server functionalities.

## Author

[Batoul Diab](https://github.com/batouldiab)

[batouldiab@outlook.com](mailto:batouldiab@outlook.com)


