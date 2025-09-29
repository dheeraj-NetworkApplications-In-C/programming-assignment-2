#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<map>

using namespace std;


// TODO: If you find time, optimise this to return char array than string array or map 
// ( point of using c++ is for performance dont waste it like this :) )
// and make it more fail proof 
std::map<std::string, string> getCommandMap(string input)  {

    map<string, string> commandmap;
    string inputsplits[2];
    int currentIndex = 0;
    string temp;
    for(char x: input){
        if(x == ' '){
                commandmap["command"] = temp;
                temp = "";
                continue;
        }
        temp += x;
    }
    commandmap["arg"] = temp;

    return commandmap;

}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[1024];
    socklen_t addr_size;


    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd <0) {
        cerr << "[-] Socket creation failed" << endl;
        return -1;
    }
    cout << "[+] socket creation successful " << endl;

    std::memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // host of the server

    addr_size = sizeof(serverAddr);

    cout << "send commands to file server " << endl;

    map<string, string> commandMap;

    while (true) {

        // getting the message from the server 
        string msg;
        cout << "file.server> ";
        getline(cin, msg) ;

        // sending the message to server
        sendto(sockfd, msg.c_str(), msg.size()+1, 0, (struct sockaddr*)&serverAddr , addr_size);

        if(msg== "exit"){
            int n = recvfrom(sockfd,buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddr, &addr_size );
            cout << "[+] Exiting client "<< endl;
            break;
        }

        // as this is UDP you can also do MSG_DONTWAIT
        if (msg == "ls"){
            int n = recvfrom(sockfd,buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddr, &addr_size );

            if( n>0) {
                cout << "[+] Server replied :" << buffer << endl;
                memset(buffer, 0, sizeof(buffer));
            }
            continue;
        }

        commandMap.clear();

        commandMap= getCommandMap(msg);

        
        


    }
}