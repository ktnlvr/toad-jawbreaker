# Путь Пакета

Этот документ описывает полный путь пакета на нескольких уровнях абстракции. Заранее ознакомьтесь с [глоссарием](./../glossary.md). 

Текст формата `X?` означает запрос, а `X!` означает ответ. 
Пример использования:

```mermaid
sequenceDiagram
    participant Client as 127.0.0.1
    participant Server as 192.168.0.1

    Client->>Server: SYN?
    Server->>Client: SYN! ACK?
    Client->>Server: ACK!
```

Ниже показана неполная диаграмма взаимодействия. На ней скрыти детали направления конкретных пакетов. Toad представляет из себя _всю_ сеть.

```mermaid
sequenceDiagram
    participant Client as 127.0.0.1:* <br> (SOCKS5 Client)<br> e.g. Firefox
    participant Server as 127.0.0.1:9150 <br> (SOCKS5 Server) <br> 71.92.114.3
    participant Toad as Toad <br> (represents the <br>entire network)

    Server->>Toad: handshake?
    Toad->>Server: handshake!

    Client->>Server: GREET? <br> VER=5, METHODS=[0x00]
    Server->>Client: GREET! <br> VER=5, METHOD=0x00
    Client->>Server: CONNECT? <br> 10.12.14.16 : 8080
    
    Server->>Toad: CONNECT? <br> 10.12.14.16
    Toad->>10.12.14.16: попытка соедениться 
    10.12.14.16->>Toad: соединение успешно

    Toad->>Server: CONNECT!


    Server->>Client: CONNECT!

    par Конкретный паттерн передачи данных может отличаться
        Client->>Server: данные от клиента
        10.12.14.16->>Toad: данные от сервера
        Server->>Toad: данные от клиента <br> dst=10.12.14.16
        Toad->>10.12.14.16: данные от клиента
        Toad->>Server: данные от сервера <br> dst=71.92.114.3 
        Server->>Client: данные от сервера
    end
```
