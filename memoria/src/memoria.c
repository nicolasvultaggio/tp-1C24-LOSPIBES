#include <../../memoria/include/memoria.h>
int main() {
    
    decir_hola("Memoria, verdad y justicia");
    
    logger_memoria = log_create("memoria_logs.log","memoria",1,LOG_LEVEL_INFO);
    config_memoria = config_create("./configs/memoria.config");
    leer_configuraciones();
    
    fd_escucha_memoria = iniciar_servidor(NULL, puerto_propio);
    
    log_info(logger_memoria, "Puerto de memoria habilitado para sus clientes");

    
    while ((fd_conexion_server = esperar_cliente(fd_escucha_memoria)) != (-1))
    {
        log_info(logger_memoria,"Se conecto un cliente");
        handshake cod_op = recibir_operacion(fd_conexion_server);
        switch (cod_op)
        {
        case handshakeCPU:
            enviar_mensaje_de_exito(fd_conexion_server, "Mensaje desde memoria a CPU");
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, cpu");
            break;
        case handshakeKERNEL:
            enviar_mensaje_de_exito(fd_conexion_server, "Mensaje desde memoria a Kernel");
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, kernel");
            break;
        case handshakeIO:
            enviar_mensaje_de_exito(fd_conexion_server, "Mensaje desde memoria a Interfaz IO");   
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, Interfaz IO");
            break;
        default:
            log_info(logger_memoria, "Codigo de operacion no reconocido en el server");
            log_info(logger_memoria,"Ya te mande el valor");
            break;
        }
        
    }
    //NO cerramos conexion porq no esta hecho con memoria dinamica

	return EXIT_SUCCESS;    
}

//Esta pensado para que en un futuro lea mas de una configuracion. 
void leer_configuraciones(){
    puerto_propio = config_get_string_value(config_memoria,"PUERTO_PROPIO");
}

void terminar_programa(){
    config_destroy(config_memoria);
    log_destroy(logger_memoria);
}
