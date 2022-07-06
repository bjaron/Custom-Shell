//Jaron Baldazo
//Feb 12 2022
//compile: g++ shell.cpp -o shell
//run: ./shell

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>

using namespace std;

#define MAX_BUFFER 1024

int main (int argc, char* argv[]){
    cout << "To exit shell, enter 'quit' or 'exit'" << endl;
    //infinite loop
    while (1) {
        char command [MAX_BUFFER];          //store user input
        cin.getline(command, MAX_BUFFER);   //take in user input
        vector <string> commandVector;      //store commands separated by ' '
        string commandString(command);      //for system calls

        //check if user wants to exit shell
        if (commandString.compare("exit") == 0 || commandString.compare("quit") == 0){
            cout << "Exiting shell" << endl;
            exit(1);
        }

        //check if user puts invalid pipe or redirection
        else if (commandString.front() == '|' || commandString.front() == '<' || commandString.front() == '>' ||
            commandString.back() == '|' || commandString.back() == '<' || commandString.back() == '>'){
                cout << "Invalid usage of pipe or redirection. Try again." << endl;
        }

        else if (commandString.find("||") != string::npos || commandString.find("|<") != string::npos || 
            commandString.find("|>|") != string::npos || commandString.find("<|") != string::npos || 
            commandString.find("<>") != string::npos || commandString.find(">|") != string::npos ||
            commandString.find("><") != string::npos){
                cout << "Invalid usage of pipe and redirection. Try again" << endl;
        }


        //questions 1 and 2, if command doesnt contain $ or end with &
        else if (strchr(command, '$') == NULL && commandString.back() != '&'){
            system(commandString.c_str());
        }

        //questions 3 and onward
        else {
            //separate commands into vector
            char* splitByToken = strtok(command, " ");
            while (splitByToken != NULL){
                commandVector.push_back(splitByToken);
                splitByToken = strtok (NULL, " ");
            }

            // for (int i = 0; i < commandVector.size(); i++){
            //     cout << commandVector[i] << endl;
            //     cout << commandVector[i].length() << endl;
            // }

            //question 4, background processes
            if (commandVector[commandVector.size() - 1] == "&"){
                // cout << commandString.c_str() << endl;
                string removedAmpersand = commandString.substr(0, commandString.size() - 1);
                // cout << removedAmpersand << endl;

                pid_t pid = fork();
                
                //error in fork
                if (pid == -1){
                    perror("Error in first fork\n");
                    exit(EXIT_FAILURE);
                }

                //run command into child process
                else if (pid == 0){
                    system(removedAmpersand.c_str());
                    exit(1);
                }
                else {
                    continue;
                }
            }
            
            //command contains special pipe $
            else {
                int pipeFlag = 0;   //flag for where the special pipe is

                //find special pipe
                for (int i = 0; i < commandVector.size(); i++){
                    if (commandVector[i].compare("$") == 0){
                        pipeFlag = i;
                        break;
                    }
                }

                //pipe at the front with no command
                if (pipeFlag == 0){
                    cout << "Error in special pipe ($), try again." << endl;
                    continue;
                }
                
                // cmd1 $ cmd2 cmd3
                else if (pipeFlag == 1 && commandVector.size() == 4) {

                    //loop to run cmd1 | cmd2 and cmd1 | cmd3
                    for (int i = 0; i < 2; i++){
                        //pipe
                        int fd[2];
                        pipe(fd);

                        pid_t pid = fork();

                        //error in fork
                        if (pid == -1){
                            perror("Error in first fork\n");
                            exit(EXIT_FAILURE);
                        }
                        //child1
                        else if (pid == 0){
                            dup2(fd[0], STDIN_FILENO);  //redirect stdin to pipe
                            close(fd[0]);
                            close(fd[1]);

                            //setup for execvp
                            char rightCommand [MAX_BUFFER];
                            strcpy (rightCommand, commandVector[i+2].c_str());  //cmd2 or cmd3
                            char* args[] = {rightCommand, NULL};

                            //execute command
                            execvp(rightCommand, args);
                            perror("Error in execvp");  //error checking
                            exit(EXIT_FAILURE);
                        }
                        //parent1
                        else {
                            pid_t pid2 = fork();

                            //error in fork
                            if (pid2 == -1){
                                perror("Error in second fork\n");
                                exit(EXIT_FAILURE);
                            }
                            //child2
                            else if (pid2 == 0){
                                dup2(fd[1], STDOUT_FILENO); //redirect stdout to pipe
                                close(fd[0]);
                                close(fd[1]);

                                //setup for execvp
                                char cmd1 [MAX_BUFFER];
                                strcpy (cmd1, commandVector[0].c_str());    //cmd1
                                char* args[] = {cmd1, NULL};

                                //execute command
                                execvp(cmd1, args);
                                perror("Error in execvp");  //error checking
                                exit(EXIT_FAILURE);
                            }
                            //parent2
                            else {
                                close(fd[0]);
                                close(fd[1]);
                                waitpid(pid2, NULL, 0); //wait for second child
                            }
                            close(fd[0]);
                            close(fd[1]);
                            waitpid(pid, NULL, 0);  //wait for first child
                        }
                    }
                }

                //cmd1 cmd2 $ cmd3
                else if (pipeFlag == 2 && commandVector.size() == 4){
                    FILE* tempFile = fopen("tempFile.txt", "w+");   //temp file to store cmd1 and cmd2

                    pid_t pid = fork();

                    //error in fork
                    if (pid == -1){
                        perror("Error in first fork\n");
                        exit(EXIT_FAILURE);
                    }
                    //child1
                    else if (pid == 0){
                        dup2(fileno(tempFile), STDOUT_FILENO);  //stdout into file

                        //setup for execvp
                        char leftCommand [MAX_BUFFER];
                        strcpy (leftCommand, commandVector[0].c_str()); //cmd1
                        char* args[] = {leftCommand, NULL};

                        //execute command
                        execvp(leftCommand, args);
                        perror("Error in execvp");  //error checking
                        exit(EXIT_FAILURE);
                    }
                    //parent1
                    else {
                        waitpid(pid, NULL, 0);  //wait for child1
                        
                        pid_t pid2 = fork();

                        //error in fork
                        if (pid2 == -1){
                            perror("Error in second fork\n");
                            exit(EXIT_FAILURE);
                        }
                        //child2
                        else if (pid2 == 0){
                            dup2(fileno(tempFile), STDOUT_FILENO);  //stdout into file

                            //setup for execvp
                            char leftCommand [MAX_BUFFER];
                            strcpy (leftCommand, commandVector[1].c_str()); //cmd2
                            char* args[] = {leftCommand, NULL};

                            //execute command
                            execvp(leftCommand, args);
                            perror("Error in execvp");  //error checking
                            exit(EXIT_FAILURE);
                        }
                        //parent2
                        else {
                            waitpid(pid2, NULL, 0); //wait for child2
                            pid_t pid3 = fork();
                            
                            //error in fork
                            if (pid3 == -1){
                                perror("Error in third fork\n");
                                exit(EXIT_FAILURE);
                            }
                            //child3
                            else if (pid3 == 0){
                                FILE* read = fopen("tempFile.txt", "r"); //open file for reading

                                dup2(fileno(read), STDIN_FILENO);   //stdin into file

                                //setup for execvp
                                char rightCommand [MAX_BUFFER];
                                strcpy (rightCommand, commandVector[3].c_str());    //cmd3
                                char* args[] = {rightCommand, NULL};

                                //execute command
                                execvp(rightCommand, args);
                                perror("Error in execvp");  //error checking
                                exit(EXIT_FAILURE);
                            }
                            //parent3
                            else {
                                waitpid(pid3, NULL, 0); //wait for child3
                            }
                        }
                    }
                    fclose(tempFile);
                    remove("tempFile.txt");
                }

                //cmd1 cmd2 $ cmd3 cmd4
                else if (pipeFlag == 2 && commandVector.size() == 5){
                    
                    //run twice for cmd3 and cmd4
                    for (int i = 0; i < 2; i++){
                        FILE* tempFile = fopen("tempFile.txt", "w+"); //temp file to store cmd1 and cmd2
                        pid_t pid = fork();

                        //error in fork
                        if (pid == -1){
                            perror("Error in first fork\n");
                            exit(EXIT_FAILURE);
                        }
                        //child1
                        else if (pid == 0){
                            dup2(fileno(tempFile), STDOUT_FILENO);  //stdout into file
                            
                            //setup for execvp
                            char leftCommand [MAX_BUFFER];
                            strcpy (leftCommand, commandVector[0].c_str()); //cmd1
                            char* args[] = {leftCommand, NULL};

                            //execute command
                            execvp(leftCommand, args);
                            perror("Error in execvp");  //error checking
                            exit(EXIT_FAILURE);
                        }
                        //parent1
                        else {
                            waitpid(pid, NULL, 0);  //wait for child1
                    
                            pid_t pid2 = fork();

                            //error in fork
                            if (pid2 == -1){
                                perror("Error in second fork\n");
                                exit(EXIT_FAILURE);
                            }
                            //child2
                            else if (pid2 == 0){
                                dup2(fileno(tempFile), STDOUT_FILENO);  //stdout into file

                                //setup for execvp
                                char leftCommand [MAX_BUFFER];          
                                strcpy (leftCommand, commandVector[1].c_str()); //cmd2
                                char* args[] = {leftCommand, NULL};

                                //execute command
                                execvp(leftCommand, args);
                                perror("Error in execvp");  //error checking
                                exit(EXIT_FAILURE);
                            }
                            //parent2
                            else {
                                waitpid(pid2, NULL, 0); //wait for child2

                                pid_t pid3 = fork();
                                
                                //error in fork
                                if (pid3 == -1){
                                    perror("Error in third fork\n");
                                    exit(EXIT_FAILURE);
                                }
                                //child3
                                else if (pid3 == 0){
                                    FILE* read = fopen("tempFile.txt", "r");    //open for read only
                                    dup2(fileno(read), STDIN_FILENO);   //stdin into file

                                    //setup for execvp
                                    char rightCommand [MAX_BUFFER];
                                    strcpy (rightCommand, commandVector[i+3].c_str());  //cmd3 or cmd4
                                    char* args[] = {rightCommand, NULL};

                                    //execute command
                                    execvp(rightCommand, args);
                                    perror("Error in execvp");
                                    exit(EXIT_FAILURE);
                                }
                                //parent3
                                else {
                                    waitpid(pid3, NULL, 0); //wait for child3
                                }
                            }
                        }
                        fclose(tempFile);   //close file
                        remove("tempFile.txt"); //delete temp file
                    }
                }
                //invalid $ usage
                else {
                    cout << "Invalid usage of $ pipe. Try again." << endl;
                }
            }
        }
    }
    return 0;
}