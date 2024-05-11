#ifndef MAIN_H_
#define MAIN_H

/* BIBLIOTECAS */
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

/* VARIABLES DE LOG y CONFIG */
t_log* logger_cpu;
t_config* config_cpu;
pcb* PCB;
bool hay_interrupcion;

/* SEMAFOROS */
sem_t sem_recibir_pcb;
sem_t sem_execute;
sem_t sem_interrupcion;

/* PARAMETROS CONFIG */
char* ip_propio;
char* puerto_cpu_dispatch; 
char* puerto_cpu_interrupt; 
char* ip_memoria;
char* puerto_memoria;

/* VARIABLES DE CONEXION */
int fd_conexion_memoria;
int fd_escucha_dispatch;
int fd_escucha_interrupt;
int fd_cpu_dispatch;
int fd_cpu_interrupt;

/* REGISTROS */
typedef struct{
	uint32_t* pc;
    uint8_t*  ax;
	uint8_t*  bx;
	uint8_t*  cx;
	uint8_t*  dx;
	uint32_t* eax;
	uint32_t* ebx;
	uint32_t* ecx;
	uint32_t* edx;
	uint32_t* si;
	uint32_t* di;
} t_registros;

/* FUNCIONES DE CPU */
void terminar_programa();
void leer__configuraciones();
void inicializar_semaforos();
void dispatch();
void fetch ();
void decode (t_linea_instruccion* instruccion, pcb* PCB);
t_linea_instruccion* prox_instruccion(int pid, int program_counter);
void terminar_programa();

/* EXECUTE Ejecutar instrucciones */
void ejecutar_set(pcb* PCB, char* parametro1, char* parametro2);
bool medir_registro(char* registro);
void setear_registro8(pcb* PCB, char* registro, uint8_t valor);
void setear_registro32(pcb* PCB, char* registro, uint32_t valor);
uint8_t capturar_registro8(pcb* PCB, char* registro);
uint32_t capturar_registro32(pcb* PCB, char* registro);
void ejecutar_mov_in();
void ejecutar_mov_out();
void ejecutar_sum(pcb* PCB, char* parametro1, char* parametro2);
void ejecutar_sub(pcb* PCB, char* parametro1, char* parametro2);
void ejecutar_jnz(pcb* PCB, char* parametro1, char* parametro2);
void ejecutar_resize();
void ejecutar_copy_string();
void ejecutar_wait(pcb* PCB, char* registro);
void ejecutar_signal(pcb* PCB, char* registro);
void ejecutar_io_gen_sleep(char* parametro1, char* parametro2);
void ejecutar_io_stdin_read();
void ejecutar_io_stdout_write();
void ejecutar_io_fs_create();
void ejecutar_io_fs_delete();
void ejecutar_io_fs_truncate();
void ejecutar_io_fs_write();
void ejecutar_io_fs_read();
void ejecutar_exit(pcb* PCB);
void ejecutar_error(pcb* PCB);
void enviar_recurso_por_signal(char * recurso, int fd_escucha_dispatch, op_code OPERACION);
void enviar_recurso_por_wait(char * recurso, int fd_escucha_dispatch, op_code OPERACION);

/* CHECK INTERRUPT */
void check_interrupt();
void* interrupcion(void *arg);
void detectar_motivo_desalojo();


#endif
