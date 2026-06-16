# esp32-labs

Meus experimentos aprendendo **ESP32** com **ESP-IDF** e **FreeRTOS**.
Cada pasta numerada e um projeto independente — faca na ordem.

## Labs de FreeRTOS

| # | Pasta | O que ensina |
|---|-------|--------------|
| 01 | `01-blink` | GPIO e o primeiro pisca-LED |
| 02 | `02-duas-tasks` | duas tasks concorrentes em ritmos diferentes |
| 03 | `03-prioridade` | prioridade e preempcao (causar e consertar starvation) |
| 04 | `04-fila` | passar dados entre tasks com filas (queues) |
| 05 | `05-botao-isr` | botao via interrupcao (ISR) + semaforo |
| 06 | `06-mutex` | proteger recurso compartilhado com mutex |
| 07 | `07-timer` | agendar com software timer |
| 08 | `08-projeto-integrado` | tudo junto: ISR + semaforo + fila + tasks + timer |

Varios labs tem um **EXPERIMENTO** comentado no topo do `main.c` — vale testar.

## Hardware

- ESP32 DevKit + cabo USB
- LED no GPIO2 (onboard na maioria das placas, ou externo via resistor 330 Ohm)
- Botao entre GPIO4 e GND (labs 05 e 08)

## Como rodar um lab

```bash
. $IDF_PATH/export.sh          # carrega o ESP-IDF
cd 01-blink                    # entra na pasta do lab
idf.py set-target esp32        # uma vez por pasta
idf.py -p /dev/ttyACM0 flash monitor
```

Sair do monitor: `Ctrl+]`.

## Documentacao

Guias em HTML (abrir no navegador) ficam em `docs/`.

## Adicionar um lab novo

1. Crie uma pasta nova (ex.: `09-meu-sensor/`) com a mesma estrutura.
2. `git add . && git commit -m "lab 09: meu sensor" && git push`

Projeto novo = pasta nova. Nada e sobrescrito.
