//
//  main.cpp
//  server
//
//  Created by 有澤悠紀 on 2023/09/02.
//

#include <iostream>
#include <fstream> // ofstream
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct_in
#include <unistd.h> // for close
#include <string>
#include <stdio.h>

#define PORT 8080
#define CONTENT_PATH "/tmp/some/path"
#define BACKLOG 5 // number of client
#define READ_CMD  "read \r\n"
//#define WRITE_CMD "write\r\n"

using namespace std;

class Repository{
    pthread_mutex_t lock;
    string filePath;
public:
    Repository(string filePath){
        pthread_mutex_init(&lock, NULL);
        filePath = filePath;
    }
    ~Repository(){
        
    }
    bool read(string &ans){
        ifstream file;// cannot share because stream has state.
        file.open(filePath);
        if(!file){
            return false;
        }
        file >> ans;
        file.close();
        return true;
    }
    bool write(string newContent){
        pthread_mutex_lock(&lock);
        ofstream file;
        if(!file){
            pthread_mutex_unlock(&lock);
            return false;
        }
        file.open(filePath);
        file << newContent;
        file.close();
        pthread_mutex_unlock(&lock);
        return true;
    }
};

int respond(int client,Repository *repo){
    char buf[1024];
    //char cmd[sizeof(READ_CMD)];
    //ssize_t n = recv(client,cmd,sizeof(READ_CMD),0);
    ssize_t n = read(client,buf,1024);
    if(n < 0){
        close(client);
        return EXIT_FAILURE;
    }
    //if(strncmp(cmd,"read \n",6)==0){
    if(strncmp(buf,"read \r\n",6)==0){
        //if(cmd == "read \n"){
        //snprintf(buf,sizeof(buf),"%s\n%s","write",argv[3]);
        strncpy(buf,"+PONG\r\n",1024);
        write(client, buf, strlen(buf));
        close(client);
    }else if(strncmp(buf,"write\r\n",6)==0){
    //}else if(strncmp(cmd,"write\n",6)==0){
    //}else if(cmd == "write\n"){
        //ssize_t n = recv(client,buf,1024,0);
        //repo.write()
        cout <<"buff:"<< buf << ":buff"<<endl;
        strncpy(buf,"+OK\r\n",1024);
        write(client, buf, strlen(buf));
        close(client);
    }else{
        cerr << "Command unknown: " << endl;
        close(client);
        return EXIT_FAILURE;
    }
    //string cmd(buf,sizeof(READ_CMD));
    //cout << ":"<<cmd<<":"<<endl;
    
    /*if(payload.start_with("read")){
     send("OK:"+content.read());
     }else if(payload.starts_with("write")){
     //read body
     string newContent = payload.readbody()
     content.write(newContent);
     send("OK:");
     }*/
    
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
    Repository * repo = new Repository("./foo");
    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server, (struct sockaddr *)&client_addr,&client_addr_len);
        if(client == -1){
            cerr << "Socket accept failed: " << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
        //respond(client);
        respond(client,repo);
        /*int child_pid = fork();
        if(child_pid < 0){//error
            
        }else if(child_pid == 0){//child
            
            return 0;
            //close(server);
        }else{//parent
            //close(client);
            continue;
        }*/
        cout << "wait"<<endl;
        //fork(){//process thread
        


        //}
    }
    //close(server);
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
