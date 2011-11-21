/*********************************************
 kasm.h
 ************************************************/

#include "defs.h"


// Sets the IDTR register
void _lidt(IDTR *idtr);

// Restarts the PC
void _restart();

// Sets the cursor
void _setCursor(int a);

/* Writes PIC1's mask */
void _mascaraPIC1(byte mascara);
/* Writes PIC2's mask */
void _mascaraPIC2(byte mascara);

/* Disables interrupts */
void _Cli(void);
/* Enables interrutps  */
void _Sti(void);

void 		_int_00_hand();
void 		_int_01_hand();
void 		_int_02_hand();
void 		_int_03_hand();
void 		_int_04_hand();
void 		_int_05_hand();
void 		_int_06_hand();
void 		_int_07_hand();
void 		_int_08_hand();
void 		_int_09_hand();
void 		_int_0A_hand();
void 		_int_0B_hand();
void 		_int_0C_hand();
void 		_int_0D_hand();
void 		_int_0E_hand();
void 		_int_0F_hand();
void 		_int_10_hand();
void 		_int_11_hand();
void 		_int_12_hand();
void 		_int_13_hand();
void 		_int_14_hand();
void 		_int_15_hand();
void 		_int_16_hand();
void 		_int_17_hand();
void 		_int_18_hand();
void 		_int_19_hand();
void 		_int_1A_hand();
void 		_int_1B_hand();
void 		_int_1C_hand();
void 		_int_1D_hand();
void 		_int_1E_hand();
void 		_int_1F_hand();

// void		_int_20_hand();      		/* Timer tick */
// void		_int_21_hand();			/* Teclado */

/* Timer tick */
void _KB_hand();
/* Keyboard */
void _timer_tick_hand();
/* Handler INT 80h */
void _int_80_hand(int systemCall, int fd, char *buffer, int count);
// For signals
void _int_79_hand();

// RDTSC
int _rdtsc();

/* System call write */
void _write ( int fd, char *buffer, int count );

/* System call read */
void _read ( int fd, void *buffer, size_t count );

/* Assembly's out */
int _out(unsigned int port, int value);

/* Assembly's in */
int _in(unsigned int port);

/* Call for debug */
void _debug(void);