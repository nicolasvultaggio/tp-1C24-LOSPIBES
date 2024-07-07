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
t_list * lista_interrupciones;

typedef struct{
	int pid;
	motivo_desalojo motivo;
}element_interrupcion;

/* TLB */
int MAX_TLB_ENTRY;
int TAM_PAGINA;
t_list* translation_lookaside_buffer;
pthread_mutex_t mutex_tlb;
pthread_mutex_t mutex_lista_interrupciones;
/* SEMAFOROS */
pthread_mutex_t mutex_motivo_x_consola; //what este para que es
sem_t sem_recibir_pcb;
sem_t sem_execute;
sem_t sem_interrupcion;

/* PARAMETROS CONFIG */
char* ip_propio;
char* puerto_cpu_dispatch; 
char* puerto_cpu_interrupt; 
char* ip_memoria;
char* puerto_memoria;
char* cantidad_entradas_tlb;
char* algoritmo_tlb;

/*Puntero para atender interrupciones*/
//motivo_desalojo* interrupcion_actual;

/* VARIABLES DE CONEXION */
int fd_conexion_memoria;
int fd_escucha_dispatch;
int fd_escucha_interrupt;
int fd_cpu_dispatch;
int fd_cpu_interrupt;

/*FLAGS PARA MANEJO DE INTERRUPCION*/

//todas estas flags tendran que actualizarse cada vez que se ejecuta una intstruccion
//bool es_exit; //siempre mofificar
//bool es_bloqueante; //modificar siempre que es_exit = false
//bool error_memoria; // modificar solo en mov in y mov out
//bool es_wait; //modificar si se pone a bloqueante = true
//bool cambio_proceso_wait; // modificar si se pone a es_wait = true
//bool es_resize; //modificar si se pone bloqueante = true
//bool resize_desalojo_outofmemory; //modificar si se pone es_resize = true
//bool hay_interrupcion_x_consola = false; //al inicio no tiene ninguna => false


bool hubo_desalojo;
bool wait_o_signal;
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
t_linea_instruccion* prox_instruccion(int pid, uint32_t program_counter);
void terminar_programa();
void inicializar_tlb();

/* EXECUTE Ejecutar instrucciones */
void ejecutar_set(pcb* PCB, char* parametro1, char* parametro2);
void ejecutar_mov_in();
void ejecutar_mov_out();
void ejecutar_sum(pcb* PCB, char* parametro1, char* parametro2);
void ejecutar_sub(pcb* PCB, char* parametro1, char* parametro2);
void ejecutar_jnz(pcb* PCB, char* parametro1, char* parametro2);
void ejecutar_resize(char* tamanio);
void ejecutar_copy_string();
void ejecutar_wait(pcb* PCB, char* registro);
void ejecutar_signal(pcb* PCB, char* registro);
void ejecutar_io_gen_sleep(pcb* PCB, char* instruccion, char* interfaz, char* unidad_de_tiempo);
void ejecutar_io_stdin_read(char * nombre_interfaz, char * registro_direccion, char * registro_tamanio);
void ejecutar_io_stdout_write(char * nombre_interfaz, char * registro_direccion, char * registro_tamanio);
void ejecutar_io_fs_create(char * nombre_interfaz,char * nombre_archivo);
void ejecutar_io_fs_delete(char * nombre_interfaz,char * nombre_archivo);
void ejecutar_io_fs_truncate(char * nombre_interfaz,char * nombre_archivo,char * registro_tamanio);
void ejecutar_io_fs_write(char * nombre_interfaz,char * nombre_archivo,char * registro_direccion,char * registro_tamanio , char * registro_puntero_archivo);
void ejecutar_io_fs_read(char * nombre_interfaz,char * nombre_archivo,char * registro_direccion,char * registro_tamanio , char * registro_puntero_archivo);
void ejecutar_exit(pcb* PCB);
void ejecutar_error(pcb* PCB);

void enviar_recurso_por_signal(char * recurso, int fd_escucha_dispatch, op_code OPERACION);
void enviar_recurso_por_wait(char * recurso, int fd_escucha_dispatch, op_code OPERACION);

bool es_entrada_TLB_de_PID(void * un_nodo_tlb );
size_t size_registro(char * registro);
void setear_registro(pcb * PCB, char * registro, uint8_t valor8, uint32_t valor32);
void * capturar_registro(char * registro);
uint32_t recibir_lectura_memoria();
t_list * obtener_traducciones(uint32_t direccion_logica_i, uint32_t tamanio_a_leer );

int solicitar_frame_memory(int numero_pagina);

element_interrupcion * recibir_motiv_desalojo(int fd_escucha_interrupt);
element_interrupcion * seleccionar_interrupcion();

bool encontrar_interrupcion_por_fin_de_consola(void* elemento);
bool encontrar_interrupcion_por_fin_de_quantum(void* elemento);

/* CHECK INTERRUPT */
void check_interrupt();
void* interrupcion(void *arg);
nodo_tlb * administrar_tlb( int PID, int numero_pagina, int marco);
uint32_t MMU( uint32_t direccion_logica);

int tam_pagina;
#endif


/*
 //vieja logica de check interrupt
		if (!es_exit){
			if(*interrupcion_actual == FIN_QUANTUM){ //si la interrupcion es por desalojo de quantum
				if(!es_bloqueante){ // se sabe que no se desalojo al proceso previamente
					
					log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
					enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
					sem_post(&sem_recibir_pcb);
					liberar_interrupcion_actual();
					
					
				}else{ //pudo haberse desalojado al proceso
					
					if(es_wait){
						if(cambio_proceso_wait){
							log_info(logger_cpu,"Cambio el PCB POR WAIT, ahora es del proceso %d, ignoramos interrupcion por FIN DE QUANTUM",PCB->PID);
							sem_post(&sem_execute); //el pcb cambiado ya lo recibimos, tenemos que simplemente ponernos a ejecutar otro ciclo de instruccion
							liberar_interrupcion_actual();
						}else{
							log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
							enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
							sem_post(&sem_recibir_pcb);
							liberar_interrupcion_actual();
						}
					}else{//no es wait, puede ser una syscall bloqueante o resize
						if(es_resize){ 
							if(resize_desalojo_outofmemory){
								log_info(logger_cpu,"Resize ya había desalojado al proceso, ignoramos interrupcion por FIN DE QUANTUM");
								liberar_interrupcion_actual(); // syscall bloqueante ya se encargo de poner a escuchar por otro pcb
							}else{
								log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
								enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
								sem_post(&sem_recibir_pcb);
								liberar_interrupcion_actual();
							}
						}else{ //este caso es las syscalls bloqueantes, ni resize ni wait
							log_info(logger_cpu,"Syscall bloqueante ya había desalojado al proceso %d, ignoramos interrupcion por FIN DE QUANTUM",PCB->PID);
							liberar_interrupcion_actual();// syscall bloqueante ya se encargo de poner a escuchar por otro pcb
						}	
					}
				}
			}else{//fin de proceso
				if(!es_bloqueante){ // se sabe que no se desalojo al proceso previamente
					if(error_memoria){ //
						log_info(logger_cpu,"Como hubo un error de escritura del proceso %d, ignoramos el pedido de finalizacion porque ya va a finalizar por este error de escritura",PCB->PID);
						liberar_interrupcion_actual(); // no atendemos interrupcion ni ponemos ningun semaforo porque mov in y mov out ya desalojan y ponen a escuchar otro pcb
					}else{ // se sabe que no se desalojo al proceso previamente
						log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
						enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
						sem_post(&sem_recibir_pcb);
						liberar_interrupcion_actual();
					}
				}else{ //pudo haberse desalojado al proceso
					if(es_wait){
						if(cambio_proceso_wait){ //se desalojo el proceso
							//avisarle a kernel del desalojo?
							liberar_interrupcion_actual();
							sem_post(&sem_execute);
						}else{ //no se desalojo el proceso
							log_info(logger_cpu,"Interrupcion: Finalizacion del proceso: %d",PCB->PID);
							enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,EXITO,NULL,NULL, NULL, NULL, NULL);
							sem_post(&sem_recibir_pcb);
							liberar_interrupcion_actual();
							//pthread_mutex_lock(&mutex_motivo_x_consola);
							hay_interrupcion_x_consola = false;
							//pthread_mutex_unlock(&mutex_motivo_x_consola);
						}
					}else{//no es wait, puede ser una syscall bloqueante o resize
						if(es_resize){ 
							if(resize_desalojo_outofmemory){
								//avisarle a kernel del desalojo?na, ya lo va a finalizar solito
								liberar_interrupcion_actual();
							}else{//resize no desalojo al proceso, atender interrupcion
								log_info(logger_cpu,"Interrupcion: Finalizacion del proceso: %d",PCB->PID);
								enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,EXITO,NULL,NULL, NULL, NULL, NULL);
								sem_post(&sem_recibir_pcb);
								liberar_interrupcion_actual();
								//pthread_mutex_lock(&mutex_motivo_x_consola);
								hay_interrupcion_x_consola = false;
								//pthread_mutex_unlock(&mutex_motivo_x_consola);
							}
						}else{ //este caso es las syscalls bloqueantes, ni resize ni wait
							//avisarle a kernel del desalojo?
							liberar_interrupcion_actual();
						}	
					}
				}
			}
		}else{//es exit
			log_info(logger_cpu,"Instruccion ejecutada: EXIT. Proceso  ya había sido desalojado Entonces ignoramos la interrupcion");
			liberar_interrupcion_actual();
		}
	}else{//no hubo una interrupcion
		if(!es_exit){
			if(!es_bloqueante){ // se sabe que no se desalojo al proceso previamente
				if(error_memoria){
					//no hacer nada porque se supone que mov in o mov out ya se pusieron a escuchar otro pcb
				}else{// se sabe que no se desalojo al proceso previamente
					sem_post(&sem_execute);
				}
			}else{ //pudo haberse desalojado al proceso
				if(es_wait){
					if(cambio_proceso_wait){
						sem_post(&sem_execute);
					}else{
						sem_post(&sem_execute);	
					}
				}else{//no es wait, puede ser una syscall bloqueante o resize
					if(es_resize){ 
						if(resize_desalojo_outofmemory){
						//no hacer nada porque la resize ya pone a escuchar otro pcb despues de desalojar ya se había encargado de poner a escuchar otro pcb
						}else{
						sem_post(&sem_execute);
						}
					}else{ //este caso es las syscalls bloqueantes, ni resize ni wait
					//no hacer nada porque la syscall bloqueante ya se había encargado de poner a escuchar otro pcb
					}	
				}
			}
		}else{
			//no hacer nada porque exit ya directamente pone a escuchar otro pcb
		}
		
	}
	*/