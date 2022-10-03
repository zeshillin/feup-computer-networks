// flag-specific bytes

// control field flags
#define FLAG 01111110

#define ADD_TX_AND_BACK 0x03
#define ADD_RX_AND_BACK 0x01

#define CTRL_SET 0x03
#define CTRL_DC  0x0B
#define CTRL_UA  0x07
#define CTRL_RR(n) (n << 8) | 0x05
#define CTRL_REJ(n) (n << 8) | 0x01

#define ESC 0x7D
#define ESC_SUB 0x5D
