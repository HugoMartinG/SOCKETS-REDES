# 📡 Servidor FTP en C++

> Servidor FTP multihilo escrito en C++ con sockets POSIX, siguiendo el estándar [RFC 959](https://www.rfc-editor.org/rfc/rfc959).

**Stack:** C++ · Sockets TCP · pthreads · Make · Linux

## ✨ Qué hace

- Acepta **varios clientes a la vez**: cada conexión se atiende en su propio hilo (`pthread`)
- Separa el **canal de control** (comandos) del **canal de datos** (ficheros), como define el protocolo
- Funciona con clientes FTP reales (`ftp` de Linux, FileZilla…)

### Comandos implementados

| Comando | Función |
|---|---|
| `USER` / `PASS` | Autenticación |
| `PORT` | Modo activo: el servidor se conecta al cliente |
| `PASV` | Modo pasivo: el cliente se conecta al servidor |
| `LIST` | Listar el directorio |
| `RETR` | Descargar un fichero |
| `STOR` | Subir un fichero |
| `TYPE` / `SYST` / `QUIT` | Tipo de transferencia, sistema y cierre |

## 🧱 Estructura

```
src/
├── ftp_server.cpp        # Punto de entrada
├── FTPServer.h/.cpp      # Socket de escucha y creación de un hilo por cliente
├── ClientConnection.h/.cpp  # Intérprete de comandos y canal de datos
└── common.h
```

## ▶️ Compilar y probar

```bash
cd src
make
./ftp_server            # escucha en el puerto configurado

# En otra terminal:
ftp localhost <puerto>
```

![Sesión de ejemplo con un cliente FTP](docs/demo.png)

## 📚 Qué aprendí

- Cómo funcionan los sockets TCP a bajo nivel (`socket`, `bind`, `listen`, `accept`, `connect`)
- La diferencia práctica entre los modos activo y pasivo de FTP y por qué el pasivo funciona mejor detrás de NAT
- Concurrencia básica con hilos y los problemas que aparecen al compartir recursos

---
Práctica de *Redes y Sistemas Distribuidos*, Grado en Ingeniería Informática, Universidad de La Laguna.
