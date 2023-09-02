usage: server <thread|process>
example:
server thread   'run as multithread'
server process  'run as multiprocess'

limitations:

- only support text data
  - null charactor(\0) may cause unexpected error 
- limit to small size file(up to 1024 bytes)
