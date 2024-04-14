#include <../../memoria/include/memoria.h>

int main() {
    
    decir_hola("Memoria, verdad y justicia");

    logger_memoria = log_create("memoria_logs.log","memoria",1,LOG_LEVEL_INFO);
    config_memoria = config_create("./configs/memoria.config");
    leer_configuraciones();

    if((socket_cliente_MODULO = iniciar_conexiones()) == 1){
        log_error(logger_memoria,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }
    
    t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_cliente_MODULO);
		switch (cod_op) {
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
	}
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
    
    log_info(logger_memoria, "Memoria espera contacto");

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