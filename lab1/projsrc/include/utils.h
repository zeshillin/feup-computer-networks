#ifndef _UTILS_H_
#define _UTILS_H_

// linklayer utils
#define FLAG 0x7E

#define ADD_TX_AND_BACK 0x03
#define ADD_RX_AND_BACK 0x01

#define CTRL_SET 0x03
#define CTRL_DC  0x0B
#define CTRL_UA  0x07
#define CTRL_RR(n) ((n << 7) | 0x05)
#define CTRL_REJ(n) ((n << 7) | 0x01)
#define SEQNUM_TO_CONTROL(n) ((1 & n) << 6)

#define ESC 0x7D
#define ESC_SUB 0x5D

// applayer utils

#define MAX_PACKSIZE 1024

#define CF_START 2
#define CF_END 3
#define CF_DATA 1

#define T_FILESIZE 0
#define T_FILENAME 1

#endif // _UTILS_H_

