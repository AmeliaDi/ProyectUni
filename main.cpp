#include <drogon/drogon.h>

using namespace drogon;

int main() {
    // Registramos una ruta (endpoint) en la raíz "/"
    app().registerHandler("/",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setBody("¡Hello thereeeeee jejej!");
            callback(resp);
        });

    LOG_INFO << "Servidor corriendo en http://127.0.0.1:8080";
    
    // Configuramos la IP, el puerto y arrancamos el ciclo de eventos del servidor
    app().addListener("127.0.0.1", 8080).run();
    
    return 0;
}