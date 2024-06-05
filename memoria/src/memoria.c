#include <../../memoria/include/memoria.h>
int main() {
    
    decir_hola("Memoria, verdad y justicia");

    logger_memoria = log_create("memoria_logs.log","memoria",1,LOG_LEVEL_INFO);
    config_memoria = config_create("./configs/memoria.config");
    leer_configuraciones();
    //iniciar servidor de memoria
    //CHECKPOINT 1 *************/
    fd_escucha_memoria = iniciar_servidor(NULL, puerto_propio,logger_memoria,"Memoria");
    
    log_info(logger_memoria, "Puerto de memoria habilitado para sus clientes");

	iniciar_memoria_contigua();
    //***************************CHECKPOINT 2*************************+
    
    // Iniciar conexiones y esperar y creo lista de procesos
	
	while(server_escuchar(fd_escucha_memoria));

	// Termino programa
	terminar_programa(logger_memoria, config_memoria);
    
    
}

void iniciar_memoria_contigua(){
	user_space = malloc(tam_memoria); //casteo porque size_t representa numeros de bytes mejor
	cant_marcos = tam_memoria / tam_pagina;//siempre devuelve un numero entero, porque tam_memoria = tam_pagina * cant_marcos, por hipotesis del tp
	bitmap=malloc(dividir_y_redondear_hacia_arriba(cant_marcos,8)); //el tamaño del bitmap es la cantidad de bits que ocupará expresada en bytes
	frames_array = bitarray_create_with_mode(bitmap,dividir_y_redondear_hacia_arriba( cant_marcos , 8), LSB_FIRST); // simplemente apunto al bitmap como si fuera un array de bits
	//solo utilizar cant_marcos, user_space, frames_array
} //IMPORTANTE, la cantidad de elementos del bitarray a veces no termina siendo igual a la cantidad de marcos, redondeando para arriba nos aseguramos de que esta no sea menor

int nro_de_marco_libre(){
	for(int i=0; i<cant_marcos; i++){
		if(bitarray_test_bit(frames_array, (off_t) i))
			return i;
	}
	return -1; //simbolizamos el -1 como que no hay marcos disponibles ----> acá devolver OUT OF MEMORY
}

size_t dividir_y_redondear_hacia_arriba(size_t a,size_t b){
	size_t resultado = a / b ;
	if (a % b == 0){
		return resultado;
	}
	return ( resultado + 1);
}

void inicializar_semaforos(){
	//pthread_mutex_init(&mutex_lista_new, NULL);
	pthread_mutex_init(&mutex_lista_instrucciones, NULL);
}

//Esta pensado para que en un futuro lea mas de una configuracion. 
void leer_configuraciones(){
    puerto_propio = config_get_string_value(config_memoria,"PUERTO_PROPIO");
    tam_memoria= (size_t) config_get_int_value(config_memoria, "TAM_MEMORIA");
    tam_pagina= (size_t) config_get_int_value(config_memoria,"TAM_PAGINA");
    path_instrucciones=config_get_string_value(config_memoria,"PATH_INSTRUCCIONES");
    retardo_respuesta= config_get_int_value(config_memoria,"RETARDO_RESPUESTA");
}

void terminar_programa(){
    config_destroy(config_memoria);
    log_destroy(logger_memoria);
}
t_list * leer_pseudocodigo(char* ruta){ //tenía que devolver un puntero a lista
    
    t_list* instrucciones = list_create();
    FILE* f;
    char buffer[256]; 
    char* palabra;
    char* instruccion_leida= NULL;
    char* parametros[5];
    int contadordeparametros;

    f=fopen(ruta,"r");
	while (fgets(buffer, 256, f) != NULL) {
		t_linea_instruccion* instruccion = malloc(sizeof(cod_instruccion)); 
		instruccion->parametro1 = malloc(256);
		instruccion->parametro2 = malloc(256);
		instruccion->parametro3 = malloc(256);
		instruccion->parametro4 = malloc(256);
        instruccion->parametro5 = malloc(256);


		// Eliminar el carácter de salto de línea del final de la línea
		buffer[strcspn(buffer, "\n")] = '\0';

        instruccion_leida = strtok(buffer, " ");                                                             
        contadordeparametros = 0;
        while ((palabra = strtok(NULL, " ")) != NULL && contadordeparametros < 5) {
            parametros[contadordeparametros++] = palabra;
        }

        if(instruccion_leida != NULL){
            cod_instruccion cod_inst = instruccion_to_enum(instruccion_leida);

            instruccion->instruccion = cod_inst;
            if(contadordeparametros > 0){
            	strcpy(instruccion->parametro1, parametros[0]);
            }else{
            	strcpy(instruccion->parametro1, "");
            }
            if(contadordeparametros > 1){
            	strcpy(instruccion->parametro2, parametros[1]);
            }else{
            	strcpy(instruccion->parametro2, "");
            }
            if(contadordeparametros > 2){
            	strcpy(instruccion->parametro3, parametros[2]);
            }else{
            	strcpy(instruccion->parametro3, "");
            }
			if(contadordeparametros>3){
                strcpy(instruccion->parametro1,parametros[3]);
            }
            else{
                strcpy(instruccion->parametro1," ");
            }
          if(contadordeparametros>4){
                strcpy(instruccion->parametro1,parametros[4]);
            }
         else{
              strcpy(instruccion->parametro1," ");
            }
            
		list_add(instrucciones, instruccion);
        }else{
        	instruccion_destroyer(instruccion);
        }
	}

	fclose(f);

	return instrucciones;
}
void instruccion_destroyer(t_linea_instruccion* instruccion){
	free(instruccion);
	free(instruccion->parametro1);
	free(instruccion->parametro2);
	free(instruccion->parametro3);
	free(instruccion->parametro4);
	free(instruccion->parametro5);
}

//aca se le conectan primero cpu, despues kernel, y despues pueden conectarse MUULTIPLES interfaces
int server_escuchar() { 
	 
	int cliente_socket = esperar_cliente(fd_escucha_memoria, logger_memoria, "SOY UN CLIENTE");

	if (cliente_socket != -1) {
		pthread_t hilo;
		int *args = malloc(sizeof(int)); 
		args = &cliente_socket;
		pthread_create(&hilo, NULL, (void*) procesar_clientes, (void*) args);
		pthread_detach(hilo);
		return 1;
	}

	return 0;
}

//por que necesita static? no la invoca ningun otro archivo
static void procesar_clientes(void* void_args){ 
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger_memoria, "El cliente se desconecto de memoria server"); 
			free(args);
			return;
		}
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger_memoria, cliente_socket);
			break;
		case PAQUETE:
			t_list *paquete_recibido = recibir_paquete(cliente_socket);
			log_info(logger_memoria, "Recibí un paquete con los siguientes valores: ");
			list_iterate(paquete_recibido, (void*) iterator);
			break;	
		case DATOS_PROCESO: // este codigo SOLO LO ENVIA EL KERNEL
				t_datos_proceso* datos_proceso = recibir_datos_del_proceso(cliente_socket);// por que esta en protocolo.h? si es una funcion que conoce solo la memoria, puede estar en memoria.h
				iniciar_memoria_apedidodeKernel(datos_proceso->path, datos_proceso->pid, cliente_socket);//el parametro size sera usado en el 3er check,"datos_proceso->size"
				free(datos_proceso->path);
				free(datos_proceso);
			break;
		case SOLICITAR_INSTRUCCION:// ESTE CODIGO SOLO LO ENVÍA CPU
				log_info(logger_memoria, "Solicitud de instruccion recibida");
				procesar_pedido_instruuccion(cliente_socket, proceso_instrucciones);
				break;
		
		default:
				log_error(logger_memoria, "Codigo de operacion no reconocido en memoria");
				return;
			}

		}
	log_warning(logger_memoria, "El cliente se desconecto de %s server", "memoria");
	return;
}
cod_instruccion instruccion_to_enum(char* instruccion){
	if(strcmp(instruccion, "SET") == 0){
		return SET;
	} else if(strcmp(instruccion, "SUM") == 0){
		return SUM;
	} else if(strcmp(instruccion, "SUB") == 0){
		return SUB;
	} else if(strcmp(instruccion, "JNZ") == 0){
		return JNZ;
	} else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	} else if(strcmp(instruccion, "WAIT") == 0){
		return WAIT;
	} else if(strcmp(instruccion, "IO_STDIN_READ") == 0){
		return IO_STDIN_READ;
	} else if(strcmp(instruccion, "SIGNAL") == 0){
		return SIGNAL;

	} else if(strcmp(instruccion, "MOV_OUT") == 0){
		return MOV_OUT;
	} else if(strcmp(instruccion, "MOV_IN") == 0){
		return MOV_IN;
	} else if (strcmp(instruccion, "RESIZE") == 0){
		return RESIZE;
	} else if (strcmp(instruccion, "COPY_STRING") == 0){
		return COPY_STRING;
	}
     else if (strcmp(instruccion, "IO_STDOUT_WRITE") == 0){
		return IO_STDOUT_WRITE;
	}
     else if (strcmp(instruccion, "IO_FS_CREATE") == 0){
		return IO_FS_CREATE;
	}
     else if (strcmp(instruccion, "IO_FS_DELETE") == 0){
		return IO_FS_DELETE;
	}
     else if (strcmp(instruccion, "IO_FS_TRUNCATE") == 0){
		return IO_FS_TRUNCATE;
	}
     else if (strcmp(instruccion, "IO_FS_WRITE") == 0){
		return IO_FS_WRITE;
	}
     else if (strcmp(instruccion, "IO_FS_READ") == 0){
		return IO_FS_READ;
	}
	return EXIT_FAILURE;
}
void iniciar_memoria_apedidodeKernel(char* path, int pid, int socket_kernel) {
    // Construir la ruta completa del archivo  
    char* rutaCompleta = string_from_format("%s%s.txt",path_instrucciones ,path);

    // Generar instrucciones y cargarlas a la variable global PROCESO_INSTRUCCIONES
    t_list* instrucciones = leer_pseudocodigo(rutaCompleta);
    free(rutaCompleta);

    // Crear el objeto de proceso_instrucciones
    t_listaprincipal* proceso_instr = malloc(sizeof(t_listaprincipal));
    proceso_instr->pid = pid;
    proceso_instr->instrucciones = instrucciones;
	proceso_instr->tabla_de_paginas=list_create(); //solo crea la lista, pero arranca sin elementos ya que no tiene marcos asignados, se le agregan elementos del tipo fila_tabla_de_paginas
	push_con_mutex(proceso_instrucciones, proceso_instr, &mutex_lista_instrucciones);
}

t_solicitud_instruccion* recv_solicitar_instruccion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_solicitud_instruccion* solicitud_instruccion_recibida = malloc(sizeof(t_solicitud_instruccion));

	int* pid = list_get(paquete, 0);
	solicitud_instruccion_recibida->pid = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	solicitud_instruccion_recibida->program_counter = *program_counter;
	free(program_counter);

	list_destroy(paquete);
	return solicitud_instruccion_recibida;
}// RECIBE EL PEDIDO DE CPU, PID Y PROGRAM COUNTER
t_linea_instruccion* buscar_instruccion(int pid, int program_counter, t_list* proceso_instrucciones){
	int i = 0;
	
	pthread_mutex_lock(&proceso_instrucciones);
	t_listaprincipal* proceso_instr = list_get(proceso_instrucciones, i);
	pthread_mutex_unlock(&proceso_instrucciones);

	while(pid != proceso_instr->pid){
		i++;
		pthread_mutex_lock(&proceso_instrucciones);
		proceso_instr = list_get(proceso_instrucciones, i);
		pthread_mutex_unlock(&proceso_instrucciones);
	}

	return list_get(proceso_instr->instrucciones, program_counter);
}//BUSCA LA INSTRUCCION EN LA LISTA QUE CREAMOS CUANDO KERNEL PIDIO INCIAR PROCESO, YA ESTA CREADA EN UNA VARIABLE GLOBAL
void send_proxima_instruccion(int filedescriptor, t_linea_instruccion* instruccion){
	t_paquete* paquete = crear_paquete(PROXIMA_INSTRUCCION);

	agregar_a_paquete(paquete, &(instruccion->instruccion), sizeof(cod_instruccion));
	agregar_a_paquete(paquete, instruccion->parametro1, strlen(instruccion->parametro1) + 1);
	agregar_a_paquete(paquete, instruccion->parametro2, strlen(instruccion->parametro2) + 1);
	agregar_a_paquete(paquete, instruccion->parametro3, strlen(instruccion->parametro3) + 1);
	agregar_a_paquete(paquete, instruccion->parametro4, strlen(instruccion->parametro4) + 1);
	agregar_a_paquete(paquete, instruccion->parametro5, strlen(instruccion->parametro5) + 1);

	enviar_paquete(paquete, filedescriptor);
	eliminar_paquete(paquete);
}//envia la instruccion pedida a CPU serializado

void procesar_pedido_instruuccion(int socket_cpu, t_list* proceso_instrucciones){

	int retardo_respuesta = config_get_long_value(config_memoria, "RETARDO_RESPUESTA");
	t_solicitud_instruccion* solicitud_instruccion = recv_solicitar_instruccion(socket_cpu);

	t_linea_instruccion* instruccion_a_enviar = buscar_instruccion(solicitud_instruccion->pid, solicitud_instruccion->program_counter - 1, proceso_instrucciones);//es -1 xq como se va a llamar varias veces una vez que 
	free(solicitud_instruccion);
	usleep(retardo_respuesta*1000);//preguntar 
	send_proxima_instruccion(socket_cpu, instruccion_a_enviar);
}


