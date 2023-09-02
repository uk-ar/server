//
//  main.cpp
//  server
//
//  Created by 有澤悠紀 on 2023/09/02.
//

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct_in
#include <unistd.h> // for close
#include <string>
#include <stdio.h>

#define PORT 8080
#define CONTENT_PATH "/tmp/some/path"
#define BACKLOG 5 // number of client

using namespace std;

/*class Content{
    pthread_mutex_t lock;
    int file;
public:
    Content(string filePath){
        pthread_mutex_init(lock, <#const pthread_mutexattr_t * _Nullable#>);
        file = open(filePath);
    }
    string read(){
        return file.read();
    }
    bool write(string newContent){
        pthread_mutex_lock(lock);
        file.write(newContent);
        pthread_mutex_unlock(lock);
    }
};*/

int respond(int client){
    char buf[1024];
    ssize_t n = recv(client,buf,1024,0);
    if(n < 0){
        return EXIT_FAILURE;
    }
    strncpy(buf,"+PONG\r\n",1024);
    send(client, buf, strlen(buf),0);
    cout << "replied"<<endl;
    close(client);
    return 0;
}

int main(int argc, const char * argv[]) {
    // create a socket
    cout << argv[0]<<endl;
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if(server == -1){
        cerr << "Socket creation failed: " << strerror(errno) << endl;
        return EXIT_FAILURE;
    }
    // enable to launch multiple times
    int reuse_flag = 1;// 1 means true
    if(setsockopt(server,SOL_SOCKET,SO_REUSEPORT,&reuse_flag,sizeof(reuse_flag)) == -1){
        cerr << "Socket cannot reuse: " << strerror(errno) << endl;
        return EXIT_FAILURE;
    }
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
        .sin_addr   = {htonl(INADDR_ANY)},
    };// C++20

    // bind port
    if(::bind(server,(struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        cerr << "Socket bind failed: " << strerror(errno) << endl;
        return EXIT_FAILURE;
    }
    cout << "Waiting for a client..." << endl;
    //Content *content = new Content(CONTENT_PATH);

    if(listen(server,BACKLOG) == -1){
        cerr << "Socket listen failed: " << strerror(errno) << endl;
        return EXIT_FAILURE;
    }
    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server, (struct sockaddr *)&client_addr,&client_addr_len);
        if(client == -1){
            cerr << "Socket accept failed: " << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
        respond(client);
        /*int child_pid = fork();
        if(child_pid < 0){//error
            
        }else if(child_pid == 0){//child
            //close(server);
            
        }else{//parent
            //close(client);
            continue;
        }*/
        cout << "wait"<<endl;
        //fork(){//process thread
        
        /*if(payload.start_with("read")){
         send("OK:"+content.read());
         }else if(payload.starts_with("write")){
         //read body
         string newContent = payload.readbody()
         content.write(newContent);
         send("OK:");
         }*/

        //}
    }
    close(server);
    return 0;
    // accpect
    
    
    // recv data from client
        // read command
            // send file content
        // write command
            // lock the file
            // read the payload
            // write to the file
            // unlock the file
    // send status
}
