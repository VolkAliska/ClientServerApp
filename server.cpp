#include <iostream>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h> // umask
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <signal.h>
#include <netdb.h>
// #include <arpa/inet.h>
#include <fcntl.h>


using namespace std;

int clientSock;
const int bufMsgSize = 4096;
const uint16_t inPort = 54000;

void sigHandler(int i) {
    close(clientSock);
    syslog(LOG_DEBUG, "Server stopped");
    closelog();
    exit(EXIT_SUCCESS);
}

int startServer() {
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        syslog(LOG_DEBUG, "Fork error");
        return 1;
    }
    else if (pid > 0) {
        exit(EXIT_SUCCESS); // parent
    }

    pid_t session = setsid();
    if (session < 0) {
        syslog(LOG_DEBUG, "Session lead error");
        return 1;
    }

    struct sigaction sigAct;
    sigset_t sigSet;
    sigemptyset(&sigSet);
    sigaddset(&sigSet, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sigSet, 0);

    sigAct.sa_handler = sigHandler;
    sigaction(SIGTERM, &sigAct, 0);
    sigaction(SIGHUP, &sigAct, 0);

    pid = fork();
    if (pid < 0) {
        syslog(LOG_DEBUG, "Fork error");
        return 1;
    }
    else if (pid > 0) {
        exit(EXIT_SUCCESS); // parent
    }

    int numFD = sysconf(_SC_OPEN_MAX); // open fds
    while (numFD >= 0) {
        close(numFD);
        numFD--;
    }

    umask(0);
    chdir("/");
    openlog("myserver", LOG_PID, LOG_DAEMON);
    syslog(LOG_DEBUG, "Start server");
    return 0;
}

int startNewConnection() {
    int listening = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listening == -1) {
        syslog(LOG_DEBUG, "Cannot create socket");
        return 1;
    }
    
        // Bind the ip address and port to a socket
    sockaddr_in sickAdr;
    sickAdr.sin_family = AF_INET;
    sickAdr.sin_port = htons(inPort);
    inet_pton(AF_INET, "0.0.0.0", &sickAdr.sin_addr);
    
    bind(listening, (sockaddr*)&sickAdr, sizeof(sickAdr));
    listen(listening, SOMAXCONN);
    
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    
    clientSock = accept(listening, (sockaddr*)&client, &clientSize);
    
    char host[NI_MAXHOST] = {'0'};      // host name
    char service[NI_MAXSERV] ={'0'};   // connected on port

    int status = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0);
    if (status != 0) {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
    }

    close(listening);
    syslog(LOG_DEBUG, "Create socket");
    return 0;
}

int sendReply() {
    string ans = "success";
    int sendRes = send(clientSock, ans.c_str(), ans.size() + 1, 0);
    if (sendRes == -1) {
        syslog(LOG_DEBUG, "Couldn't send reply");
        return 1;
    }
    return 0;
}

/*
add "server_" to actual filename
*/
string makeFileName(string filename) {
    int dotPos = filename.find_last_of('/');
    string tail = filename.substr(dotPos + 1);
    filename = filename.substr(0, dotPos + 1) + "server_" + tail;
    return filename;
}

int main() {
    int status = startServer();
    if (status != 0) {
        exit(EXIT_FAILURE);
    }

    while(true) {
        
        int clSock = startNewConnection();
        if (clSock != 0) {
            break;
        }
    
        char bufMsg[bufMsgSize] = {'0'};
        while (true) {
            int received = recv(clientSock, bufMsg, bufMsgSize, 0);
            sendReply();

            string filename(bufMsg);
            filename = makeFileName(filename);// optional

            if (received == -1) {
                syslog(LOG_DEBUG, "Error in receiving");
                break;
            }
            else if (received != 0) {
                int fout;
                syslog(LOG_DEBUG, filename.c_str());
                fout = open(filename.c_str() , O_CREAT  | O_WRONLY, S_IRUSR | S_IWUSR);
                if (fout == -1){
                    syslog(LOG_DEBUG, "File is not created");
                    // syslog(LOG_DEBUG, strerror(errno));
                }
                int fileFilled = 0;
                char recText[bufMsgSize] = {'0'};
                while(1) {
                    int bytes = recv(clientSock, recText, bufMsgSize, 0);
                    if (bytes == -1) {
                        syslog(LOG_DEBUG, "Error in receiving");
                        cerr << "Error in recv(). Quitting" << endl;
                        break;
                    }
                    else if (bytes != 0) {
                        string line(recText);
                        int written = write(fout, line.c_str(), line.length());
                        fileFilled = 1;
                        if (written != line.length())
                            syslog(LOG_DEBUG, "not writed");
                        sendReply();
                    }
                    else {
                        // disconnected
                        break;
                    }
                }
                close(fout); 
            }
            if (received == 0) {
                syslog(LOG_DEBUG, "Client disconnected ");
                break;
            }
        }
        close(clientSock);
    }
    return 0;
}