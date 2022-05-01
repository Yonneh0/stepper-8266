/*
 * FTP Serveur for ESP8266
 * based on FTP Serveur for Arduino Due and Ethernet shield (W5100) or WIZ820io (W5200)
 * based on Jean-Michel Gallego's work
 * modified to work with esp8266 SPIFFS by David Paiva david@nailbuster.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//  2017: modified by @robo8080
//  2019: modified by @HenrikSte

#include <WiFiClient.h>

#define FTP_CTRL_PORT 21          // Command port on wich server is listening
#define FTP_DATA_PORT_PASV 50009  // Data port in passive mode

#define FTP_TIME_OUT 5               // Disconnect client after 5 minutes of inactivity
#define FTP_CMD_SIZE 255 + 8         // max size of a command
#define FTP_CWD_SIZE 255 + 8         // max size of a directory name
#define FTP_FIL_SIZE 255             // max size of a file name
#define FTP_BUF_SIZE (8192 * 1) - 1  // 512   // size of file buffer for read/write

class FtpServer {
   public:
    FtpServer();
    void end();
    void begin(String uname, String pword);
    int handleFTP();

   private:
    void iniVariables();
    void clientConnected();
    void disconnectClient();
    boolean userIdentity();
    boolean userPassword();
    boolean processCommand();
    boolean dataConnect();
    boolean doRetrieve();
    boolean doStore();
    void closeTransfer();
    void abortTransfer();
    boolean makePath(char *fullname);
    boolean makePath(char *fullName, char *param);
    int8_t readChar();

    IPAddress dataIp;  // IP address of client for data
    WiFiClient client;
    WiFiClient data;

    File file;

    boolean dataPassiveConn;
    uint16_t dataPort;
    char buf[FTP_BUF_SIZE];      // data buffer for transfers
    char cmdLine[FTP_CMD_SIZE];  // where to store incoming char from client
    char cwdName[FTP_CWD_SIZE];  // name of current directory
    char command[5];             // command sent by client
    boolean rnfrCmd;             // previous command was RNFR
    char *parameters;            // point to begin of parameters sent by client
    uint16_t iCL;                // pointer to cmdLine next incoming char
    int8_t cmdStatus,            // status of ftp command connexion
        transferStatus;          // status of ftp data transfer
    uint32_t millisTimeOut,      // disconnect after 5 min of inactivity
        millisDelay,
        millisEndConnection,  //
        millisBeginTrans,     // store time of beginning of a transaction
        bytesTransfered;      //
    String _FTP_USER;
    String _FTP_PASS;
};

WiFiServer ftpServer(FTP_CTRL_PORT);
WiFiServer dataServer(FTP_DATA_PORT_PASV);

FtpServer::FtpServer() {
}

void FtpServer::end() {
    ftpServer.close();
    delay(10);
    dataServer.close();
    delay(10);
    millisTimeOut = (uint32_t)FTP_TIME_OUT * 60 * 1000;
    millisDelay = 0;
    cmdStatus = 0;
}
void FtpServer::begin(String uname, String pword) {
    // Tells the ftp server to begin listening for incoming connection
    _FTP_USER = uname;
    _FTP_PASS = pword;
    ftpServer.begin();
    delay(10);
    dataServer.begin();
    delay(10);
    millisTimeOut = (uint32_t)FTP_TIME_OUT * 60 * 1000;
    millisDelay = 0;
    cmdStatus = 0;
    iniVariables();
}

void FtpServer::iniVariables() {
    // Default for data port
    dataPort = FTP_DATA_PORT_PASV;

    // Default Data connection is Active
    dataPassiveConn = true;

    // Set the root directory
    strcpy(cwdName, "/");

    rnfrCmd = false;
    transferStatus = 0;
}

int FtpServer::handleFTP() {
    if ((int32_t)(millisDelay - millis()) > 0)
        return 0;

    if (ftpServer.hasClient()) {
        //  if (ftpServer.available()) {
        client.stop();
        client = ftpServer.available();
    }

    if (cmdStatus == 0) {
        if (client.connected())
            disconnectClient();
        cmdStatus = 1;
    } else if (cmdStatus == 1)  // Ftp server waiting for connection
    {
        abortTransfer();
        iniVariables();
        cmdStatus = 2;
    } else if (cmdStatus == 2)  // Ftp server idle
    {
        if (client.connected())  // A client connected
        {
            clientConnected();
            millisEndConnection = millis() + 10 * 1000;  // wait client id during 10 s.
            cmdStatus = 3;
        }
    } else if (readChar() > 0)  // got response
    {
        if (cmdStatus == 3)  // Ftp server waiting for user identity
            if (userIdentity())
                cmdStatus = 4;
            else
                cmdStatus = 0;
        else if (cmdStatus == 4)  // Ftp server waiting for user registration
            if (userPassword()) {
                cmdStatus = 5;
                millisEndConnection = millis() + millisTimeOut;
            } else
                cmdStatus = 0;
        else if (cmdStatus == 5)  // Ftp server waiting for user command
        {
            if (!processCommand())
                cmdStatus = 0;
            else
                millisEndConnection = millis() + millisTimeOut;
        }
    } else if (!client.connected() || !client) {
        cmdStatus = 1;
    }

    if (transferStatus == 1)  // Retrieve data
    {
        if (!doRetrieve())
            transferStatus = 0;
    } else if (transferStatus == 2)  // Store data
    {
        if (!doStore())
            transferStatus = 0;
    } else if (cmdStatus > 2 && !((int32_t)(millisEndConnection - millis()) > 0)) {
        client.println("530 Timeout");
        millisDelay = millis() + 200;  // delay of 200 ms
        cmdStatus = 0;
    }

    return transferStatus != 0 || cmdStatus != 0;
}

void FtpServer::clientConnected() {
    client.println("220 Welcome");
    iCL = 0;
}

void FtpServer::disconnectClient() {
    abortTransfer();
    client.println("221 Goodbye");
    client.stop();
}

boolean FtpServer::userIdentity() {
    if (strcmp(command, "USER"))
        client.println("500 Syntax error");
    if (strcmp(parameters, _FTP_USER.c_str()))
        client.println("530 user not found");
    else {
        client.println("331 OK. Password required");
        strcpy(cwdName, "/");
        return true;
    }
    millisDelay = millis() + 100;  // delay of 100 ms
    return false;
}

boolean FtpServer::userPassword() {
    if (strcmp(command, "PASS"))
        client.println("500 Syntax error");
    else if (strcmp(parameters, _FTP_PASS.c_str()))
        client.println("530 ");
    else {
        client.println("230 OK.");
        return true;
    }
    millisDelay = millis() + 100;  // delay of 100 ms
    return false;
}

boolean FtpServer::processCommand() {
    ///////////////////////////////////////
    //                                   //
    //      ACCESS CONTROL COMMANDS      //
    //                                   //
    ///////////////////////////////////////

    //
    //  CDUP - Change to Parent Directory
    //
    if (!strcmp(command, "CDUP")) {
        uint16_t needle = 1;
        for (int i = needle; i < 263; i++) {
            if (cwdName[i] == 47) needle = i;
            if (cwdName[i] == 0) break;
        }
        cwdName[needle] = 0;

        client.println("250 Ok. Current directory is \"" + String(cwdName) + "\"");
    }
    //
    //  CWD - Change Working Directory
    //
    else if (!strcmp(command, "CWD")) {
        if (strcmp(parameters, ".") == 0)  // 'CWD .' is the same as PWD command
            client.println("257 \"" + String(cwdName) + "\" is your current directory");
        else {
            String dir;

            if (parameters[0] == '/') {
                dir = parameters;
            } else if (!strcmp(cwdName, "/"))  // avoid "\\newdir"
            {
                dir = String("/") + parameters;
            } else {
                dir = String(cwdName) + "/" + parameters;
            }

            if (LittleFS.exists(dir)) {
                File file = LittleFS.open(dir, "r");
                if (file && file.isDirectory()) {
                    strcpy(cwdName, dir.c_str());
                    client.println("250 CWD Ok. Current directory is \"" + String(dir) + "\"");
                } else {
                    client.println("550 directory or file does not exist \"" + String(parameters) + "\"");
                }
            } else {
                client.println("550 directory or file does not exist \"" + String(parameters) + "\"");
            }
        }

    }
    //
    //  PWD - Print Directory
    //
    else if (!strcmp(command, "PWD"))
        client.println("257 \"" + String(cwdName) + "\" is your current directory");
    //
    //  QUIT
    //
    else if (!strcmp(command, "QUIT")) {
        disconnectClient();
        return false;
    }

    ///////////////////////////////////////
    //                                   //
    //    TRANSFER PARAMETER COMMANDS    //
    //                                   //
    ///////////////////////////////////////

    //
    //  MODE - Transfer Mode
    //
    else if (!strcmp(command, "MODE")) {
        if (!strcmp(parameters, "S"))
            client.println("200 S Ok");
        // else if( ! strcmp( parameters, "B" ))
        //  client.println( "200 B Ok\r\n";
        else
            client.println("504 Only S(tream) is suported");
    }
    //
    //  PASV - Passive Connection management
    //
    else if (!strcmp(command, "PASV")) {
        if (data.connected()) data.stop();
        // dataServer.begin();
        // dataIp = Ethernet.localIP();
        dataIp = WiFi.localIP();
        dataPort = FTP_DATA_PORT_PASV;
        client.println("227 Entering Passive Mode (" + String(dataIp[0]) + "," + String(dataIp[1]) + "," + String(dataIp[2]) + "," + String(dataIp[3]) + "," + String(dataPort >> 8) + "," + String(dataPort & 255) + ").");
        dataPassiveConn = true;
    }
    //
    //  PORT - Data Port
    //
    else if (!strcmp(command, "PORT")) {
        if (data) data.stop();
        // get IP of data client
        dataIp[0] = atoi(parameters);
        char *p = strchr(parameters, ',');
        for (uint8_t i = 1; i < 4; i++) {
            dataIp[i] = atoi(++p);
            p = strchr(p, ',');
        }
        // get port of data client
        dataPort = 256 * atoi(++p);
        p = strchr(p, ',');
        dataPort += atoi(++p);
        if (p == NULL)
            client.println("501 Can't interpret parameters");
        else {
            client.println("200 PORT command successful");
            dataPassiveConn = false;
        }
    }
    //
    //  STRU - File Structure
    //
    else if (!strcmp(command, "STRU")) {
        if (!strcmp(parameters, "F"))
            client.println("200 F Ok");
        // else if( ! strcmp( parameters, "R" ))
        //  client.println( "200 B Ok\r\n";
        else
            client.println("504 Only F(ile) is suported");
    }
    //
    //  TYPE - Data Type
    //
    else if (!strcmp(command, "TYPE")) {
        if (!strcmp(parameters, "A"))
            client.println("200 TYPE is now ASII");
        else if (!strcmp(parameters, "I"))
            client.println("200 TYPE is now 8-bit binary");
        else
            client.println("504 Unknow TYPE");
    }

    ///////////////////////////////////////
    //                                   //
    //        FTP SERVICE COMMANDS       //
    //                                   //
    ///////////////////////////////////////

    //
    //  ABOR - Abort
    //
    else if (!strcmp(command, "ABOR")) {
        abortTransfer();
        client.println("226 Data connection closed");
    }
    //
    //  DELE - Delete a File
    //
    else if (!strcmp(command, "DELE")) {
        char path[FTP_CWD_SIZE];
        if (strlen(parameters) == 0)
            client.println("501 No file name");
        else if (makePath(path)) {
            if (!LittleFS.exists(path))
                client.println("550 File " + String(parameters) + " not found");
            else {
                if (LittleFS.remove(path))
                    client.println("250 Deleted " + String(parameters));
                else
                    client.println("450 Can't delete " + String(parameters));
            }
        }
    }
    //
    //  LIST - List
    //
    else if (!strcmp(command, "LIST")) {
        if (!dataConnect())
            client.println("425 No data connection");
        else {
            client.println("150 Accepted data connection");
            uint16_t nm = 0;
            File dir = LittleFS.open(cwdName, "r");
            if ((!dir) || (!dir.isDirectory()))
                client.println("550 Can't open directory " + String(cwdName));
            else {
                File file = dir.openNextFile();
                while (file) {
                    char buff[20];
                    time_t ft = file.getLastWrite();
                    strftime(buff, 20, "%m-%d-%Y %I:%M%p", gmtime(&ft));
                    if (file.isDirectory()) {
                        data.printf("%s <DIR> %s", buff, file.name());
                    } else {
                        data.printf("%s %u %s", buff, file.size(), file.name());
                    }
                    nm++;
                    file = dir.openNextFile();
                }
                client.println("226 " + String(nm) + " matches total");
            }
            data.stop();
        }
    }
    //
    //  MLSD - Listing for Machine Processing (see RFC 3659)
    //
    else if (!strcmp(command, "MLSD")) {
        if (!dataConnect()) {
            client.println("425 No data connection MLSD");
        } else {
            client.println("150 Accepted data connection");
            uint16_t nm = 0;
            File dir = LittleFS.open(cwdName, "r");
            if ((!dir) || (!dir.isDirectory()))
                client.println("550 Can't open directory " + String(cwdName));
            else {
                File file = dir.openNextFile();
                while (file) {
                    // String fn;
                    // fn = file.name();
                    // fn.remove(0, strlen(cwdName) - 1);
                    // if (fn[0] == '/') fn.remove(0, 1);
                    char buff[20];
                    time_t ft = file.getLastWrite();
                    strftime(buff, 20, "%Y%m%d%H%M%S", gmtime(&ft));
                    data.printf(" Type=%s;Size=%u;modify=%s; %s\n", file.isDirectory() ? "dir" : "file", file.size(), buff, file.name());
                    nm++;
                    file = dir.openNextFile();
                }
                client.println("226-options: -a -l");
                client.println("226 " + String(nm) + " matches total");
            }
            data.stop();
        }
    }
    //
    //  NLST - Name List
    //
    else if (!strcmp(command, "NLST")) {
        if (!dataConnect())
            client.println("425 No data connection");
        else {
            client.println("150 Accepted data connection");
            uint16_t nm = 0;
            //      Dir dir=LittleFS.openDir(cwdName);
            File dir = LittleFS.open(cwdName, "r");
            if (!LittleFS.exists(cwdName))
                client.println("550 Can't open directory " + String(parameters));
            else {
                File file = dir.openNextFile();
                //        while( dir.next())
                while (file) {
                    //          data.println( dir.fileName());
                    data.println(file.name());
                    nm++;
                    file = dir.openNextFile();
                }
                client.println("226 " + String(nm) + " matches total");
            }
            data.stop();
        }
    }
    //
    //  NOOP
    //
    else if (!strcmp(command, "NOOP")) {
        // dataPort = 0;
        client.println("200 Zzz...");
    }
    //
    //  RETR - Retrieve
    //
    else if (!strcmp(command, "RETR")) {
        char path[FTP_CWD_SIZE];
        if (strlen(parameters) == 0)
            client.println("501 No file name");
        else if (makePath(path)) {
            file = LittleFS.open(path, "r");
            if (!file)
                client.println("550 File " + String(parameters) + " not found");
            else if (!file)
                client.println("450 Can't open " + String(parameters));
            else if (!dataConnect())
                client.println("425 No data connection");
            else {
                client.println("150-Connected to port " + String(dataPort));
                client.println("150 " + String(file.size()) + " bytes to download");
                millisBeginTrans = millis();
                bytesTransfered = 0;
                transferStatus = 1;
            }
        }
    }
    //
    //  STOR - Store
    //
    else if (!strcmp(command, "STOR")) {
        char path[FTP_CWD_SIZE];
        if (strlen(parameters) == 0)
            client.println("501 No file name");
        else if (makePath(path)) {
            file = LittleFS.open(path, "w");
            if (!file)
                client.println("451 Can't open/create " + String(parameters));
            else if (!dataConnect()) {
                client.println("425 No data connection");
                file.close();
            } else {
                client.println("150 Connected to port " + String(dataPort));
                millisBeginTrans = millis();
                bytesTransfered = 0;
                transferStatus = 2;
            }
        }
    }
    //
    //  MKD - Make Directory
    //
    else if (!strcmp(command, "MKD")) {
        String dir;

        if (!strcmp(cwdName, "/"))  // avoid "\\newdir"
        {
            dir = String("/") + parameters;
        } else {
            dir = String(cwdName) + "/" + parameters;
        }
        fs::FS &fs = LittleFS;
        if (fs.mkdir(dir.c_str())) {
            client.println("257 \"" + String(parameters) + "\" - Directory successfully created");
        } else {
            client.println("502 Can't create \"" + String(parameters));
        }
    }
    //
    //  RMD - Remove a Directory
    //
    else if (!strcmp(command, "RMD")) {
        String dir;

        if (!strcmp(cwdName, "/"))  // avoid "\\newdir"
        {
            dir = String("/") + parameters;
        } else {
            dir = String(cwdName) + "/" + parameters;
        }
        fs::FS &fs = LittleFS;
        if (fs.rmdir(dir.c_str())) {
            client.println("250 RMD command successful");
        } else {
            client.println("502 Can't delete \"" + String(parameters));  // not support on espyet
        }

    }
    //
    //  RNFR - Rename From
    //
    else if (!strcmp(command, "RNFR")) {
        buf[0] = 0;
        if (strlen(parameters) == 0)
            client.println("501 No file name");
        else if (makePath(buf)) {
            if (!LittleFS.exists(buf))
                client.println("550 File " + String(parameters) + " not found");
            else {
                client.println("350 RNFR accepted - file exists, ready for destination");
                rnfrCmd = true;
            }
        }
    }
    //
    //  RNTO - Rename To
    //
    else if (!strcmp(command, "RNTO")) {
        char path[FTP_CWD_SIZE];
        // char dir[ FTP_FIL_SIZE ];
        if (strlen(buf) == 0 || !rnfrCmd)
            client.println("503 Need RNFR before RNTO");
        else if (strlen(parameters) == 0)
            client.println("501 No file name");
        else if (makePath(path)) {
            if (LittleFS.exists(path))
                client.println("553 " + String(parameters) + " already exists");
            else {
                if (LittleFS.rename(buf, path))
                    client.println("250 File successfully renamed or moved");
                else
                    client.println("451 Rename/move failure");
            }
        }
        rnfrCmd = false;
    }

    ///////////////////////////////////////
    //                                   //
    //   EXTENSIONS COMMANDS (RFC 3659)  //
    //                                   //
    ///////////////////////////////////////

    //
    //  FEAT - New Features
    //
    else if (!strcmp(command, "FEAT")) {
        client.println("211-Extensions suported:");
        client.println(" MLSD");
        client.println("211 End.");
    }
    //
    //  MDTM - File Modification Time (see RFC 3659)
    //
    else if (!strcmp(command, "MDTM")) {
        client.println("550 Unable to retrieve time");
    }

    //
    //  SIZE - Size of the file
    //
    else if (!strcmp(command, "SIZE")) {
        char path[FTP_CWD_SIZE];
        if (strlen(parameters) == 0)
            client.println("501 No file name");
        else if (makePath(path)) {
            file = LittleFS.open(path, "r");
            if (!file)
                client.println("450 Can't open " + String(parameters));
            else {
                client.println("213 " + String(file.size()));
                file.close();
            }
        }
    }
    //
    //  SITE - System command
    //
    else if (!strcmp(command, "SITE")) {
        client.println("500 Unknow SITE command " + String(parameters));
    }
    //
    //  Unrecognized commands ...
    //
    else
        client.println("500 Unknow command");

    return true;
}

boolean FtpServer::dataConnect() {
    unsigned long startTime = millis();
    // wait 5 seconds for a data connection
    if (!data.connected()) {
        while (!dataServer.hasClient() && millis() - startTime < 10000)
        //    while (!dataServer.available() && millis() - startTime < 10000)
        {
            // delay(100);
            yield();
        }
        if (dataServer.hasClient()) {
            //    if (dataServer.available()) {
            data.stop();
            data = dataServer.available();
        }
    }

    return data.connected();
}

boolean FtpServer::doRetrieve() {
    // int16_t nb = file.readBytes((uint8_t*) buf, FTP_BUF_SIZE );
    int16_t nb = file.readBytes(buf, FTP_BUF_SIZE);
    if (nb > 0) {
        data.write((uint8_t *)buf, nb);
        bytesTransfered += nb;
        return true;
    }
    closeTransfer();
    return false;
}

unsigned long count = 0;
boolean FtpServer::doStore() {
    if (data.available()) {
        int16_t nb = data.readBytes((uint8_t *)buf, FTP_BUF_SIZE);
        if (nb > 0) {
            file.write((uint8_t *)buf, nb);
            bytesTransfered += nb;
        }
        return true;
    }
    closeTransfer();
    return false;
}

void FtpServer::closeTransfer() {
    uint32_t deltaT = (int32_t)(millis() - millisBeginTrans);
    if (deltaT > 0 && bytesTransfered > 0) {
        client.println("226-File successfully transferred");
        client.println("226 " + String(deltaT) + " ms, " + String(bytesTransfered / deltaT) + " kbytes/s");
    } else
        client.println("226 File successfully transferred");
    file.close();
    data.stop();
}

void FtpServer::abortTransfer() {
    if (transferStatus > 0) {
        file.close();
        data.stop();
        client.println("426 Transfer aborted");
    }
    transferStatus = 0;
}

int8_t FtpServer::readChar() {
    int8_t rc = -1;

    if (client.available()) {
        char c = client.read();
        if (c == '\\') c = '/';
        if (c != '\r') {
            if (c != '\n') {
                if (iCL < FTP_CMD_SIZE)
                    cmdLine[iCL++] = c;
                else
                    rc = -2;  //  Line too long
            } else {
                cmdLine[iCL] = 0;
                command[0] = 0;
                parameters = NULL;
                // empty line?
                if (iCL == 0)
                    rc = 0;
                else {
                    rc = iCL;
                    // search for space between command and parameters
                    parameters = strchr(cmdLine, ' ');
                    if (parameters != NULL) {
                        if (parameters - cmdLine > 4) {
                            rc = -2;  // Syntax error
                        } else {
                            strncpy(command, cmdLine, parameters - cmdLine);
                            command[parameters - cmdLine] = 0;

                            while (*(++parameters) == ' ')
                                ;
                        }
                    } else if (strlen(cmdLine) > 4)
                        rc = -2;  // Syntax error.
                    else
                        strcpy(command, cmdLine);
                    iCL = 0;
                }
            }
        }
        if (rc > 0) {
            for (uint8_t i = 0; i < strlen(command); i++) {
                command[i] = toupper(command[i]);
            }
        }
        if (rc == -2) {
            iCL = 0;
            client.println("500 Syntax error");
        }
    }
    return rc;
}

// Make complete path/name from cwdName and parameters
//
// 3 possible cases: parameters can be absolute path, relative path or only the name
//
// parameters:
//   fullName : where to store the path/name
//
// return:
//    true, if done

boolean FtpServer::makePath(char *fullName) {
    return makePath(fullName, parameters);
}

boolean FtpServer::makePath(char *fullName, char *param) {
    if (param == NULL)
        param = parameters;

    // Root or empty?
    if (strcmp(param, "/") == 0 || strlen(param) == 0) {
        strcpy(fullName, "/");
        return true;
    }
    // If relative path, concatenate with current dir
    if (param[0] != '/') {
        strcpy(fullName, cwdName);
        if (fullName[strlen(fullName) - 1] != '/')
            strncat(fullName, "/", FTP_CWD_SIZE);
        strncat(fullName, param, FTP_CWD_SIZE);
    } else
        strcpy(fullName, param);
    // If ends with '/', remove it
    uint16_t strl = strlen(fullName) - 1;
    if (fullName[strl] == '/' && strl > 1)
        fullName[strl] = 0;
    if (strlen(fullName) < FTP_CWD_SIZE)
        return true;

    client.println("500 Command line too long");
    return false;
}
