#include "../../include/kernel.h"
#include "../../include/kasm.h"
#include "../../include/defs.h"


#include "video.h"
#include "kernel.h"
#include "tty.h"
#include "scheduler.h"
#include "fd.h"
#include "fs/cache.h"
#include "fs/hdd.h"

#include "paging/page.h"
// #include "paging/mall.h"
// #include "mem/mem.h"


///////////// Inicio de Variables del Kernel

// extern struct PagingNamespace Paging;
/* IDT de 80h entradas*/
DESCR_INT idt[0x101]; 
/* IDTR */
IDTR idtr; 

// Clears the kernel passage variables.
void clear_kernel_buffer() {
	int i = 0;
	for(i = 0; i < KERNEL_BUFFER_SIZE; ++i)	{
		kernel_buffer[i] = 0;
	}
}

void* getPidStack(int pid){
	return process_getbypindex(pid)->stack;
}

/*************************************************************
*initialize_pics
* Inicializa los PICS, el 1 como Master el 2 como slave
* y les coloca los offset enviados
* Recibe: Offset1 para el PIC1
*	  Offset2 para el PIC2
**************************************************************/
void initialize_pics(int offset_pic1, int offset_pic2){
	unsigned char mask1 = _inb(PIC1_DATA);
	unsigned char mask2 = _inb(PIC2_DATA);	

	_outb(PIC1_COMMAND, ICW1);
	_outb(PIC2_COMMAND, ICW1);

	_outb(PIC1_DATA, offset_pic1);
	_outb(PIC2_DATA, offset_pic2);
	
	_outb(PIC1_DATA, 4);
	_outb(PIC2_DATA, 2);

	_outb(PIC1_DATA, ICW4_8086);
	_outb(PIC2_DATA, ICW4_8086);

	_outb(PIC1_DATA, mask1);
	_outb(PIC2_DATA, mask2);
}
unsigned char *exception_messages[] =
{
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};
int rxz=0;
void fault_handler(struct regs *r)
{
    // if (r->int_no < 32)
    //   {
    //       printf("%s", exception_messages[r->int_no]);
    //       printf(" Exception.\n");
    //   }
	char esp=r->int_no;
	// printf("killin process"); Nunca va a funcionar xq no se lo ejecuta desde una terminal
	sigkill_h(getp());
	int a=0;
	for(a;a<strlen(exception_messages[esp]);a++){
		*(char*)(0xb8410 + 80*2*(rxz+1) + 2*a) = exception_messages[esp][a];
	}
	// *(char*)(0xb8410 + 80*2*rxz) = esp % 10 + '0';
	// *(char*)(0xb840e + 80*2*rxz) = (esp / 10) % 10 + '0';
	// *(char*)(0xb840c + 80*2*rxz) = (esp / 100) % 10 + '0';
	// *(char*)(0xb840a + 80*2*rxz) = (esp / 1000) % 10 + '0';
	// *(char*)(0xb8408 + 80*2*rxz) = (esp / 10000) % 10 + '0';
	// *(char*)(0xb8406 + 80*2*rxz) = (esp / 100000) % 10 + '0';
	// *(char*)(0xb8404 + 80*2*rxz) = (esp / 1000000) % 10 + '0';
	// *(char*)(0xb8402 + 80*2*rxz) = (esp / 10000000) % 10 + '0';
	// *(char*)(0xb8400 + 80*2*rxz) = (esp / 100000000) % 10 + '0';
    	//     	}
	rxz++;
	_Sti();
}
///////////// Fin de Variables del Kernel

///////////// Inicio de funciones auxiliares del Kernel.

/*
 *	setup_IDT_entry
 * 		Inicializa un descriptor de la IDT
 *
 *	Recibe: Puntero a elemento de la IDT
 *	 Selector a cargar en el descriptor de interrupcion
 *	 Puntero a rutina de atencion de interrupcion
 *	 Derechos de acceso del segmento
 *	 Cero
 */
void setup_IDT_entry(DESCR_INT *item, byte selector, dword offset, byte access, byte cero) {
	item->selector = selector;
	item->offset_l = offset & 0xFFFF;
	item->offset_h = offset >> 16;
	item->access = access;
	item->cero = cero;
}

///////////// Fin de funciones auxiliares del kernel.

///////////// Inicio Handlers de interrupciones.
block_t kernelMem=NULL;
int krn = 0;

int ready = 0;

// Sets the kernel as ready
void setready() {
	ready = 1;
}
block_t getKernelMem(){
	return kernelMem;
}

void setKernelMem(block_t aux){
	if(kernelMem==NULL)
		kernelMem=aux;
}
// Tells if the kernel is ready
int kernel_rd() {
	return ready;
}

// Tells if the kernel is ready to the keyboard
int kernel_ready() {
 	_in(0x60);	 // Needed for not blcoking the keyboard on idleness
	return ready;
}

// Keyboard handler
void int_09() {
	krn++;
	char scancode;
	scancode = _in(0x60);

	// We check if the scancode is a char or a control key.
	int flag = scancode >= 0x02 && scancode <= 0x0d;
	flag = flag || (scancode >= 0x10 && scancode <= 0x1b);
	flag = flag || (scancode >= 0x1E && scancode <= 0x29);
	flag = flag || (scancode >= 0x2b && scancode <= 0x35);
	if (flag)	{
		char sc = scanCodeToChar(scancode);
		if(sc != 0 && sc != EOF)	{
			pushC(sc); 		  //guarda un char en el stack
		}
	}
	else {
		controlKey(scancode); // Envia el scancode al analizador de control keys.
	}
	
	kernel_buffer[0] = KILL;
	
	krn--;
}

// Tells the other kernel functions if the code they are using is in the kernel or not.
int in_kernel(){
	return krn;
}

// INT 80h Handler, kernel entry.
void int_80() {
	if(krn)	{
		return;
	}
	
	krn++;
	int systemCall = kernel_buffer[0];
	int fd         = kernel_buffer[1];
	int buffer     = kernel_buffer[2];
	int count      = kernel_buffer[3];

	
	int i, j;
	Process * current;
	Process * p;
	int inode;
	int _fd;
	// Yeah, wanna know why we don't access an array directly? ... Because of big bugs we might have.
	switch(systemCall) {
		case READY:
			kernel_buffer[KERNEL_RETURN] = kernel_ready();
			break;
		case WRITE:
 			current = getp();
			kernel_buffer[KERNEL_RETURN] = fd_write(current->file_descriptors[fd],(char *)buffer,count);
			break;
		case READ:
			current = getp();
			kernel_buffer[KERNEL_RETURN] = fd_read(current->file_descriptors[fd],(char *)buffer,count);
			break;
		case MKFIFO:
			_fd = process_getfreefd();
			fd = fd_open(_FD_FIFO, (void *)kernel_buffer[1],kernel_buffer[2]);
			if(_fd != -1 && fd != -1)	{
				getp()->file_descriptors[_fd] = fd;
				kernel_buffer[KERNEL_RETURN] = _fd;
			}
			else {
				kernel_buffer[KERNEL_RETURN] = -1;
			}
			break;
		case OPEN:
			_fd = process_getfreefd();
			fd = fd_open(_FD_FILE, (void *) kernel_buffer[1], kernel_buffer[2]);
			if(_fd != -1 && fd >= 0)
			{
				getp()->file_descriptors[_fd] = fd;
				kernel_buffer[KERNEL_RETURN] = _fd;
			}
			else {
				kernel_buffer[KERNEL_RETURN] = fd;
			}
			break;
		case CLOSE:
			kernel_buffer[KERNEL_RETURN] = fd_close(getp()->file_descriptors[fd]);
			break;
		case PCREATE:
			kernel_buffer[KERNEL_RETURN] = sched_pcreate(kernel_buffer[1],kernel_buffer[2],kernel_buffer[3]);
			break;
		case PRUN:
			kernel_buffer[KERNEL_RETURN] = sched_prun(kernel_buffer[1]);
			break;
		case PDUP2:
			kernel_buffer[KERNEL_RETURN] = sched_pdup2(kernel_buffer[1],kernel_buffer[2],kernel_buffer[3]);
			break;
		case GETPID:
			kernel_buffer[KERNEL_RETURN] = sched_getpid();
			break;
		case WAITPID:
			kernel_buffer[KERNEL_RETURN] = sched_waitpid(kernel_buffer[1]);
			break;
		case PTICKS:
			kernel_buffer[KERNEL_RETURN] = (int) storage_index();
			break;
		case PNAME:
			p = process_getbypid(kernel_buffer[1]);
			if(p == NULL)
			{
				kernel_buffer[KERNEL_RETURN] = (int) NULL;
			} else {
				kernel_buffer[KERNEL_RETURN] = (int) p->name;
			}
			break;
		case PSTATUS:
			p = process_getbypid(kernel_buffer[1]);
			if(p == NULL)
			{
				kernel_buffer[KERNEL_RETURN] = (int) -1;
			} else {
				kernel_buffer[KERNEL_RETURN] = (int) p->state;
			}
			break;
		case PPRIORITY:
			p = process_getbypid(kernel_buffer[1]);
			if(p == NULL)
			{
				kernel_buffer[KERNEL_RETURN] = (int) -1;
			} else {
				kernel_buffer[KERNEL_RETURN] = (int) p->priority;
			}
			break;
		case PGID:
			p = process_getbypid(kernel_buffer[1]);
			if(p == NULL)
			{
				kernel_buffer[KERNEL_RETURN] = (int) -1;
			} else {
				kernel_buffer[KERNEL_RETURN] = (int) p->gid;
			}
			break;
		case PGETPID_AT:
			p = process_getbypindex(kernel_buffer[1]);
			if (p->state != -1) {
				kernel_buffer[KERNEL_RETURN] = (int) p->pid;
			} else {
				kernel_buffer[KERNEL_RETURN] = -1;
			}
			break;
		case KILL:
			kernel_buffer[KERNEL_RETURN - 1] = kernel_buffer[1];
			kernel_buffer[KERNEL_RETURN - 2] = kernel_buffer[2];
			break;
		case PSETP:
			p = process_getbypid(kernel_buffer[1]);
			if(p == NULL)	{
				kernel_buffer[KERNEL_RETURN] = (int) -1;
			} else {
				if(kernel_buffer[2] <= 4 && kernel_buffer[2] >= 0)	{
					p->priority = kernel_buffer[2];
				}
				kernel_buffer[KERNEL_RETURN] = (int) p->gid;
			}
			break;
		case SETSCHED:
			sched_set_mode(kernel_buffer[1]);
			break;
		case PWD:
			kernel_buffer[KERNEL_RETURN] = (int) fs_pwd();
			break;
		case CD:
			kernel_buffer[KERNEL_RETURN] = (int) fs_cd(kernel_buffer[1]);
			break;
		case FINFO:
			fs_finfo(kernel_buffer[1], kernel_buffer[2]);
			break;
		case MOUNT:
			fs_init();
			break;
		case MKDIR:
			kernel_buffer[KERNEL_RETURN] = (int) fs_mkdir(kernel_buffer[1],current_ttyc()->pwd);
			break;
		case RM:
			inode = fs_indir(kernel_buffer[1],current_ttyc()->pwd);
			if (inode) {
				kernel_buffer[KERNEL_RETURN] = (int) fs_rm(inode,0);
			} else {
				kernel_buffer[KERNEL_RETURN] = ERR_NO_EXIST;
			}
			break;
		case GETUID:
			if(kernel_buffer[1] == 0)
			{
				kernel_buffer[KERNEL_RETURN] = (int) current_ttyc()->uid;
			} else {
				kernel_buffer[KERNEL_RETURN] = (int) user_exists(kernel_buffer[1]);
			}
			break;
		case GETGID:
			if(kernel_buffer[1] == 0)
			{
				kernel_buffer[KERNEL_RETURN] = (int) user_gid(current_ttyc()->uid);
			} else {
				kernel_buffer[KERNEL_RETURN] = (int) user_gid(kernel_buffer[1]);
			}

			break;
		case MAKEUSER:
			kernel_buffer[KERNEL_RETURN] = user_create(kernel_buffer[1],
							kernel_buffer[2], user_gid(current_ttyc()->uid));
			break;
		case SETGID:
			kernel_buffer[KERNEL_RETURN] = user_setgid(kernel_buffer[1], 
													   kernel_buffer[2]);
			break;
		case UDELETE:
			kernel_buffer[KERNEL_RETURN] = user_delete(kernel_buffer[1]);
			break;
		case UEXISTS:
			kernel_buffer[KERNEL_RETURN] = user_exists(kernel_buffer[1]);
			break;
		case ULOGIN:
			kernel_buffer[KERNEL_RETURN] = user_login(kernel_buffer[1], 
													  kernel_buffer[2]);
			break;
		case ULOGOUT:
			kernel_buffer[KERNEL_RETURN] = user_logout();
			break;
		case CHOWN:
			kernel_buffer[KERNEL_RETURN] = fs_chown(kernel_buffer[1],
													kernel_buffer[2]);
			break;
		case CHMOD:
			kernel_buffer[KERNEL_RETURN] = fs_chmod(kernel_buffer[1],
												    kernel_buffer[2]);
			break;
		case GETOWN:
			kernel_buffer[KERNEL_RETURN] = fs_getown(kernel_buffer[1]);
			break;
		case GETMOD:
			kernel_buffer[KERNEL_RETURN] = fs_getmod(kernel_buffer[1]);
			break;
		case CP:
			kernel_buffer[KERNEL_RETURN] = fs_cp(kernel_buffer[1], kernel_buffer[2], current_ttyc()->pwd, current_ttyc()->pwd);
			break;
		case MV:
			kernel_buffer[KERNEL_RETURN] = fs_mv(kernel_buffer[1], kernel_buffer[2], current_ttyc()->pwd);
			break;
		case LINK:
			kernel_buffer[KERNEL_RETURN] = fs_open_link(kernel_buffer[1], kernel_buffer[2], current_ttyc()->pwd);
			break;
		case FSSTAT:
			kernel_buffer[KERNEL_RETURN] = fs_stat(kernel_buffer[1]);
			break;
		case SLEEP:
			kernel_buffer[KERNEL_RETURN] = scheduler_sleep(kernel_buffer[1]);
			break;
		default:
			break;
	}
	
	krn--;
}

// This call is done outside the kernel, since we had maaaaaaaany problems with it inside #nofun
// Fires a signal after a syscall, only if the kernel has been set to do so.
void signal_on_demand() {
	if (kernel_buffer[KERNEL_RETURN - 1] != 0) {
		
		if (kernel_buffer[0] == KILL) {
			make_atomic();
			int sigcode = kernel_buffer[KERNEL_RETURN - 1];
			int pid = kernel_buffer[KERNEL_RETURN - 2];
		
			sg_handle(sigcode, pid);
			release_atomic();
		}
		kernel_buffer[KERNEL_RETURN - 1] = 0; // SIGCODE
		kernel_buffer[KERNEL_RETURN - 2] = 0; // PID
	}

}

///////////// Fin Handlers de interrupciones.

int val = 0;
Process * p1, * idle, * kernel;


// Starts the kernel's idle process. This process has kernel permissions.
int idle_main(int argc, char ** params) {
	
	int i = 0;
	char a[] = { ' ', 0x07 };
	
	for(; i < 2000; ++i)	{
		memcpy((char*)0xb8000 + i * 2, a, 2);
	}
	
	// The funny message, windows says starting windows... why shouldn't we? D:
	char start_msg[][2] =	{ 
		{ 'S', 0x07 },
		{ 't', 0x08 },
		{ 'a', 0x09 },
		{ 'r', 0x0a },
		{ 't', 0x0b },
		{ 'i', 0x0c },
		{ 'n', 0x0d },
		{ 'g', 0x0e },
		{ ' ', 0x0f },
		{ 'M', 0x02 },
		{ 'o', 0x03 },
		{ 'n', 0x04 },
		{ 'i', 0x05 },
		{ 'x', 0x06 },
		{ ' ', 0x0f },
		{ '\001', 0x5f },
	};
	
	i = 0;
	for(; i < 16; ++i)	{
		memcpy((char*)0xb8000 + i * 2, start_msg[i], 2);
		_setCursor(i);
	}
	
	Cli();
	make_atomic();

	mount();					// Mount or start the FS

	tty_init(0);				// Load up the TTY's
	tty_init(1);
	tty_init(2);
	tty_init(3);
	tty_init(4);
	tty_init(5);		
	
	setready(); 				// Set the kernel as ready and the FS as loaded
	users_init();				// Init the users
	
	fs_finish();

	release_atomic();	
	Sti();


	while(1) {
		_Halt(); // Now set to idle.
	}
}

void _rtc();

///////////// Inicio KMAIN

/**********************************************
 kmain()
 Punto de entrada de código C.
 *************************************************/
kmain() {
	int i, num;
	// Paging.start(0x200000);
	// init_malloc();
	
	initialize_pics(0x20,0x70);
	setup_IDT_entry(&idt[0x70], 0x08, (dword) & _rtc, ACS_INT, 0);

	_cache_init();
	hdd_init();
	/* CARGA DE IDT CON LA RUTINA DE ATENCION DE IRQ0    */
	
	setup_IDT_entry (&idt[0x00], 0x08, (dword)&_int_00_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x01], 0x08, (dword)&_int_01_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x02], 0x08, (dword)&_int_02_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x03], 0x08, (dword)&_int_03_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x04], 0x08, (dword)&_int_04_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x05], 0x08, (dword)&_int_05_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x06], 0x08, (dword)&_int_06_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x07], 0x08, (dword)&_int_07_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x08], 0x08, (dword)&_int_08_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x09], 0x08, (dword)&_int_09_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x0A], 0x08, (dword)&_int_0A_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x0B], 0x08, (dword)&_int_0B_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x0C], 0x08, (dword)&_int_0C_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x0D], 0x08, (dword)&_int_0D_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x0E], 0x08, (dword)&_int_0E_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x0F], 0x08, (dword)&_int_0F_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x10], 0x08, (dword)&_int_10_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x11], 0x08, (dword)&_int_11_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x12], 0x08, (dword)&_int_12_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x13], 0x08, (dword)&_int_13_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x14], 0x08, (dword)&_int_14_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x15], 0x08, (dword)&_int_15_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x16], 0x08, (dword)&_int_16_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x17], 0x08, (dword)&_int_17_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x18], 0x08, (dword)&_int_18_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x19], 0x08, (dword)&_int_19_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x1A], 0x08, (dword)&_int_1A_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x1B], 0x08, (dword)&_int_1B_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x1C], 0x08, (dword)&_int_1C_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x1D], 0x08, (dword)&_int_1D_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x1E], 0x08, (dword)&_int_1E_hand, ACS_INT, 0);
	setup_IDT_entry (&idt[0x1F], 0x08, (dword)&_int_1F_hand, ACS_INT, 0);
	// setup_IDT_entry (&idt[0x20], 0x08, (dword)&_int_20_hand, ACS_INT, 0);
	// setup_IDT_entry (&idt[0x21], 0x08, (dword)&_int_21_hand, ACS_INT, 0);

	setup_IDT_entry(&idt[0x20], 0x08, (dword) & _timer_tick_hand, ACS_INT, 0);

	/* CARGA DE IDT CON LA RUTINA DE ATENCION DE IRQ1    */

	setup_IDT_entry(&idt[0x21], 0x08, (dword) & _KB_hand, ACS_INT, 0);

	/* CARGA DE IDT CON LA RUTINA DE ATENCION DE int80h    */

	setup_IDT_entry(&idt[0x80], 0x08, (dword) & _int_80_hand, ACS_INT, 0);
	
	/* CARGA DE IDT CON LA RUTINA DE ATENCION DE int79h    */

	setup_IDT_entry(&idt[0x79], 0x08, (dword) & _int_79_hand, ACS_INT, 0);

	/* Carga de IDTR */

	idtr.base = 0;
	idtr.base += (dword) & idt;
	idtr.limit = sizeof(idt) - 1;

	_lidt(&idtr);	
	
	
	Cli();
	int rate = 0x06;
	_outb(0x70, 0x0A); //set index to register A
	char prev=_inb(0x71); //get initial value of register A
	_outb(0x70, 0x0A); //reset index to A
	_outb(0x71, (prev & 0xF0) | rate); //write only our rate to A. Note, rate is the bottom 4 bits.
	
		
		init_paging();
	scheduler_init();



	/* Habilito interrupcion de timer tick*/
	_mascaraPIC1(0xFC);
	_mascaraPIC2(0xFE);
	
	_outb(0x70, 0x0B); //set the index to register B
	prev= _inb(0x71); //read the current value of register B
	_outb(0x70, 0x0B); //set the index again(a read will reset the index to register D)
	_outb(0x71, prev | 0x40); //write the previous value or'd with 0x40. This turns on bit 6 of register B

	Sti();




	idle = create_process("idle", idle_main, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
	
	

	// We soon exit out of here :)
	while (1);

}

///////////// Fin KMAIN
