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

## Data Link Layer:
The Data Link Layer is the lowest level of the application and is responsible for supervising direct communication with the serial port, detecting errors and handling them.

## Application Layer: 
The application layer is above the link layer in terms of leveling and is responsible for handling file contents and sending them through the link layer. Information is retrieved from a file and sent through the link layer, which is then received in this same link layer: the application layer simply takes care of unpacking it in the receiver's end.

## Main 
Simple interface that runs transmitter or receiver code depending on the role given during the program execution: 
    
&nbsp;&nbsp;&nbsp;&nbsp;*serialport role [filepath]*  

where:
- *serialport* is the port's path (e.g.: dev/ttyS01)
- *role* is the role (*{rx, tx}* where *rx* is the receiver, *tx* is the transmitter)
- *[filepath] is the file-to-send's path (only needed if initializing the program as transmitter)

<br>

# 3. Code Structure

## Application Layer
The application layer stores every function and usable without any regard for who is using it, be it is the receiver or the transmitter. Important parameters for the application layer to know about would be the file descriptor and the role it is currently executing, however, these are both specified in *main.c* and, as such, there is no need for them.

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

<br>

# 5. Data Link Protocol

<br>

# 6. Application protocol

<br>

# 7. Validation

<br>

# 8. Data Link Protocol Efficiency

<br>

# 9. Conclusions

## Annex: Code
