#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // IPPROTO_TCP
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>

using namespace std;

int startConnection() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        return -1;
    }
    int port = 54000;
    string ip = "127.0.0.1";
    sockaddr_in sockAdr;
    sockAdr.sin_family = AF_INET;
    sockAdr.sin_port = htons(port);
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
    char bufans[1024];
    cout << "Wait server's reply" << endl;
    while(1) {
        int bytes = recv(sock, bufans, 1024, 0);
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

    char msgBuf[4096];
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
        if (fileToSend.is_open()) {
            while (getline(fileToSend, line)) {
                // cout << line << endl;
                int sendRes = send(sock, line.c_str(), line.size() + 1, 0);
                if (sendRes == -1) {
                    cout << "Couldn;t send to server" << endl;;
                    continue;
                }
                if (!getServerAck(sock)) {
                    break;
                }
            }
        }
        fileToSend.close();
    }

    close(sock);
    return 0;
}