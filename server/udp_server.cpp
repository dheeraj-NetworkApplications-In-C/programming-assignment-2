#include<stdio.h>
#include <iostream>
#include<stdlib.h>
#include<cstring>
#include<dirent.h>
#include<vector>
#include<fstream>
#include<unistd.h>

#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<map>

using namespace std;


# define BUFFER_SIZE 1400 // safe UDP packet size

struct FilePacket{
    /* data */
    int packet_id;
    int total_packets;
    int data_size;
    char data[BUFFER_SIZE - sizeof(int)*3]; // each data size has to be reduced by 3 ints because there are 3 integers that are also sent in with the data packet
};


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

// FilePacket fileSendCheck(string filename){
//     std::ifstream file(filename, std::ios::binary);

//     if(!file.is_open()){
//         std::cout << "Error : Cannot open the file " << filename << endl;

//         // building error packet and returning it 
//     }

//     file.close();
// }


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
    map<string, string> commandMap;
    while (true) {
        std::memset(buffer, 0, sizeof(buffer)); 
        cout << endl;
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
            message = '\0';
            break;
        }
        
        if(strcmp(buffer, "ls")==0) {
            // creating a new remoteAddr (oh wait you already have it in buffer damn )
            message = getfilesCWD();
            cout << "Remote asked for files listing" << endl;
            sendto(sockfd, message.c_str(), message.size()+1, 0, (struct sockaddr*)&remoteAddr, addr_size);
            message='\0';
        }

        string buffer_string(buffer);

        // Am i good at naming or something :)
        commandMap = getCommandMap(buffer_string);

        string fileName = commandMap["arg"];

        if (commandMap["command"]=="get"){
            std::cout << "Sending file " << fileName << endl;
            
            int max_data_size = sizeof(FilePacket::data); // refer the struct above 

            std::ifstream file(fileName, std::ios::binary);

            // getting the file size 

            // going back and forth to cal the total size of the file 
            file.seekg(0, std::ios::end);
            int file_size = file.tellg();
            file.seekg(0, std::ios::beg);

            // cal the number of packets needed 
            // celieing fit formulae, just ensures you will use just enough packets 
            int total_packets = (file_size + max_data_size -1)/max_data_size;

            std::cout << "Sending file :" << fileName << "file size :" << file_size << " bytes";
            std::cout << " Total packets :" << total_packets;
            
            // FIXME: try again with end of file thing, could be a bit faster :)
            // if(!file.eof()){
            //     FilePacket packet;
            //     packet.packet_id = packet_id++;
            //     // reading data 
            //     file.read(packet.data, max_data_size);
            //     packet.data_size = file.gcount();
                
            // }

            for(int packet_id=0;packet_id < total_packets; packet_id++) {
                FilePacket packet;
                packet.packet_id = packet_id;
                packet.total_packets = total_packets;

                // read data for this packet 
                file.read(packet.data, max_data_size);
                packet.data_size = file.gcount(); // get the actual size of data read

                // sending the packet
                int bytes_sent = sendto(sockfd, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&remoteAddr, addr_size);
                if(bytes_sent <0){
                    cerr << "Error : sending packet id " << packet_id << endl;
                    // try sending again 
                    packet_id--;
                    continue;
                }
                std::cout << "Sent packet id " << packet_id << " with size " << bytes_sent << " bytes" << endl;
                usleep(1000); // sleep for 1 ms to avoid flooding the network
            }
            file.close();
            std::cout << "File transfer completed for file " << fileName << endl;
            message = '\0';

        }


    }
    return 0;

}