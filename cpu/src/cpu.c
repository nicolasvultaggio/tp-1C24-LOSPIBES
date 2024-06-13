#include <../../cpu/include/cpu.h>

int main(int argc, char* argv[]){
    
	decir_hola("CPU");
    
    logger_cpu = log_create("cpu_logs.log","cpu",1,LOG_LEVEL_INFO);
    config_cpu = config_create("./configs/cpu.config");

	log_info(logger_cpu, "INICIA CPUTA");

    leer__configuraciones();
	inicializar_semaforos();

	/* CONECTO CPU CON MEMORIA */
	fd_conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu, "CPU");
	enviar_mensaje("Hola, soy CPUta!", fd_conexion_memoria);
	snd_handshake(fd_conexion_memoria);
	log_info(logger_cpu, "Handshake a MEMORIA realizado");
	
	inicializar_tlb();

	/* COMENZAMOS CON LA LOCURA DE HILOS */
	pthread_t *hilo_dispatch = malloc(sizeof(pthread_t));
	pthread_t *hilo_interrupt = malloc(sizeof(pthread_t));
	pthread_t *hilo_cilo_instruccion = malloc(sizeof(pthread_t));

	pthread_create(hilo_dispatch, NULL, (void*)dispatch, NULL);
	pthread_create(hilo_interrupt, NULL, (void*)interrupcion, NULL);
	pthread_create(hilo_cilo_instruccion, NULL, (void*)fetch, NULL);
	
	pthread_join(*hilo_dispatch, NULL);
	pthread_join(*hilo_interrupt, NULL);
	pthread_join(*hilo_cilo_instruccion, NULL);

    terminar_programa();

    return EXIT_SUCCESS;    
}

/* OBTENER PARAMETROS DE .CONFIG */
void leer__configuraciones(){
    ip_memoria = config_get_string_value(config_cpu,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_cpu,"PUERTO_MEMORIA");
    puerto_cpu_dispatch = config_get_string_value(config_cpu,"PUERTO_ESCUCHA_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config_cpu,"PUERTO_ESCUCHA_INTERRUPT");
	cantidad_entradas_tlb = config_get_string_value(config_cpu,"CANTIDAD_ENTRADAS_TLB");
	algoritmo_tlb = config_get_string_value(config_cpu,"ALGORITMO_TLB");
}

/* Inicializo SEMAFOROS */
void inicializar_semaforos(){

	pthread_mutex_init(&mutex_tlb, NULL);
	pthread_mutex_init(&mutex_motivo_x_consola, NULL);

	sem_init(&sem_recibir_pcb, 0, 1);
	sem_init(&sem_execute, 0, 0);
	sem_init(&sem_interrupcion, 0, 0);

	
}

void dispatch(void *arg){

	fd_cpu_dispatch = iniciar_servidor(NULL, puerto_cpu_dispatch,logger_cpu,"CPU");
	log_info(logger_cpu, "Levantado el puerto DISPATCH");	
	fd_escucha_dispatch  = esperar_cliente(fd_cpu_dispatch, logger_cpu, "Kernell (dispatch)");

	while (1) {
		sem_wait(&sem_recibir_pcb);
		hay_interrupcion = false;// ponerle mutex a hay_interrupción, ya que dos procesos que se ejecutan paralelamente lo modifican o leen --> puede haber condicion de carrera
		int codigo_operacion = recibir_operacion(fd_escucha_dispatch, logger_cpu, "Kernell (dispatch)");
		switch (codigo_operacion) {
		case PCBBITO:
			PCB = recibir_pcb(fd_escucha_dispatch);
			sem_post(&sem_execute);
			break;
		case -1:
			log_error(logger_cpu, "El cliente se desconecto.");
			fd_escucha_dispatch = esperar_cliente(fd_cpu_dispatch, logger_cpu, "Kernel (dispatch)");
			break;
		default:
			log_warning(logger_cpu,"Operacion desconocida.");
			break;
		}
	}
}

/* FETCH busco las proximas intrucciones a ejecutar */
void fetch (void *arg){

	while(1){

		sem_wait(&sem_execute);
		t_linea_instruccion* proxima_instruccion = prox_instruccion(PCB->PID, PCB->PC);
		log_info(logger_cpu, "PID: %d - FETCH - Program Counter: %d", PCB->PID, PCB->PC);
		PCB->PC++;
		decode(proxima_instruccion, PCB);

	}
}

t_linea_instruccion* prox_instruccion(int pid, uint32_t program_counter){
	
	t_linea_instruccion* instruccion_recibida = malloc(sizeof(t_linea_instruccion*));
	
	//enviar_solicitud_de_instruccion(fd_conexion_memoria, pid, program_counter);
	
	int codigo_operacion = recibir_operacion(fd_conexion_memoria, logger_cpu, "Memoria");
		
		switch (codigo_operacion) {
			case PROXIMA_INSTRUCCION:
				instruccion_recibida = recibir_proxima_instruccion(fd_conexion_memoria);
				break;		
		}
	
	return instruccion_recibida;
}

/* DECODE interpreto las intrucciones y las mando a ejecutar */
void decode (t_linea_instruccion* instruccion, pcb* PCB){
	switch(instruccion->instruccion){
		case SET:
			ejecutar_set(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case MOV_IN:
			ejecutar_mov_in(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case MOV_OUT:
			ejecutar_mov_out(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case SUM:
			ejecutar_sum(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case SUB:
			ejecutar_sub(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case JNZ:
			ejecutar_jnz(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case RESIZE:
			ejecutar_resize(PCB,instruccion->parametro1);
			break;
		case COPY_STRING:
			//ejecutar_copy_string();
			break;
		case WAIT:
			ejecutar_wait(PCB, instruccion->parametro1);
			check_interrupt();
			break;
		case SIGNAL:
			ejecutar_signal(PCB, instruccion->parametro1);
			check_interrupt();
			break;
		case IO_GEN_SLEEP:
			ejecutar_io_gen_sleep(PCB, "IO_GEN_SLEEP", instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case IO_STDIN_READ:
			//ejecutar_io_stdin_read();
			break;
		case IO_STDOUT_WRITE:
			//ejecutar_io_stdout_write();
			break;
		case IO_FS_CREATE:
			//ejecutar_io_fs_create();
			break;
		case IO_FS_DELETE:
			//ejecutar_io_fs_delete();
			break;
		case IO_FS_TRUNCATE:
			//ejecutar_io_fs_truncate();
			break;
		case IO_FS_WRITE:
			//ejecutar_io_fs_write();
			break;
		case IO_FS_READ:
			//ejecutar_io_fs_read();
			break;
		case EXIT:
			ejecutar_exit(PCB);
			check_interrupt();
			break;
		default:
			ejecutar_error(PCB);
			break;
	}
}

/* EXECUTE se ejecua lo correspodiente a cada instruccion */

void ejecutar_set(pcb* PCB, char* registro, char* valor){
	
	uint8_t valor8 = strtoul(valor, NULL, 10);
	uint32_t valor32 = strtoul(valor, NULL, 10);
	
	setear_registro(PCB, registro, valor8, valor32);

	es_exit =false; //poner en true 
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_mov_in(pcb* PCB, char* DATOS, char* DIRECCION){

	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "MOV IN", DATOS, DIRECCION);
	
	int direccion_logica = atoi(DIRECCION);
	
	
	size_t direccion_fisica = MMU(pcb, direccion_logica);
	enviar_solicitud_lectura_memoria(direccion_fisica, PCB->pid, fd_conexion_memoria);
	uint32_t valor = recibir_valor_leido();
	log_info(logger_cpu, "PID: %d - Accion: LEER - Direccion Fisica: %d - Valor: %d", pcb->pid, direccion_fisica, valor);
	ejecutar_set(PCB, DATOS, valor);

}

void ejecutar_mov_out(pcb* PCB, char* DIRECCION, char* DATOS){
	
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s, %s]", PCB->pid, "MOV OUT", DIRECCION, DATOS);

	int direccion_logica = atoi(DIRECCION);

	int cantidad_paginas_a_traducir = calculo_de_paginas(DATOS, direccion_logica);

	size_t direccion_fisica = MMU(pcb, direccion_logica);

	uint32_t valor32;

	if(strcmp(registro, "EAX") == 0){
		valor32 = PCB->registros.EAX;
	} else if(strcmp(registro, "EBX") == 0){
		valor32 = PCB->registros.EBX;
	} else if(strcmp(registro, "ECX") == 0){
		valor32 = PCB->registros.ECX;
	} else if(strcmp(registro, "EDX") == 0){
		valor32 = PCB->registros.EDX;
	}

	int numero_pagina = floor(direccion_logica/TAM_PAGINA);
	valores_tlb* valores = malloc(sizeof(valores_tlb));
	valores->numero_pagina = numero_pagina;
	valores->pid = PCB->pid;
	enviar_solicitud_escritura_memoria(direccion_fisica, valor32, valores, fd_conexion_memoria);
	log_info(logger_cpu, "PID: %d - Accion: ESCRIBIR - Direccion Fisica: %d - Valor: %d", pcb->pid, direccion_fisica, valor);
	check_interrupt();
	free(valores);
}

void ejecutar_sum(pcb* PCB, char* destinoregistro, char* origenregistro){
	
	size_t size_origen = 0, size_destino = 0;

	uint8_t destino8, origen8;
		
	uint32_t destino32, origen32;

	size_origen = size_registro(origenregistro);
	size_destino = size_registro(destinoregistro);

	if((size_origen == 1) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 + origen8;
	}else if((size_origen == 1) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 + (uint32_t) origen8;
	}else if((size_origen == 4) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 + (uint8_t) origen32;
	}else if((size_origen == 4) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 + origen32;
	}

	es_exit = false;  
	es_bloqueante = false; 
	return;
}

void ejecutar_sub(pcb* PCB, char* destinoregistro, char* origenregistro){

	size_t size_origen = 0, size_destino = 0;

	uint8_t destino8, origen8;
		
	uint32_t destino32, origen32;

	size_origen = size_registro(origenregistro);
	size_destino = size_registro(destinoregistro);

	if((size_origen == 1) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 - origen8;
	}else if((size_origen == 1) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 - (uint32_t) origen8;
	}else if((size_origen == 4) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 - (uint8_t) origen32;
	}else if((size_origen == 4) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 - origen32;
	}

	es_exit =false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_jnz(pcb* PCB, char* registro, char* valor){
	
	uint8_t * registro8, * valor8;

	uint32_t * registro32, * valor32;
	
	







	uint32_t PC_actualizado = valor;
	

	size_t * reg_value = capturar_registro (registro);

	if (reg_value != 0)
	{
		
	}else if (reg != 00)
	{
		/* code */
	}
	
	

	if(medir_registro(registro)){
		if(capturar_registro8(PCB, registro) != 0)
			PCB->PC = program_counter_actualizado;
	}else{
		if(capturar_registro32(PCB, registro) != 0)
			PCB->PC = program_counter_actualizado;
	}
	es_exit=false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_resize(pcb* PCB, char* tamanio){











}


void ejecutar_wait(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "WAIT", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch, RECURSO, SOLICITAR_WAIT,NULL,NULL,NULL,NULL,NULL);
	free(recurso);
	sem_post(&sem_recibir_pcb);
	es_exit=false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
}

void ejecutar_signal(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "SIGNAL", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch, RECURSO, SOLICITAR_SIGNAL,NULL,NULL,NULL,NULL,NULL);
	free(recurso);
	//aca te falta esperar la respuesta del kernel
	es_exit = false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
}

void ejecutar_io_gen_sleep(pcb* PCB, char* instruccion, char* interfaz, char* unidad_de_tiempo){
	enviar_pcb(PCB, fd_escucha_dispatch, INTERFAZ, SOLICITAR_INTERFAZ_GENERICA, instruccion, interfaz, unidad_de_tiempo,NULL,NULL);
	sem_post(&sem_recibir_pcb);
	es_exit=false;  //siempre modificar
	es_bloqueante=true; //modificar siempre que es_exit = false
	es_wait = false;  //modificar si se pone a bloqueante = true
	es_resize = false; //modificar si se pone bloqueante = true
	return;
}

void ejecutar_exit(pcb* PCB){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch, PCB_ACTUALIZADO, EXITO,NULL,NULL,NULL,NULL,NULL);
	sem_post(&sem_recibir_pcb);
	es_exit=true;  //siempre modificar
}

void ejecutar_error(pcb* PCB){ //se usa esta en algun momento?
	log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch, PCB_ACTUALIZADO, EXIT_CONSOLA,NULL,NULL,NULL,NULL,NULL);
	sem_post(&sem_recibir_pcb);
}


/* FUNCIONES PARA EL CALCULO ARITMETICOLOGICO */

size_t size_registro(char * registro){

	size_t SIZEEE;

	if(strcmp(registro, "AX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else if(strcmp(registro, "BX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else if(strcmp(registro, "CX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else if(strcmp(registro, "DX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else  if(strcmp(registro, "EAX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "EBX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "ECX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "EDX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "SI") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "DI") == 0){
		SIZEEE = sizeof(uint32_t);
	}

	return SIZEEE;
}

void setear_registro(pcb * PCB, char * registro, uint8_t valor8, uint32_t valor32){
	if(strcmp(registro, "AX") == 0){
		PCB->registros.AX = valor8;
	} else if(strcmp(registro, "BX") == 0){
		PCB->registros.BX = valor8;
	} else if(strcmp(registro, "CX") == 0){
		PCB->registros.CX = valor8;
	} else if(strcmp(registro, "DX") == 0){
		PCB->registros.DX = valor8;
	}else if(strcmp(registro, "EAX") == 0){
		PCB->registros.EAX = valor32;
	} else if(strcmp(registro, "EBX") == 0){
		PCB->registros.EBX = valor32;
	} else if(strcmp(registro, "ECX") == 0){
		PCB->registros.ECX = valor32;
	} else if(strcmp(registro, "EDX") == 0){
		PCB->registros.EDX = valor32;
	} else if(strcmp(registro, "SI") == 0){
		PCB->registros.SI = valor32;
	} else if(strcmp(registro, "DI") == 0){
		PCB->registros.DI = valor32;
	}
	return;
}

void * capturar_registro(pcb * PCB, char * registro){
	
	if(strcmp(registro, "AX") == 0){
		return * PCB->registros.AX;
	} else if(strcmp(registro, "BX") == 0){
		return * PCB->registros.BX;
	} else if(strcmp(registro, "CX") == 0){
		return * PCB->registros.CX;
	} else if(strcmp(registro, "DX") == 0){
		return * PCB->registros.DX;
	}else if(strcmp(registro, "EAX") == 0){
		return * PCB->registros.EAX;
	} else if(strcmp(registro, "EBX") == 0){
		return * PCB->registros.EBX;
	} else if(strcmp(registro, "ECX") == 0){
		return * PCB->registros.ECX;
	} else if(strcmp(registro, "EDX") == 0){
		return * PCB->registros.EDX;
	} else if(strcmp(registro, "SI") == 0){
		return * PCB->registros.SI;
	} else if(strcmp(registro, "DI") == 0){
		return * PCB->registros.DI;
	}

}

/* FUNCIONES MOVEIN MOVEOUT */



int calculo_de_paginas(char* DATOS, int direccion_logica){

	int numero_pagina_inicial = floor(direccion_logica/TAM_PAGINA);
	int numero_pagina_final = numero_pagina_inicial + size_registro(DATOS);

	return numero_pagina_final - numero_pagina_inicial;
}

uint32_t recibir_valor_leido(){
	uint32_t valor;
	int cod_op = recibir_operacion(fd_conexion_memoria);
		switch (cod_op) {
			case VALOR_LEIDO:
				valor = recibir_valor_leido_memoria(fd_conexion_memoria);
				break;
			}
}

size_t MMU(pcb* PCB, char* DIRECCION){
	int marco;
	int direccion_logica = atoi(DIRECCION);
	int numero_pagina = floor(direccion_logica / TAM_PAGINA);
	int desplazamiento =  direccion_logica - numero_pagina * TAM_PAGINA;

	switch (MAX_TLB_ENTRY)
	{
	case 0:
		enviar_solicitud_marco(fd_conexion_memoria, PCB->PID, numero_pagina);
		marco = recibir_marco();
		break;
	default:
		marco = consultar_tlb(PCB->PID, numero_pagina);
		break;
	}

	log_info(logger_cpu,  "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", PCB->pid, numero_pagina, marco); 
	size_t direccion_fisica = marco*tam_pagina + desplazamiento;      
	return direccion_fisica;
}

/* CHECK INTERRUPT */
void* interrupcion(void *arg) {

	fd_cpu_interrupt = iniciar_servidor(NULL, puerto_cpu_interrupt, logger_cpu, "CPU");
	log_info(logger_cpu, "Leavantado el puerto INTERRUPT");
	fd_escucha_interrupt = esperar_cliente(fd_cpu_interrupt,logger_cpu, "Kernel (interrupt)");
	
	while (1) {
		int codigo_operacion = recibir_operacion(fd_escucha_interrupt, logger_cpu, "Kernel (interrupt)");
		switch (codigo_operacion) {
		case INTERR:
			motivo_desalojo motivo = recibir_motiv_desalojo(fd_escucha_interrupt);
			interrupcion_actual = malloc(sizeof(motivo_desalojo));
			pthread_mutex_lock(&mutex_motivo_x_consola);
			if(!hay_interrupcion_x_consola){
				if(motivo == EXIT_CONSOLA){
					hay_interrupcion_x_consola = true;
					(*interrupcion_actual) = motivo; // motivo = EXIT_CONSOLA
				}else{
					(*interrupcion_actual) = motivo; // motivo = EXIT_CONSOLA
				}
			}
			pthread_mutex_unlock(&mutex_motivo_x_consola);
			break;
		case -1:
			log_error(logger_cpu, "El cliente se desconecto.");
			fd_escucha_interrupt = esperar_cliente(fd_cpu_interrupt,logger_cpu, "Kernel (interrupt)");
			break;
		default:
			log_warning(logger_cpu,"Operacion desconocida.");
			break;
		}
	}
}

// yo se que es un quilombo de if y else, pero posta que si no mostras que hacer en todos los casos es un quilombo entenderlo
void check_interrupt (){
	if(interrupcion_actual!=NULL){
		if (!es_exit){
			if(*interrupcion_actual == FIN_QUANTUM){ //si la interrupcion es por desalojo de quantum
				if(!es_bloqueante){ // se sabe que no se desalojo al proceso previamente
					if(error_memoria){ //
						log_info(logger_cpu,"Como hubo un error de escritura del proceso %d, ignoramos interrupcion de fin de quantum",PCB->PID);
						liberar_interrupcion_actual(); // no atendemos interrupcion ni ponemos ningun semaforo porque mov in y mov out ya desalojan y ponen a escuchar otro pcb
					}else{ // se sabe que no se desalojo al proceso previamente
						log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
						enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
						sem_post(&sem_recibir_pcb);
						liberar_interrupcion_actual();
					}
					
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
							pthread_mutex_lock(&mutex_motivo_x_consola);
							hay_interrupcion_x_consola = false;
							pthread_mutex_unlock(&mutex_motivo_x_consola);
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
								pthread_mutex_lock(&mutex_motivo_x_consola);
								hay_interrupcion_x_consola = false;
								pthread_mutex_unlock(&mutex_motivo_x_consola);
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
}

void liberar_interrupcion_actual(){
	free(interrupcion_actual);
	interrupcion_actual = NULL;
	return;
}


/* TLB */

/* TRADUCCION LOGICA A FISICA */

int MMU(pcb* PCB, char* DIRECCION){
	int direccion_fisica;
	int marco;
	int direccion_logica = atoi(DIRECCION);
	int numero_pagina = floor(direccion_logica / TAM_PAGINA);
	int desplazamiento =  direccion_logica - numero_pagina * TAM_PAGINA;

	switch (MAX_TLB_ENTRY)
	{
	case 0:
		enviar_solicitud_marco(fd_conexion_memoria, PCB->PID, numero_pagina);
		marco = recibir_marco(fd_conexion_memoria);
		break;
	default:
		marco = consultar_tlb(PCB->PID, numero_pagina);
		break;
	}

	if(marco == -1){
		log_info(logger_cpu, "Page Fault PID: %d - Pagina: %d", PCB->pid, numero_pagina); //log ob
		//iniciar acciones page fault
		//send_pcb(pcb, dispatch_cliente_fd);
		//send_pcb_pf(numero_pagina, desplazamiento, dispatch_cliente_fd);
		//sem_post(&sem_nuevo_proceso);
		return marco;
	}
	log_info(logger_cpu,  "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", PCB->pid, numero_pagina, marco); //log ob
	int direccion_fisica = marco*TAM_PAGINA + desplazamiento;
	return direccion_fisica;
}

void inicializar_tlb(){

	translation_lookaside_buffer = list_create();
	MAX_TLB_ENTRY = atoi (cantidad_entradas_tlb);
	TAM_PAGINA = solicitar_tamanio_pagina();

}

int solicitar_tamanio_pagina(){
	int tamanio;
	int codigo_operacion = recibir_operacion(fd_conexion_memoria);
	switch (codigo_operacion){
		case SIZE_PAGE:
			TAM_PAGINA = recibir_tamanio_pagina(fd_conexion_memoria);
			break;
	}		
	
	return tamanio;
}

int solicitar_info_memory(int PID){

	enviar_solicitud_marco(fd_conexion_memoria, PCB->PID, numero_pagina);
	return recibir_marco();

}

void agregar_entrada_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria, pthread_mutex_t* mutex, int PID, int numero_pagina, int marco){

	info_proceso_memoria->valor->pid = PID;
	info_proceso_memoria->valor->numero_pagina = numero_pagina;
	info_proceso_memoria->marco = marco;

	push_con_mutex(TLB, info_proceso_memoria, &mutex_tlb);

}

void eliminar_entrada_tlb(t_list* TLB, pthread_mutex_t* mutex, int posicion){

	remove_con_mutex(TLB, mutex, posicion);
	
} 

void verificar_tamanio_tlb(t_list* TLB, pthread_mutex_t* mutex){

	if((list_size(TLB)/MAX_TLB_ENTRY) =! 1){
		return;
	}
	pop_con_mutex(TLB, mutex);
}

int consultar_tlb(int PID, int numero_pagina){
	
	int marco;
	int posicion_elemento_buscado;
	nodo_tlb * info_proceso_memoria;
	
	if(list_is_empty(translation_lookaside_buffer)){
		marco = solicitar_info_memory(PID, numero_pagina);
		agregar_entrada_tlb(translation_lookaside_buffer, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
		return marco;
	}
	
	info_proceso_memoria = list_find(translation_lookaside_buffer,(void*)PCB->PID);

	if(info_proceso_memoria == NULL){
		marco = solicitar_info_memory(PID, numero_pagina);
		info_proceso_memoria = administrar_tlb(translation_lookaside_buffer, info_proceso_memoria, PID, numero_pagina, marco);
	}

	t_list * lista_auxiliar = list_create();
	posicion_elemento_buscado = posicion_elemento_tlb(translation_lookaside_buffer, info_proceso_memoria);
	lista_auxiliar = list_duplicate(translation_lookaside_buffer);
	eliminar_entrada_tlb(translation_lookaside_buffer, &mutex_tlb, posicion_elemento_buscado);
	info_proceso_memoria = list_get(lista_auxiliar,posicion_elemento_buscado);
	push_con_mutex(translation_lookaside_buffer, info_proceso_memoria, &mutex_tlb);
	info_proceso_memoria = list_get(translation_lookaside_buffer, 0);
	list_clean(lista_auxiliar);

	return info_proceso_memoria->marco;
}

int posicion_elemento_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria){
	for(int i = 0; i<list_size(TLB); i++){
		nodo_tlb * NODO = list_get(TLB, i);
		if(NODO->valor->pid == info_proceso_memoria->valor->pid){
			if(NODO->valor->numero_pagina == info_proceso_memoria->valor->numero_pagina){
				return i;
			}
		}
	}
}

nodo_tlb * administrar_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria, int PID, int numero_pagina, int marco){

	nodo_tlb * elemento_buscado_segun_marco = list_find(TLB, (void*)marco);
	verificar_tamanio_tlb(TLB, &mutex_tlb);

	if(elemento_buscado_segun_marco == NULL){
		
		agregar_entrada_tlb(TLB, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
		elemento_buscado_segun_marco = list_find(TLB, (void*)marco);
		return elemento_buscado_segun_marco->marco;

	}

	int posicion_elemento_buscado = posicion_elemento_tlb(TLB, elemento_buscado_segun_marco);
	agregar_entrada_tlb(TLB, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
	eliminar_entrada_tlb(TLB, &mutex_tlb, posicion_elemento_buscado);
	
	return info_proceso_memoria = list_find(TLB, (void*)marco);;
}


/* LIBERAR MEMORIA USADA POR CPU */
void terminar_programa(){
    if(logger_cpu != NULL){
		log_destroy(logger_cpu);
	}
	if(config_cpu != NULL){
		config_destroy(config_cpu);
	}
    liberar_conexion(fd_conexion_memoria);
}


