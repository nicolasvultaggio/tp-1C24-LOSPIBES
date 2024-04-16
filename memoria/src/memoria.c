#include <../../memoria/include/memoria.h>
int main() {
    
    decir_hola("Memoria, verdad y justicia");
    

    logger_memoria = log_create("memoria_logs.log","memoria",1,LOG_LEVEL_INFO);
    config_memoria = config_create("./configs/memoria.config");
    leer_configuraciones();
    
    socket_server_MEMORIA = iniciar_servidor(NULL, PUERTO_ESCUCHA);
    
    log_info(logger_memoria, "Puerto de memoria habilitado para sus clientes");

    
    while (socket_cliente_MODULO = esperar_cliente(socket_server_MEMORIA))
    {
        log_info(logger_memoria,"Se conecto un cliente");
        int cod_op = recibir_operacion(socket_cliente_MODULO);
        switch (cod_op)
        {
        case 0:
            enviar_mensaje_de_exito(socket_cliente_MODULO, "Mensaje desde memoria a CPU");
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, cpu");
            break;
        case 1:
            enviar_mensaje_de_exito(socket_cliente_MODULO, "Mensaje desde memoria a Kernel");
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, kernel");
            break;
        case 2:
            enviar_mensaje_de_exito(socket_cliente_MODULO, "Mensaje desde memoria a Interfaz IO");   
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, Interfaz IO");
            break;
        default:
            log_info(logger_memoria, "Codigo de operacion no reconocido en el server");
            log_info(logger_memoria,"Ya te mande el valor");
            break;
        }
    }
    

    
    /*
    while(1){
        socket_cliente_MODULO = accept(socket_server_MEMORIA,NULL,NULL);
        log_info(logger_memoria, "Cliente conectado");

        while(!socket_cliente_MODULO){
            handshakeSERVIDOR(socket_cliente_MODULO,handshakeDeCPU, conexionExitosaCPU, noCoincideHandshakeCPU);
        }

    }
    //socket_cliente_MODULO = accept(socket_server_MEMORIA,NULL,NULL);

    // hace handshakeSERVIDOR();
    //despues vuelve a hacer accept
    
    

    /*
    if((socket_cliente_MODULO = iniciar_conexiones()) == 1){ // socket_cliente_modulo representa la conexion que esta esperando un cliente
        log_error(logger_memoria,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }
    
    // t_list* lista;
   
    while (1) {
		int cod_op = recibir_operacion(socket_cliente_MODULO);
		switch (cod_op) {
        // case HANDSHAKE:
        //    if(cod_op)
        case MENSAJE:
			recibir_mensaje(socket_cliente_MODULO);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_cliente_MODULO);
			log_info(logger_memoria, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(logger_memoria, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger_memoria,"Operacion desconocida. No quieras meter la pata");
			break;
		}
    */
	
        
        
	
	return EXIT_SUCCESS;
    return 0;
    
}

void leer_configuraciones(){
    PUERTO_ESCUCHA = config_get_string_value(config_memoria,"PUERTO_ESCUCHA");
    TAM_MEMORIA = config_get_string_value(config_memoria,"TAM_MEMORIA");
    TAM_PAGINA = config_get_string_value(config_memoria,"TAM_PAGINA");
    PATH_INSTRUCCIONES = config_get_string_value(config_memoria,"PATH_INSTRUCCIONES");
    RETARDO_RESPUESTA = config_get_string_value(config_memoria,"RETARDO_RESPUESTA");

}

int iniciar_conexiones(){

    socket_server_MEMORIA = iniciar_servidor(NULL, PUERTO_ESCUCHA);
    
    log_info(logger_memoria, "Puerto de memoria habilitado para sus clientes");

    socket_cliente_MODULO = esperar_cliente(socket_server_MEMORIA);

    return socket_cliente_MODULO; 
}

void terminar_programa(){
    config_destroy(config_memoria);
    log_destroy(logger_memoria);
}

void iterator(char* value) {
	log_info(logger_memoria,"%s", value);
}