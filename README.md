This program emulates a simplified file system using actual files to reduce complexity. It supports basic file system operations such as navigating directories and modifying the file system structure. The key features and functionalities include:

#Directory and File Support: 
The program can handle both directories and regular files, allowing users to navigate and modify the file system.
#Command-Line Interface: 
Users interact with the file system through a command-line interface, providing commands to traverse and manipulate the directory structure.
#Initialization and Validation: 
The program initializes by loading an inodes_list file, which contains metadata for the inodes in use, and validates these inodes.
#Commands: 
The program supports commands such as exit, cd <name>, ls, mkdir <name>, and touch <name>, enabling users to perform common file system operations.
#Error Handling: 
The program includes error handling for invalid commands and operations, ensuring robustness and user-friendly interaction.
#Emulation Limits: 
The program is designed to support a maximum of 1024 inodes, and each directory entry has a fixed size, simplifying the file system emulation.
