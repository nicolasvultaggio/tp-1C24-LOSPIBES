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

t_linea_instruccion* prox_instruccion(int pid, int program_counter){
	
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
			//ejecutar_resize();
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
	if(medir_registro(registro)){
		uint8_t valor8 = strtoul(valor, NULL, 10);
		setear_registro8(PCB, registro, valor8);
	}else{
		uint32_t valor32 = strtoul(valor, NULL, 10);
		setear_registro32(PCB, registro, valor32);
	}
	es_exit =false; //poner en true 
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_mov_in(pcb* PCB, char* DATOS, char* DIRECCION){

log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "MOV IN", param1, param2);
	
	int direccion_logica = atoi(DIRECCION);
	int direccion_fisica = solicitar_direccion_fisica(pcb, direccion_logica);
	if(direccion_fisica == -1){ //al final esto no existe, la mmu siempre devuelve una direccion traducida (nunca te va a devolver -1), despues vos le pedis a la memoria hacer lo q tengas que hacer, y esta te dice si pudo hacerlo, si no pudo hacerlo vos tenes que desalojar el proceso
		//pcb->program_counter -= 1;
		return;
	}
	//eliminar este if

	//¿como es la secuencia despues de obtener la direccion fisica?

	// pedis la escritura o lectura a memoria (esta parte hablala con Marce)
	// te quedas esperando su respuesta   (esta parte hablala con Marce)
	// si salio bien (pudo escribir donde le pediste), perfecto no haces nada mas en esta funcion, pasas a check interrupt
	// si no salio bien (le pediste una direccion invalida -> le desalojas el pcb con motivo de desalojo fallo o error, para que kernel pueda finalizar el proceso) y haces check interrupt
	//esta misma logica la tendran que implementar las interfaces stdin y stdout, porque tambien le piden escribir o leer a memoria
	//atenti a los flags que hay que modificar en cada instruccion, fijate en el .h, sobre todo el que se llama error_memoria

	es_exit=false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
	//error_memoria; se pone true o false segun respuesta de memoria 
	
}

void ejecutar_mov_out(pcb* PCB, char* DIRECCION, char* DATOS){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s, %s]", PCB->pid, "MOV OUT", param1, param2);
		
	int direccion_fisica = MMU(PCB, DIRECCION); // ojo estas 
	
	if(direccion_fisica == -1){ //al final esto no existe, la mmu siempre devuelve una direccion traducida (nunca te va a devolver -1), despues vos le pedis a la memoria hacer lo q tengas que hacer, y esta te dice si pudo hacerlo, si no pudo hacerlo vos tenes que desalojar el proceso
		//pcb->program_counter -= 1;
		return;
	}
	//eliminar este if

	//¿como es la secuencia despues de obtener la direccion fisica?

	// pedis la escritura o lectura a memoria (esta parte hablala con Marce)
	// te quedas esperando su respuesta   (esta parte hablala con Marce)
	// si salio bien (pudo escribir donde le pediste), perfecto no haces nada mas en esta funcion, pasas a check interrupt
	// si no salio bien (le pediste una direccion invalida -> le desalojas el pcb con motivo de desalojo fallo o error, para que kernel pueda finalizar el proceso) y haces check interrupt
	//esta misma logica la tendran que implementar las interfaces stdin y stdout, porque tambien le piden escribir o leer a memoria
	//atenti a los flags que hay que modificar en cada instruccion, fijate en el .h, sobre todo el que se llama error_memoria

	es_exit=false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
	//error_memoria; se pone true o false segun respuesta de memoria 

}


void ejecutar_sum(pcb* PCB, char* destinoregistro, char* origenregistro){
	uint8_t destino8;
	uint8_t origen8;
	uint32_t destino32;
	uint32_t origen32;
	if(medir_registro(destinoregistro)){
		destino8 = capturar_registro8(PCB, destinoregistro);
	}else{
		destino32 = capturar_registro32(PCB, destinoregistro);
	}

	if(medir_registro(origenregistro)){
		origen8 = capturar_registro8(PCB, origenregistro);
	}else{
		origen32 = capturar_registro32(PCB, origenregistro);
	}

	if(medir_registro(destinoregistro)){
		setear_registro8(PCB, destinoregistro, destino8 + (uint8_t) origen8); 
	}else{//																	//ojo, aca no hay mas referencia a origen ni destino, solo 
		setear_registro32(PCB, destinoregistro, destino32 + (uint32_t) origen32);
	}
	es_exit = false;  
	es_bloqueante = false; 
	return;
}

void ejecutar_sub(pcb* PCB, char* destinoregistro, char* origenregistro){

	uint8_t origen;
	uint32_t origen32;

	if(medir_registro(origenregistro)){
		origen = capturar_registro8(PCB, origenregistro);
	}else{
		origen = capturar_registro32(PCB, origenregistro);
	}

	if(medir_registro(destinoregistro)){
		uint8_t destino = capturar_registro8(PCB, destinoregistro);
		uint8_t origen8 = (uint8_t) origen;
		setear_registro8(PCB, destinoregistro, destino + origen8);
	}else{
		uint32_t destino = capturar_registro32(PCB, destinoregistro);
		uint32_t origen8 = (uint32_t) origen32;
		setear_registro32(PCB, destinoregistro, destino + origen32);
	}
		
	es_exit =false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_jnz(pcb* PCB, char* registro, char* valor){
	uint32_t program_counter_actualizado = (uint32_t)strtoul(valor,NULL,10);
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

bool medir_registro(char* registro){
	if(strcmp(registro, "AX") == 0){
		return true;
	} else if(strcmp(registro, "BX") == 0){
		return true;
	} else if(strcmp(registro, "CX") == 0){
		return true;
	} else if(strcmp(registro, "DX") == 0){
		return true;
	}else{
		return false;
	}
}

void setear_registro8(pcb* PCB, char* registro, uint8_t valor){
	if(strcmp(registro, "AX") == 0){
		PCB->registros.AX = valor;
	} else if(strcmp(registro, "BX") == 0){
		PCB->registros.BX = valor;
	} else if(strcmp(registro, "CX") == 0){
		PCB->registros.CX = valor;
	} else if(strcmp(registro, "DX") == 0){
		PCB->registros.DX = valor;
	}
	return;
}

void setear_registro32(pcb* PCB, char* registro, uint32_t valor){
   if(strcmp(registro, "EAX") == 0){
		PCB->registros.EAX = valor;
	} else if(strcmp(registro, "EBX") == 0){
		PCB->registros.EBX = valor;
	} else if(strcmp(registro, "ECX") == 0){
		PCB->registros.ECX = valor;
	} else if(strcmp(registro, "EDX") == 0){
		PCB->registros.EDX = valor;
	} else if(strcmp(registro, "SI") == 0){
		PCB->registros.SI = valor;
	} else if(strcmp(registro, "DI") == 0){
		PCB->registros.DI = valor;
	}
	return;
}

uint8_t capturar_registro8(pcb* PCB, char* registro){
	if(strcmp(registro, "AX") == 0){
		return PCB->registros.AX;
	} else if(strcmp(registro, "BX") == 0){
		return PCB->registros.BX;
	} else if(strcmp(registro, "CX") == 0){
		return PCB->registros.CX;
	} else if(strcmp(registro, "DX") == 0){
		return PCB->registros.DX;
	}
}

uint32_t capturar_registro32(pcb* PCB, char* registro){
   if(strcmp(registro, "EAX") == 0){
		return PCB->registros.EAX;
	} else if(strcmp(registro, "EBX") == 0){
		return PCB->registros.EBX;
	} else if(strcmp(registro, "ECX") == 0){
		return PCB->registros.ECX;
	} else if(strcmp(registro, "EDX") == 0){
		return PCB->registros.EDX;
	} else if(strcmp(registro, "SI") == 0){
		return PCB->registros.SI;
	} else if(strcmp(registro, "DI") == 0){
		return PCB->registros.DI;
	}
}

/*****************************************************/

/* TRADUCCION LOGICA A FISICA */

int MMU(pcb* PCB, char* DIRECCION){ //que la direccion fisica sea un size_t, fijate que las interfaces y todo lo que es direcciones de memoria ya implementado es con size_t
	int direccion_fisica;
	int marco;
	int direccion_logica = atoi(DIRECCION);
	int numero_pagina = floor(direccion_logica / TAM_PAGINA); // tam_pagina no existe
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


	
	send_solicitud_marco(fd_memoria, pcb->pid, numero_pagina);
	int marco = recibir_marco();
	if(marco == -1){
		//log_info(logger, "Page Fault PID: %d - Pagina: %d", pcb->pid, numero_pagina); //log ob
		//iniciar acciones page fault
		//send_pcb(pcb, dispatch_cliente_fd); ojo sergio, creo que cambio la funcion para enviar un pcb, preguntale a tomi
		//send_pcb_pf(numero_pagina, desplazamiento, dispatch_cliente_fd);
		//sem_post(&sem_nuevo_proceso); 
		//return marco;
	}
	log_info(logger_cpu,  "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pcb->pid, numero_pagina, marco); 
	//int direccion_fisica = marco*tam_pagina + desplazamiento;        //aca  ^^^ no podes poner pcb->pid, es PCB->pid, pcb en minusculas es el tipo de dato, no un puntero -> DEBUGGEEN ESTAS COSAS PORQUE ESTOS ERRORES SE ACUMULAN
	return direccion_fisica; //direccion fisica tiene que ser un size_t, no un int, fijate como esta en todo el tp, los size_t se usan para representar numeros de bytes, no los int
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
			(*interrupcion_actual) = motivo;
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

void inicializar_tlb(){

	translation_lookaside_buffer = list_create();
	MAX_TLB_ENTRY = atoi (cantidad_entradas_tlb);

}

int solicitar_info_memory(int PID){

	enviar_solicitud_marco(fd_conexion_memoria, PCB->PID, numero_pagina);
	return recibir_marco();
}

void agregar_entrada_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria, pthread_mutex_t* mutex, int PID, int numero_pagina, int marco){

	info_proceso_memoria->valor->pid = PID;
	info_proceso_memoria->valor->numero_pagina = numero_pagina;
	info_proceso_memoria->valor->marco = marco;

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

	nodo_tlb * info_proceso_memoria;

	if(list_is_empty(translation_lookaside_buffer)){
		marco = solicitar_info_memory(PID, numero_pagina);
		agregar_entrada_tlb(translation_lookaside_buffer, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
		return info_proceso_memoria->valor->marco;
	}
	
	info_proceso_memoria = list_find(translation_lookaside_buffer,(void*)PCB->PID);

	if(info_proceso_memoria = NULL){
		marco = solicitar_info_memory(PID, numero_pagina);
		agregar_entrada_tlb(translation_lookaside_buffer, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
	}

	marco = info_proceso_memoria->valor->marco;

	administrar_tlb(translation_lookaside_buffer, info_proceso_memoria);

	return marco;
}

int posicion_nodo_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria){
	for(int i = 0; i<list_size(TLB); i++){
		nodo_tlb * NODO = list_get(TLB, i);
		if(NODO->valor->pid == info_proceso_memoria->valor->pid){
			return i;
		}
}
}

void administrar_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria){

	if(algoritmo_tlb == "FIFO"){
		return;
	}

	verificar_tamanio_tlb(TLB &mutex_tlb);

	int posicion_nodo_buscado = posicion_nodo_tlb(TLB, info_proceso_memoria);

	agregar_entrada_tlb(TLB, info_proceso_memoria, &mutex_tlb, info_proceso_memoria->valor->pid, info_proceso_memoria->valor->numero_pagina, info_proceso_memoria->valor->marco);
	
	eliminar_entrada_tlb(TLB, &mutex_tlb, posicion_nodo_buscado);

	return;
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
