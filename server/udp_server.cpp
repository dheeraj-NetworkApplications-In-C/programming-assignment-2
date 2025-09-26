#include<stdio.h>
#include <iostream>
#include<stdlib.h>
#include<cstring>
#include<dirent.h>
#include<vector>

#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

using namespace std;

string getfilesCWD(){
    const char* path = ".";

    DIR* dir = opendir(path);

    if (dir == nullptr){
        cerr<< "Error : couldn't open current working directory " << endl;
        return "\0";
    }

    struct dirent* entry;
    string fileList=" ";

    while((entry = readdir(dir)) != nullptr){
        fileList += entry->d_name;
        fileList+= " ";
    }
    closedir(dir);
    return fileList;
}




int main(){

    // for now for testing I'm defining port manually 
    int port = 8080;
    int sockfd;

    struct sockaddr_in myaddr, remoteAddr;
    char buffer[1024];
    socklen_t addr_size;

    // creating packets , in this case IPV4 -- AF_INET , UDP packet -- SOCK_DGRAM , 0 means default port for specific protocol

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);


    cout << "socket created" << endl;
    // blocking data for memory 
    memset(&myaddr, '\0', sizeof(myaddr));

    // defining the TCP packet 
    // binding your address
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cout << "TCP Packet defined" << endl;

    // binding your socket 
    // socket type, your address and size of your address 

    
    bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr));
    addr_size = sizeof(remoteAddr);

    cout << "Socket binded to TCP packets" << endl;

    // recieving from the socket 

    cout << "Listening on the socket " << endl;
    cout << "Type 'exit' to end the session " << endl;
    string message; // replace with char array ASAP
    while (true) {
        recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&remoteAddr, &addr_size);

        cout<< "[+] Data recieved from the buffer is " << buffer << endl;
        cout << "[+] Remote IP address " << inet_ntoa(remoteAddr.sin_addr) << endl;
        cout << "[+] Remote port " << ntohs(remoteAddr.sin_port) << endl;
        cout << "[+] Address Family " << remoteAddr.sin_family << endl;
        cout << "Clearning the buffer " << endl;
        // for some reason buffer is being used by other things, I have to fill them with zeros
        if ( strcmp(buffer, "exit") == 0){
            message = "exiting the file server bye bye !";
            sendto(sockfd, message.c_str(), message.size()+1, 0, (struct sockaddr*)&remoteAddr, addr_size);
            cout<< "[+] exiting the server bye bye !" << endl;
            break;
        }
        
        if(strcmp(buffer, "ls")==0) {
            // creating a new remoteAddr (oh wait you already have it in buffer damn )
            message = getfilesCWD();
            cout << "Remote asked for files listing" << endl;
            sendto(sockfd, message.c_str(), message.size()+1, 0, (struct sockaddr*)&remoteAddr, addr_size);
            message='\0';
        }

        std::memset(buffer, 0, sizeof(buffer)); 
        cout << endl;

    }
    return 0;

}