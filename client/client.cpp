#include<iostream>
#include<cstring>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<map>
#include<fstream>
using namespace std;

# define BUFFER_SIZE 1400  // safe UDP packet size 

//TODO: Combine common code in seperate folder but for evaluation it needs to be this way, 
// one evaluation is done, configure it to be good stuff 

struct FilePacket {
    /* data */
    int packet_id;
    int total_packets;
    int data_size;
    char data[BUFFER_SIZE - sizeof(int)*3];

    void clear() noexcept {
        packet_id = -1;
        total_packets = -1;
        data_size = 0;
        std::memset(data, 0, sizeof(data));
    }
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
    std::map<int, FilePacket> recived_packets;


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

        if (commandMap["command"]=="get"){
            // clearning all the data recived before for a fresh start :)
            recived_packets.clear();

            int total_packets = -1;
            int packets_recieved = 0;

            // setting socket time out : common I dont wanna overwelm server for my mistake 
            // (if this works :))
            struct timeval timeout;
            timeout.tv_sec = 10; // seconds 
            timeout.tv_usec = 0; // micro seconds 
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            cout << "requested file : " << commandMap["arg"];
            string filename = commandMap["arg"];
            
            // loop to recv file 
            while (true) {
                FilePacket packet;
                
                ssize_t recieved = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&serverAddr, &addr_size);

                if (recieved < 0) {
                    std::cout << "Timeout or recieved error " << std::endl;
                    break;
                }
                
                // checking for error packet
                if(packet.packet_id == -1) {
                    std::cout << "server error : file not found or inaccessable check server logs" << std::endl;
                    break;
                }

                // setting total packets from the first packet 
                if(total_packets == -1) {
                    total_packets = packet.total_packets;
                    std::cout << "expecting :" << total_packets << " packets" << endl;
                }
                
                // check if this is a new packet 
                // TODO: This can be done efficiently for now lets make this work 
                // why this ?
                /* coz later when you add ack, that ack packet might be lost right, 
                hence we need to check its recived or not and I'm not sure at this point,
                should I send ack here as this means ack didnt reach the server before ? 
                lets see.
                */
                if(recived_packets.find(packet.packet_id) == recived_packets.end()) {
                    recived_packets[packet.packet_id] = packet;
                    packets_recieved ++;
                    cout << "packet recieved :" << packet.packet_id << " current status :" 
                    << packets_recieved << " /" << total_packets << endl;
                }

                // sending ack 
                // TODO: This is gonna be next version ( forgot to handle this on server side )
                int ack = packet.packet_id;
                //sendto(sockfd, &ack, sizeof(ack), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr));

                // checking for all packets 
                if(packets_recieved == total_packets) {
                    cout << "all packets recived, writing file ...." << endl;
                    break;
                }
                packet.clear();
            }

            // making sure its not the error thing 
            if (packets_recieved == total_packets) {
                std::ofstream output_file(filename, std::ios::binary);
                if(!output_file.is_open()){
                    std::cout << "error : cannot create output file " << endl;
                    return false;
                }

                /*
                this definetly can be done better, than map use a better data structure
                for now lets again make it work 
                */

                for(int i=0; i<total_packets; i++) {
                    if(recived_packets.find(i) != recived_packets.end()){
                        output_file.write(recived_packets[i].data, recived_packets[i].data_size);
                    }
                }

                output_file.close();
                std::cout << "file saved successfully " << filename << endl;
                continue;
            }

            std::cout << "file transfer incomplete " << endl;
            continue;
        }
        
    }
}

    