#include "stry.h"
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <unistd.h>
using namespace std;
using Lock = lock_guard<mutex>;
extern mutex mtx; 

Stry::Stry() {
    ports = {20, 21, 22, 23, 25, 53, 67, 69, 80, 110, 119, 
        123, 135, 137, 138, 139, 143, 161, 194, 443, 445, 465, 993, 
        995, 3306, 3389, 5900, 6379, 8080
    };

    services = {
        {20, "FTP-DATA"}, {21, "FTP"}, {22, "SSH"}, {23, "TELNET"}, {25, "SMTP"},
        {53, "DNS"}, {67, "DHCP"}, {69, "TFTP"}, {80, "HTTP"}, {110, "POP3"},
        {119, "NNTP"}, {123, "NTP"}, {135, "RPC"}, {137, "NETBIOS-NS"},
        {138, "NETBIOS-DGM"}, {139, "NETBIOS-SSN"}, {143, "IMAP"}, {161, "SNMP"},
        {194, "IRC"}, {443, "HTTPS"}, {445, "SMB"}, {465, "SMTPS"}, {993, "IMAPS"},
        {995, "POP3S"}, {3306, "MYSQL"}, {3389, "RDP"}, {5900, "VNC"}, 
        {6379, "REDIS"}, {8080, "HTTP-PROXY"}
    };
};

void Stry::banner(){
    cout << "\033[36m";
    cout << R"(
####################################
#███████╗████████╗██████╗ ██╗   ██╗#
#██╔════╝╚══██╔══╝██╔══██╗╚██╗ ██╔╝#
#███████╗   ██║   ██████╔╝ ╚████╔╝ #
#╚════██║   ██║   ██╔══██╗  ╚██╔╝  #
#███████║   ██║   ██║  ██║   ██║   #
#╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝   #
#                                  #
#            Stry v1.2             #
# By Xpl01t | www.XploitPower.com  #
####################################           
)";         
    cout << "\033[0m";
}

void Stry::takeIp(){
    cout << "[?]Enter target Ip: ";
    cin >> ip;   
}

void Stry::scanPort(int port){  

    //declare socket        
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    //check if socket creation was successful
    if(sock < 0){
        cerr << "[-]Sock creating failed." << endl;
    }
    
    //set up address structure for ip and port
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    //check if ip address is valid
    if(inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0){
        cerr << "\033[31m[-]Invalid IP address format.\033[0m" << endl;
        return;
    }

    //set timeout for socket connection
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    //attempt to connect to the port
    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));
    
    //check if its open or closed
    string status = (result == 0)? "open" : "closed";
    //version of port
    string version = "";

    //if port is open, get the version
    if(status == "open"){
        char buffer[1024] = {0};
        recv(sock, buffer, sizeof(buffer), 0);
        version = string(buffer);
        
    }

    //lock the mutex and push the result to results vector
    Lock lock(mtx);
    //push port, status and version
    results.push_back({port, status, version});
    close(sock);
        
}

void Stry::scanPorts(){
    //set timer
    auto start = chrono::steady_clock::now();
    cout << "[*]Scanning " << ip << ".." << endl;

    //set threads vector to hold threads for each port
    vector<thread> threads;

    //scan all ports
    for(int port : ports){
        threads.emplace_back(&Stry::scanPort, this, port);
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    //join all threads
    for(auto& t : threads){
        t.join();
    }

    //sort results by port number
    sort(results.begin(), results.end());


    for(const auto& [port, status, version] : results){
        string color = (status == "open") ? "\033[32m" : "\033[31m";
        string service = services.count(port)? services[port] : "UNKNOWN";

        cout << "[SCAN] ▶ " << color << "Port " 
                            << port << "/tcp" 
                            << "(" << service << ")" 
                            << ": " << status <<  endl;
        if(status == "open" && !version.empty()){
            cout << "       [VERSION] ▶ " << version ;
        }
        cout << "\033[0m";
    }

    //end timer and calculate duration
    auto end = chrono::steady_clock::now();
    auto duration_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "[+]Scan Completed in \033[1m" << duration_ms << " ms!\033[0m" << endl;
}
void Stry::run(){
    banner();
    takeIp();
    scanPorts();   
}
