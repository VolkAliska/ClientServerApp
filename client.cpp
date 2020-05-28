#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // IPPROTO_TCP
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>

using namespace std;

const int bufMsgSize = 1024;
const int strSize = 4096;
const uint16_t inPort = 54000;

int startConnection() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        return -1;
    }
    string ip = "127.0.0.1";
    sockaddr_in sockAdr;
    sockAdr.sin_family = AF_INET;
    sockAdr.sin_port = htons(inPort);
    inet_pton(AF_INET, ip.c_str(), &sockAdr.sin_addr);

    int connection = connect(sock, (sockaddr*)&sockAdr, sizeof(sockAdr));
    if (connection == -1) {
        return -1;
    }
    return sock;
}

int isFileExist(string filename) {
    ifstream file;
    file.open(filename);
    if (file) {
        return 1;
    }
    else {
        cout << "File doesn;t exist" << endl;
        return 0;
    }
}

string getNameWithPath(string filename) {
    char path[1024];
    getcwd(path, sizeof(path));
    string pathStr(path);
    string fullname = pathStr + "/" + filename;
    return fullname;
}

int getServerAck(int sock) {
    char bufans[bufMsgSize];
    while(1) {
        int bytes = recv(sock, bufans, bufMsgSize, 0);
        if (bytes == -1) {
            cerr << "Error recieving" << endl;
            return 0;
        }
        else if (bytes != 0) {
            string ans(bufans);
            if (ans == "success"){
                return 1;
            }
        }
    }
}

int main() {
    int sock = startConnection();
    if (sock == -1) {
        return 1;
    }

    string filename;

    while (true) {
        cout << "filename: ";
        cin >> filename;
        if (!isFileExist(filename)) {
            break;
        }

        filename = getNameWithPath(filename);

        int sendRes = send(sock, filename.c_str(), filename.size() + 1, 0);
        if (sendRes == -1) {
            cout << "Couldn;t send to server" << endl;
            continue;
        }
        if (!getServerAck(sock)) {
            break;
        }

        string line;

        ifstream fileToSend(filename);
        FILE * fp;
        char str[strSize];
        if(fp = fopen(filename.c_str(), "r")) {
            while(!feof(fp)) {
                if (fgets(str, strSize-2, fp)) {
                    printf("%s", str);
                    int sendRes = send(sock, str, strSize-1, 0);
                    if (sendRes == -1) {
                        cout << "Couldn;t send to server" << endl;;
                        continue;
                    }
                    if (!getServerAck(sock)) {
                        break;
                    }
                }
            }
        }
        // if (fileToSend.is_open()) {
        //     while (getline(fileToSend, line)) {
        //         int sendRes = send(sock, line.c_str(), line.size() + 1, 0);
        //         if (sendRes == -1) {
        //             cout << "Couldn;t send to server" << endl;;
        //             continue;
        //         }
        //         if (!getServerAck(sock)) {
        //             break;
        //         }
        //     }
        // }
        fileToSend.close();
        fclose(fp);
    }

    close(sock);
    return 0;
}