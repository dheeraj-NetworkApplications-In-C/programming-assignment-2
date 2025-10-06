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
# define WIND_SIZE 4 // trying to implement go back n arg, hence defining window size 

struct FilePacket{
    /* data */
    int packet_id;
    int total_packets;
    int data_size;
    char data[BUFFER_SIZE - sizeof(int)*3]; // each data size has to be reduced by 3 ints because there are 3 integers that are also sent in with the data packet

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
    cout << "UDP Packet defined" << endl;

    // binding your socket 
    // socket type, your address and size of your address 

    
    bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr));
    addr_size = sizeof(remoteAddr);

    cout << "Socket binded to UDP packets" << endl;

    // recieving from the socket 

    cout << "Listening on the socket " << endl;
    cout << "Type 'exit' to end the session " << endl;
    string message; // replace with char array ASAP
    map<string, string> commandMap;
    std::map<int, FilePacket> recived_packets; // to store the recived packets
    uint32_t ack_pack_id;
    while (true) {
        std::memset(buffer, 0, sizeof(buffer)); 
        cout << endl;
        recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&remoteAddr, &addr_size);

        cout << "[+] Data recieved from the buffer is " << buffer << endl;
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
            continue;
        }

        string buffer_string(buffer);

        // Am i good at naming or something :)
        commandMap = getCommandMap(buffer_string);

        string fileName = commandMap["arg"];

        // get command from the client 
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

            int window_start = 0, current_window_pos = 0, window_end = WIND_SIZE;
            bool acks[WIND_SIZE];
            int packed_id_window[WIND_SIZE];
            int current_acks = 0;
            int __xl = ack_pack_id;

            // FIXME: update this with linked list (double linked) when you find time 

            // making packet array, this is very bad for large files 

            //FilePacket packets[total_packets];
            vector<FilePacket> packets(total_packets);

            for(int packet_id =0; packet_id < total_packets; packet_id++){
                FilePacket p;
                p.packet_id = packet_id;
                p.total_packets = total_packets;

                file.read(p.data, max_data_size);
                p.data_size = file.gcount();
                packets[packet_id] = p;
            }

            while(current_window_pos < total_packets) { 
                FilePacket currentPacket;

                currentPacket = packets[current_window_pos];
                int bytes_sent = sendto(sockfd, (const char*)&currentPacket, sizeof(currentPacket), 0, (struct sockaddr*)&remoteAddr, addr_size);

                current_window_pos++;

                if(bytes_sent < 0) {
                    cerr << "error : sending packet id failed " << currentPacket.packet_id << endl;
                }

                cout << "info : sent packet id " << currentPacket.packet_id << " size " << currentPacket.data_size << endl;
                usleep(2000);

                // reading ack here 
                //std::memset(&ack_pack_id, 0, sizeof(ack_pack_id));
                int n = recvfrom(sockfd, &ack_pack_id, sizeof(ack_pack_id), MSG_DONTWAIT, (struct sockaddr*)&remoteAddr, &addr_size);
                ack_pack_id = static_cast<uint32_t>(ntohl(ack_pack_id));
               // cout << "info : recieved ack for packet " << ack_pack_id << endl;
                if (n > 0) {
                    int ack_index = ack_pack_id - window_start;
                    if(ack_index >= 0 && ack_index < WIND_SIZE) { 
                        acks[ack_index] = true;
                    }
                }
                // also checking window start is
                if(((current_window_pos - window_start) % WIND_SIZE == 0 || (current_window_pos == total_packets )&& current_window_pos != window_start)) {
                    // sent all the packets in the window checking for acks
                    cout << "info : checking acks for window " << window_start << " to " << window_end << endl;
                    for(int i =0; i<WIND_SIZE ; i++ ){
                        if (!acks[i]){
                            // found something that ack has not been recieved hence shifting window to this point 
                            current_window_pos = window_start + i;
                            window_start += i;
                            window_end += i;
                            cout << "info : ack not recieved for packet " << current_window_pos << endl;
                            cout << "info : shifting window to packet " << current_window_pos << endl;
                            cout << "info : updated window start " << window_start << " current window end " << window_end << endl;
                            current_acks = 0;
                            std::memset(acks, false, sizeof(acks));
                            break;
                        } else {
                            current_acks++;
                            cout << "info : ack recieved for packet " << window_start + i << endl;
                        }
                    }
                    // if it came here it means loop didnt break hence it recived acks for all the data 
                    if(current_acks == WIND_SIZE){
                        cout << "info : all acks recived for the window " << window_start << " to " << window_end << endl;
                        window_start += WIND_SIZE;
                        window_end += WIND_SIZE;
                        cout << "info : updated window start " << window_start << " current window end " << window_end << endl;
                        current_acks = 0;
                        current_window_pos = window_start;
                        std::memset(acks, false, sizeof(acks));
                    } 
                }
            }

            file.close();
            std::cout << "File transfer completed for file " << fileName << endl;
            message = '\0';

            continue;

            // legacy code :) 

            /*for(int packet_id=0;packet_id < total_packets; packet_id++) {
                FilePacket packet;
                packet.packet_id = packet_id;
                packet.total_packets = total_packets;

                // read data for this packet 
                file.read(packet.data, max_data_size);
                packet.data_size = file.gcount(); // get the actual size of data read

                // sending the packet
                int bytes_sent = sendto(sockfd, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&remoteAddr, addr_size);
                current_window_pos ++;
                if(bytes_sent <0){
                    cerr << "Error : sending packet id " << packet_id << endl;
                    // try sending again 
                    packet_id--;
                    continue;
                }
                std::cout << "Sent packet id " << packet_id << " with size " << bytes_sent << " bytes" << endl;

                usleep(2000); // sleep for 2 ms to avoid flooding the network
                // reading ack here 
                std::memset(&ack_pack_id, 0, sizeof(ack_pack_id));
                int n = recvfrom(sockfd, &ack_pack_id, sizeof(ack_pack_id), 0, (struct sockaddr*)&remoteAddr, &addr_size);

                if(ack_pack_id == current_acks) {
                    // as soon as the ack reaches the correct one, I'm gonna 
                    current_acks ++;
                    window_end++;
                } else {
                    // again send all the packets from the the current_acks and current packet id
                    // shouldn't be done but for now I'm gonna stay in loop till I recive a acknowledgement 
                    
                }

                if(n > 0) {
                    cout << "error : wasnt able to read the acknowledgement somethings wrong " << endl;

                }
                
            }
            file.close();
            std::cout << "File transfer completed for file " << fileName << endl;
            message = '\0';

            continue; */
        }

        if (commandMap["command"]=="delete") {
            std::remove(fileName.c_str());
            
            // confirming delete
            bool success = false; 
            
            if(!std::ifstream(fileName.c_str())) {
                cout << "Cannot read  " << fileName<< " file hence deleted " ;
                success = true;
            }
            
            // convert it to network byte order 
            uint32_t netVal = htonl(static_cast<uint32_t>(success));
            sendto(sockfd, &netVal, sizeof(netVal), 0, (struct sockaddr*)&remoteAddr, addr_size);
            continue;
        }

        if (commandMap["command"] == "put") {
            // client uploaded a file and asked to store it 
            fileName = commandMap["arg"];

            // cleaning all the data recived before 
            recived_packets.clear();

            int total_packets = -1;
            int packets_recieved = 0;

            // setting timeout for socket, dont wanna spend too much time on the 
            struct timeval timeout;
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;

            // loop to recv file 
            while (true){
                FilePacket packet;
                ssize_t recived = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&remoteAddr, &addr_size);
                
                if (recived < 0) {
                    std::cout << "Timeout or recived error " << endl;
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
                std::ofstream output_file(fileName, std::ios::binary);
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
                std::cout << "file saved successfully " << fileName << endl;
                continue;
            }

            std::cout << "file transfer incomplete " << endl;
            continue;
            }
            
        }
    return 0;

}