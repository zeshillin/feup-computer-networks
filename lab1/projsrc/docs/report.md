# RC - Serial Port Project
### by José Castro and Pedro Silva - T14G9,
### Faculty of Engineering of the University of Porto

<br>

# 1. Introduction

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The goal of this project is to develop a digital infrastructure to serve as the basis for information exchange through a serial port. This infrastructure is divided into two layers - the application layer and the data link layer - and each of these operate independently of each other. <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;This report will serve as a written specification of what was achieved in this development process.  

- Architecture
- Code Structure
- Main Use Cases
- Data Link Protocol
- Application Protocol
- Validation
- Data Link Protocol Efficiency
- Conclusions 

<br>

# 2. Architecture

## 2.1 Data Link Layer:
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The Data Link Layer is the lowest level of the application and is responsible for supervising direct communication with the serial port, detecting errors and handling them.

## 2.2 Application Layer: 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The application layer is above the link layer in terms of leveling and is responsible for handling file contents and sending them through the link layer. Information is retrieved from a file and sent through the link layer, which is then received in this same link layer: the application layer simply takes care of unpacking it in the receiver's end.

## 2.3 Main 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Simple interface that runs transmitter or receiver code depending on the role given during the program execution: 
    
&nbsp;&nbsp;&nbsp;&nbsp;*serialport role [filepath]*  

where:
- *serialport* is the port's path (e.g.: dev/ttyS01)
- *role* is the role (*{rx, tx}* where *rx* is the receiver, *tx* is the transmitter)
- *[filepath] is the file-to-send's path (only needed if initializing the program as transmitter)

<br>

# 3. Code Structure

## 3.1 Application Layer
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The application layer stores every function and usable without any regard for who is using it, be it is the receiver or the transmitter. Important parameters for the application layer to know about would be the file descriptor and the role it is currently executing, however, these are both specified in *main.c* and, as such, there is no need for them.

### *application_layer.h & application_layer.c*

#
    typedef struct {
        char* filename;
        u_int8_t filename_size;
        long filesize;
    } fileStruct;

A struct is used to store the file we're sending and to make a new name in the receiver end.<br>
The *filename* field is a character buffer that contains the file's (relative) path.<br>
The *filename_size* field contains the size of the *filename* buffer.<br>
The *filesize* field contains the size of the file.

#

    typedef struct {
        unsigned char* packet;
        int packet_size;
    } startPacket;

A struct is used to store the starting control packet sent to then send it again as an end packet at the end of the program's data transfer.<br>
The *packet* buffer stores the packet's contents.<br>
The *packet_size* specifies the *packet* buffer size.

#

    typedef struct {
        u_int8_t T;
        u_int8_t L;
        u_int8_t* V;
    } TLV;

A struct is used to store the TLV (Type, Length, Value) section of a control packet.<br>
The *T* field represents the Type parameter (0 – file size, 1 – file name).<br>
The *L* field represents the Length parameter (the size of the V field in octets).
The *V* field contains the data to be sent (file size or file name).

#

    int applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename)

Effectively starts and estabilishes the connection between transmitter and receiver and defines the roles in the current context. Uses link layer's **llopen**


<br>

# 4. Main Use-Cases

The two main use cases are sending a file or receiving a file. The flow of these use cases is as follows:

### Sender

- Runs the program (`main`), which starts the application (`applicationLayer`).

- `llopen` is called, and the connection to the serial port is opened. `sendFile` is called.

- `sendFile` opens the file and sends its general information (not the contents) to the receiver, and then calls `sendFileContents` with the file and its size as the parameters.
 
- `sendFileContents` divides the file data in packets, whose size depends on the user's choice, and sends them sequentially through `llwrite` to the receiver, until it finishes the file or or errors occur, that trigger the timeout more than the number of retries specified. These can be detected thanks to the BCCs, and along with them the header and the start/end flags are also added. The data is also stuffed to prevent misinterpretation of part of it as flags, for example.

- The file is closed, followed by the application in `appLayer_exit`, which calls `llclose`.

### Reader

- Runs the program (`main`), which starts the application (`applicationLayer`).

- `llopen` is called, and the connection to the serial port is opened. `readFile` is called.

- `readFile` reads file information (except the content) and stores it in a struct called `file_info`, and then calls `writeFileContents` with the file to be received as the parameter.

- `writeFileContents` receives the information packets through `llread` which, after being destuffed and checked for errors, have their headers, flags and BCCs removed. Finally, each packet is written to the file sequentially, until every packet is received or errors occur, that trigger the timeout more than the number of retries specified.

- The file is closed, followed by the application in `appLayer_exit`, which calls `llclose`.

# 5. Data Link Protocol
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The data link layer has 4 main functions: *llopen* (used for estabilishing connection at the start of the data transfer), *llwrite* used to send information frames, consequently, *llread* to read information frames and, finally, *llclose* to end the connection.


## 5.1 - **llopen**
    int llopen(LinkLayer connectionParameters)

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Before any information is exchanged between the receiver and the transmitter, *llopen* verifies that the serial port is open and available for usage. In order to use timeouts on read and write functions, a *termios* structure is also used with values VTIME and VMIN set to *connectionParameters.timeout* * 10 and 0 respectively.<br>
*connectionParameters* that contains important information used for the function's protocol, that goes as follows:
- The transmitter emits a SET frame and waits for the receiver's response. Once an acknowledgement frame (UA frame) sent by the receiver is received, the connection has been correctly estabilished and *llopen* closes. If no acknowledgement frame is received by the transmitter during a set timeout period (parameter defined by *connectionParameters*'s *timeout* value), it retries the protocol, sending a SET frame and waiting for an UA frame again. If this does not happen after the number of retries set by the *connectionParameters*'s *nRetransmissions* value, *llopen* ends with an error number, and the program closes.

        if (connectionParameters.role == LlTx) {

            while (nTries++ < connectionParameters.nRetransmissions) {
                sendSUFrame(fd, LlTx, CTRL_SET);
                
                if (readSUFrame(fd, LlTx) == CTRL_UA) {
                    printf("Received acknowledgement frame. Continuing...\n\n");
                    return 1;
                }
            }
            return -2;
        }

- The receiver waits for a SET frame reception amd acknowledges it by sending a UA frame back.
    
        if (connectionParameters.role == LlRx) {
            //wait for frame to arrive
            while (readSUFrame(fd, LlTx) != CTRL_SET) { }
            sendSUFrame(fd, LlTx, CTRL_UA);
        }

#

## 5.2 - **llwrite** 

    int llwrite(const unsigned char *buf, int bufSize)

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*llwrite*'s goal is to send information to the receiver in an information frame and, because of this, is exclusively used by the transmitter (no information frame is ever sent by the receiver).<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Initially, it writes to the serial port using a function called *sendIFrame*. This auxiliary function takes the serial port file descriptor (global variable in our link layer code file), the current protocol's sequence number, the file buffer and its filesize and, after building an information frame with these latter 3 parameters, writes it to the file descriptor. This information frame follows the same structure every time: 
- a starting flag (1 octet), 
- the address of the information (1 octet),
- the control field that carries the sequence number (1 octet),
-  a BCC calculated through an XOR between the address and the control field (1 octet), 
- every data byte contained in the information buffer being sent (buffer size octets), 
- another BCC calculated through an XOR between every data octet being sent, 
- and an ending flag. 
To ensure the receiver always understands where the first and last flags are (delimiters of the frame), a byte stuffing mechanism is implemented, and its job is to escape any flag or escape byte contained in the frame by replacing it with two bytes: an escape byte 0x7e followed by a byte calculated through an XOR between the escaped byte and 0x20.

After this, there are 3 major ways in which the program can proceed, depending on the receiver's answer:
- The transmitter receives a supervision frame with a RR control field within the previously estabilished *ll_connectionParameters.timeout* and the next sequence number. With this, the transmitter knows the reeceiver acknowledged the information frame positively, since it deemed it correct (it contained no header errors, its sequence number is correct and there were no errors in the data portion of the frame). With this, *llwrite* closes successfully and returns the number of bytes were sent (size of the information frame).
- The transmitter receives a supervision frame with a REJ control field with an unchanged sequence number. This means the information frame sent was not acknowledged correctly and hence needs to be re-sent. The retry number is reset to 0, and the protocol restarts from scratch.
- The transmitter receives any other type of response, and re-sends the frame, awaiting a correctly acknowledged RR response.
The retry number is increased, and the protocol restarts.

This back-and-forth is wrapped in a *for* loop that repeats until the number of retries is the same as the previously estabilished *ll_connectionParameters.nRetransmissions*. If this for loop reaches its end condition, the program returns an error number. 

    for (int nTries = 0; nTries < ll_connectionParameters.nRetransmissions; nTries++) {
        //printf("Sending Iframe...\n");
        if ((bytes = sendIFrame(fd, buf, bufSize, old_seqNum)) < 0) {
            printf("Error sending Iframe.\n\n");
            continue;
        }

        response = readSUFrame(fd, ll_connectionParameters.role);
        //printf("response: %d\n", response);

        if (response == CTRL_RR(next_seqNum)) {
            //printf("Iframe acknowledged correctly. Continuing...\n\n");
            seqNum = next_seqNum;
            return bytes;
        }
        else if (response == CTRL_REJ(old_seqNum)) {   
            printf("Iframe rejected (wrong BCC2).\n\n"); 
            //seqNum = next_seqNum;
            nTries = 0;
            continue;
        }
       
        else if (response == 0) {
            printf("No SUframe received (timeout).\n\n");
            continue; 
        }
    }
    printf("LLWrite failure (too many tries).\n\n");
    return -1;

#

## 5.3 - **llread**

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*llread*'s goal is to read an information frame sent by the transmitter and posteriorly fill the received *packet* parameter with the data contained inside the information frame. It is also the only place where the sequence number is changed depending on the flow of the function.


# 6. Application protocol

<br>

# 7. Validation

<br>

# 8. Data Link Protocol Efficiency

<br>

# 9. Conclusions

## Annex: Code
