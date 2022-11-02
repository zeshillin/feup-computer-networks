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
