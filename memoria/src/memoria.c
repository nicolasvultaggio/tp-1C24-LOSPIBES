#include <../../memoria/include/memoria.h>
#include <utils/include/hello.h>

int main(int argc, char* argv[]) {
    
    decir_hola("Memoria, verdad y justicia");

    logger = log_create("memoria_logs.log","memoria",1,LOG_LEVEL_INFO);
    config = config_create("./configs/memoria.config");
    leer_configuraciones();
    if(!iniciar_conexiones()){
        log_error(logger,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }
    
    return 0;
}

void leer_configuraciones(){
    PUERTO_ESCUCHA = config_get_string_value(config,"PUERTO_ESCUCHA");
    TAM_MEMORIA = config_get_string_value(config,"TAM_MEMORIA");
    TAM_PAGINA = config_get_string_value(config,"TAM_PAGINA");
    PATH_INSTRUCCIONES = config_get_string_value(config,"PATH_INSTRUCCIONES");
    RETARDO_RESPUESTA = config_get_string_value(config,"RETARDO_RESPUESTA");

}

bool iniciar_conexiones(){

    socket_server_MEMORIA = iniciar_servidor(NULL, PUERTO_ESCUCHA);
    
    log_info(logger, "Memoria recibe ordenes");

    socket_cliente_MODULO = esperar_cliente(socket_server_MEMORIA);

    return socket_cliente_MODULO; 
}

void terminar_programa(){
    config_destroy(config);
    log_destroy(logger);
}