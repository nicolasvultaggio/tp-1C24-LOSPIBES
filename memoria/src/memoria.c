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

    log_info(logger_memoria, "Esperando CPU");
    fd_conexion_cpu = esperar_cliente(fd_escucha_memoria, logger_memoria, "CPU");
  
    log_info(logger_memoria, "Esperando KERNEL");
    fd_conexion_kernel = esperar_cliente(fd_escucha_memoria, logger_memoria, "KERNEL");
    
    log_info(logger_memoria, "Esperando IO");
    fd_conexion_io = esperar_cliente(fd_escucha_memoria, logger_memoria, "IO");
    
    args_atendedor * args_cpu = crear_args(fd_conexion_cpu,logger_memoria,"CPU");
    pthread_t hilo_cpu;
    pthread_create(&hilo_cpu,NULL,(void*) atender_conexion,(void *)args_cpu);
    pthread_detach(hilo_cpu);

    //atender a Kernel-Interrupt
    args_atendedor * args_kernel = crear_args(fd_conexion_kernel,logger_memoria,"Kernel");
    pthread_t hilo_kernel;
    pthread_create(&hilo_kernel,NULL,(void*) atender_conexion,(void *)args_kernel);
    pthread_detach(hilo_kernel);

    //atender a Memoria
    args_atendedor * args_io = crear_args(fd_conexion_io,logger_memoria,"IO");
    pthread_t hilo_io;
    pthread_create(&hilo_io,NULL,(void*)atender_conexion,(void *)args_io);
    // pthread_detach(hilo_memoria); no hago detach porque me terminaría el programa, terminando mis otros hilos
    pthread_join(hilo_io,NULL); // así, la cpu se ejecuta hasta que terminen todos los hilos
    



    //***************************CHECKPOINT 2*************************+
    
    // Iniciar conexiones y esperar y creo lista de procesos
	
	while(server_escuchar(fd_escucha_memoria));

	// Termino programa
	terminar_programa(logger_memoria, config_memoria);
    
    
}

//Esta pensado para que en un futuro lea mas de una configuracion. 
void leer_configuraciones(){
    puerto_propio = config_get_string_value(config_memoria,"PUERTO_PROPIO");
     tam_memoria=config_get_int_value(config_memoria, "TAM_MEMORIA");
    tam_pagina= config_get_int_value(config_memoria,"TAM_PAGINA");
    path_instrucciones=config_get_string_value(config_memoria,"PATH_INSTRUCCIONES");
    retardo_respuesta= config_get_int_value(config_memoria,"RETARDO_RESPUESTA");
}

void terminar_programa(){
    config_destroy(config_memoria);
    log_destroy(logger_memoria);
}
t_list leer_pseudocodigo(char* ruta){
    
    t_list* instrucciones = list_create();
    FILE* f;
    char buffer[256];
    char* palabra;
    char* instruccion_leida= NULL;
    char* parametros[5];
    int contadordeparametros;

    f=fopen(ruta;"r");
	while (fgets(buffer, 256, f) != NULL) {
		t_instruccion* instruccion = malloc(sizeof(t_instruccion));
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
            codigo_instrucciones cod_inst = instruccion_to_enum(instruccion_leida);

            instruccion->instruccion = cod_inst;
            if(param_count > 0){
            	strcpy(instruccion->parametro1, parametros[0]);
            }else{
            	strcpy(instruccion->parametro1, "");
            }
            if(param_count > 1){
            	strcpy(instruccion->parametro2, parametros[1]);
            }else{
            	strcpy(instruccion->parametro2, "");
            }
            if(param_count > 2){
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
	

int server_escuchar() {
	char* server_name = "SOY UN CLIENTE";
	 
	int cliente_socket = esperar_cliente(fd_escucha_memoria, logger_memoria, server_name);

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
static void procesar_clientes(void* void_args){
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger_memoria, "El cliente se desconecto de %s server", server_name);
			return;
		}
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger_memoria, cliente_socket);
			break;
		case PAQUETE:
			t_list *paquete_recibido = recibir_paquete(cliente_socket);
			log_info(logger, "Recibí un paquete con los siguientes valores: ");
			list_iterate(paquete_recibido, (void*) iterator);
			break;	
		case DATOS_PROCESO: //se arregla cuando unamos ramas,  en protocolo.h, SINO agregar a struct op_code de protocolo.h
				t_datos_proceso* datos_proceso = recibir_datos_del_proceso(cliente_socket);//se arregla cuando unamos ramas,se halla en protocolo.h cuando
				iniciar_memoria_apedidodeKernel(datos_proceso->path, datos_proceso->pid, cliente_socket);//el parametro size sera usado en el 3er check,"datos_proceso->size"
				free(datos_proceso->path);
				free(datos_proceso);
			break;
		
		/*case PEDIDO_LECTURA_CPU:  // NO APLICA CASE XQ NO SE LEE MEMORIA EN ESTE CHECK solo operaciones CPUbound
			t_list* parametros_lectura_cpu= recv_leer_valor(cliente_socket);
			int* posicion_lectura_cpu = list_get(parametros_lectura_cpu, 0);
			int* tamanio_lectura_cpu = list_get(parametros_lectura_cpu, 1);
			int* pid_lectura_cpu = list_get(parametros_lectura_cpu, 2);
			char* valor_leido_cpu = malloc(*tamanio_lectura_cpu);
			usleep(RETARDO_MEMORIA * 1000);
			memcpy(valor_leido_cpu, espacio_usuario + *posicion_lectura_cpu, *tamanio_lectura_cpu);
			log_info(logger_obligatorio, "PID: %d - Acción: LEER - Dirección física: %d - Tamaño: %d - Origen: CPU",*pid_lectura_cpu, *posicion_lectura_cpu, *tamanio_lectura_cpu);
			log_valor_espacio_usuario(valor_leido_cpu, *tamanio_lectura_cpu);
			send_valor_leido_cpu(valor_leido_cpu, *tamanio_lectura_cpu, cliente_socket);
			break;*/
		
		case SOLICITAR_INSTRUCCION:// preguntar a sergio como envia el pedido ojo que es distinto a como lo va recibir el
				log_info(logger_memoria, "Solicitud de instruccion recibida");
				procesar_pedido_instruuccion(cliente_socket, proceso_instrucciones);
				break;
		
		default:
				log_error(logger_memoria, "Codigo de operacion no reconocido en el server de %s", server_name);
				return;
			}

		}
	log_warning(logger, "El cliente se desconecto de %s server", "memoria");
	return;
}
codigo_instrucciones instruccion_to_enum(char* instruccion){
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
void iniciar_memoria_apedidodeKernel(char* path, int size, int pid, int socket_kernel) {
    // Construir la ruta completa del archivo
    char* rutaCompleta = string_from_format("/home/utnso/tp-2023-2c-Sisop-Five/mappa-pruebas/%s.txt", path);//CAMBIAR RUTAAA

    // Generar instrucciones y cargarlas a la variable global PROCESO_INSTRUCCIONES
    t_list* instrucciones = leer_pseudocodigo(rutaCompleta);
    free(rutaCompleta);

    // Crear el objeto de proceso_instrucciones
    t_listaprincipal* proceso_instr = malloc(sizeof(t_listaprincipal));
    proceso_instr->pid = pid;
    proceso_instr->instrucciones = instrucciones;

    list_add(proceso_instrucciones, proceso_instr);

    /* Crear la tabla de páginas
    t_tdp* tdp = malloc(sizeof(t_tdp));
    tdp->pid = pid;

    int cant_paginas = size / tam_pagina;
    t_list* paginas = list_create();

    for (int i = 0; i < cant_paginas; i++) {
        t_pagina* pag = malloc(sizeof(t_pagina));
        pag->pid = pid;
        pag->numpag = i;
        pag->marco = -1;
        pag->bit_presencia = 0;
        pag->bit_modificado = 0;
        list_add(paginas, pag);
    }

    tdp->paginas = paginas;//FUTURA IMPLEMENTACION PAGINACION SIMPLE!!!

    list_add(tablas_de_paginas, tdp);
    log_info(logger, "Tabla de paginas creada. PID: %d - Tamaño: %d\n", pid, cant_paginas);*/
}
/*t_datos_proceso* recv_proceso_dekernel(int socket_1){
	t_list* paquetes = recibir_paquete(fd);
	t_datos_proceso* datos = malloc(sizeof(t_datos_proceso));
	char* path = list_get(paquetes, 0);
	datos->path = malloc(strlen(path));
	strcpy(datos->path, path);
	free(path);

	int* size = list_get(paquetes, 1);
	datos->size = *size;
	free(size);

	int* pid = list_get(paquetes, 2);
	datos->pid = *pid;
	free(pid);

	list_destroy(paquetes);
	return datos;
}*/ //LO COMENTO PORQUE TOMI YA lo hizo en protrocolo
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
t_instruccion* buscar_instruccion(int pid, int program_counter, t_list* proceso_instrucciones){
	int i = 0;
	t_listaprincipal* proceso_instr = list_get(proceso_instrucciones, i);

	while(pid != proceso_instr->pid){
		i++;
		proceso_instr = list_get(proceso_instrucciones, i);
	}

	return list_get(proceso_instr->instrucciones, program_counter);
}//BUSCA LA INSTRUCCION EN LA LISTA QUE CREAMOS CUANDO KERNEL PIDIO INCIAR PROCESO, YA ESTA CREADA EN UNA VARIABLE GLOBAL
void send_proxima_instruccion(int filedescriptor, t_instruccion* instruccion){
	t_paquete* paquete = crear_paquete(PROXIMA_INSTRUCCION);///MUUUUY IMPORTANTE PREGUNTAR A SERGIO CON QUE NOMBRE LO RECIBE Y AGREGARLO AL op_code sino esta

	agregar_a_paquete(paquete, &(instruccion->instruccion1), sizeof(codigo_instrucciones));
	agregar_a_paquete(paquete, instruccion->parametro1, strlen(instruccion->parametro1) + 1);
	agregar_a_paquete(paquete, instruccion->parametro2, strlen(instruccion->parametro2) + 1);
	agregar_a_paquete(paquete, instruccion->parametro3, strlen(instruccion->parametro3) + 1);
	agregar_a_paquete(paquete, instruccion->parametro4, strlen(instruccion->parametro4) + 1);
	agregar_a_paquete(paquete, instruccion->parametro5, strlen(instruccion->parametro5) + 1);

	enviar_paquete(paquete, filedescriptor);
	eliminar_paquete(paquete);
}//envia la instruccion pedida a CPU serializado

void procesar_pedido_instruuccion(int socket_cpu, t_list* proceso_instrucciones){

	int retardo_respuesta = config_get_long_value(config, "RETARDO_RESPUESTA");
	t_solicitud_instruccion* solicitud_instruccion = recv_solicitar_instruccion(socket_cpu);
	/*t_listaprincipal* pruebita = list_get(proceso_instrucciones, 0);
//	t_instruccion* pruebita2 = list_get(pruebita->instrucciones, 0);*/

	t_instruccion* instruccion_a_enviar = buscar_instruccion(solicitud_instruccion->pid, solicitud_instruccion->program_counter - 1, proceso_instrucciones);//es -1 xq como se va a llamar varias veces una vez que 
	free(solicitud_instruccion);
	usleep(retardo_respuesta*1000);//preguntar 
	send_proxima_instruccion(socket_cpu, instruccion_a_enviar);
}

