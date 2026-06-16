# esp32-labs

Meus experimentos aprendendo ESP32 com ESP-IDF e FreeRTOS.

## Labs de FreeRTOS
1. Pisca LED — primeiro contato com GPIO e tasks
2. Duas tasks — concorrência em ritmos diferentes
3. Prioridade e preempção
4. Filas (queues)
5. Botão + interrupção (ISR) + semáforo
6. Mutex
7. Software timer
8. Mini-projeto integrado

## Como rodar
```bash
. $IDF_PATH/export.sh
cd <pasta-do-lab>
idf.py -p /dev/ttyACM0 flash monitor
```

Documentação extra em `docs/`.
