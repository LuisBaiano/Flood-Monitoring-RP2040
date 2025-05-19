# Estação de Alerta de Enchente com Simulação por Joystick usando FreeRTOS na BitDogLab

## Índice

- [Estação de Alerta de Enchente com Simulação por Joystick usando FreeRTOS na BitDogLab](#estação-de-alerta-de-enchente-com-simulação-por-joystick-usando-freertos-na-bitdoglab)
  - [Índice](#índice)
  - [Objetivos](#objetivos)
  - [Descrição do Projeto](#descrição-do-projeto)
    - [Simulação de Dados](#simulação-de-dados)
    - [Modos de Operação](#modos-de-operação)
    - [Arquitetura FreeRTOS](#arquitetura-freertos)
  - [Funcionalidades Implementadas](#funcionalidades-implementadas)
  - [Requisitos Técnicos Atendidos](#requisitos-técnicos-atendidos)
  - [Como Compilar e Executar](#como-compilar-e-executar)
    - [Requisitos de Hardware](#requisitos-de-hardware)
    - [Requisitos de Software](#requisitos-de-software)
    - [Passos para Compilação e Execução](#passos-para-compilação-e-execução)
  - [Estrutura do Código](#estrutura-do-código)
    - [Principais Arquivos](#principais-arquivos)
    - [Comunicação entre Tarefas](#comunicação-entre-tarefas)
  - [Sugestões para Criatividade e Impacto Social](#sugestões-para-criatividade-e-impacto-social)
  - [Demonstrativo em Vídeo](#demonstrativo-em-vídeo)

## Objetivos

*   Aplicar e consolidar os conhecimentos sobre o sistema operacional de tempo real FreeRTOS, com foco na criação e gerenciamento de tarefas.
*   Desenvolver um sistema embarcado multitarefa funcional na placa BitDogLab com o microcontrolador RP2040.
*   Implementar a comunicação entre tarefas utilizando **exclusivamente filas (Queues)**, conforme o requisito do projeto.
*   Simular um sistema de monitoramento de enchentes, processando dados de sensores (simulados pelo joystick) para ativar alertas.
*   Controlar múltiplos periféricos: Joystick (ADC), LED RGB (GPIO), Matriz de LEDs WS2812 (PIO), Buzzer (PWM) e Display OLED SSD1306 (I2C).
*   Criar um sistema com feedback visual e sonoro claro para os diferentes estados de alerta.
*   Estruturar o código de forma modular e comentada para facilitar o entendimento e a manutenção.

## Descrição do Projeto

Este projeto implementa uma "Estação de Alerta de Enchente" utilizando a placa BitDogLab (RP2040) e o FreeRTOS. O sistema monitora continuamente dados simulados de sensores para o nível da água e volume de chuva, ativando diferentes níveis de alerta e fornecendo feedback ao usuário através de múltiplos periféricos.

### Simulação de Dados

As leituras dos sensores são simuladas através dos eixos do joystick analógico:
*   **Eixo X do Joystick:** Simula o **nível da água** em um rio ou reservatório.
*   **Eixo Y do Joystick:** Simula o **volume de chuva** acumulado.

Estes valores analógicos são lidos pela task `vJoystickReadTask`, convertidos para porcentagens (0-100%) e enviados para processamento.

### Modos de Operação

O sistema opera com base nos níveis simulados, transitando entre dois modos principais:

*   **Modo Normal:**
    *   Indica que os níveis de água e chuva estão abaixo dos limiares de alerta.
    *   **Display OLED:** Exibe continuamente as porcentagens atuais de "NVL. AGUA" e "VOL. CHUVA", junto com o status "STATUS: NORMAL".
    *   **LED RGB:** Acende na cor Verde.
    *   **Matriz de LEDs:** Exibe um ícone de status normal (ex: um "check" ou um padrão tranquilo).
    *   **Buzzer:** Permanece em silêncio.

*   **Modo Alerta:**
    *   Ativado automaticamente pela `vDataProcessingTask` se:
        *   Nível da água ≥ 70% (definido por `WATER_LEVEL_ALERT_THRESHOLD`).
        *   **OU** Volume de chuva ≥ 80% (definido por `RAIN_VOLUME_ALERT_THRESHOLD`).
    *   **Display OLED:** Exibe "!!! ALERTA !!!" e uma mensagem específica descrevendo o tipo de alerta ("Nivel Agua Alto!", "Chuva Intensa!" ou "PERIGO MAXIMO!"), além das porcentagens.
    *   **LED RGB:** Acende na cor Vermelha.
    *   **Matriz de LEDs:** Exibe um ícone de alerta correspondente ao tipo de perigo (padrões para chuva alta, nível de água alto ou ambos).
    *   **Buzzer:** Emite padrões sonoros distintos e intermitentes, específicos para cada tipo de alerta (água alta, chuva alta, ou ambos).

### Arquitetura FreeRTOS

O sistema é gerenciado pelo FreeRTOS e é composto por múltiplas tarefas, cada uma com uma responsabilidade específica:

*   `vJoystickReadTask`: Lê os dados do joystick, converte para porcentagens e envia para a fila de dados do sensor.
*   `vDataProcessingTask`: Recebe os dados do sensor, determina o status e nível de alerta, e distribui (fan-out) o `AlertStatus_t` para as filas das tarefas de feedback.
*   `vDisplayInfoTask`: Recebe o `AlertStatus_t` e atualiza o display OLED.
*   `vRgbLedAlertTask`: Recebe o `AlertStatus_t` e controla o LED RGB.
*   `vLedMatrixAlertTask`: Recebe o `AlertStatus_t` e controla a Matriz de LEDs.
*   `vBuzzerAlertTask`: Recebe o `AlertStatus_t` e controla o Buzzer.

A comunicação entre a `vDataProcessingTask` e as tarefas de feedback é realizada por meio de **filas dedicadas** para cada tarefa consumidora, garantindo que cada uma receba uma cópia do estado de alerta.

## Funcionalidades Implementadas

```

✅ Leitura dos eixos X e Y do joystick para simular nível de água e volume de chuva.
✅ Conversão dos dados do joystick para porcentagens (0-100%).
✅ Task de processamento para analisar os dados e determinar o status/nível de alerta.
✅ Definição de limiares para ativação de alertas (70% para água, 80% para chuva).
✅ Modos de operação Normal e Alerta com transição automática.
✅ Controle do LED RGB para indicar status Normal (Verde) e Alerta (Vermelho).
✅ Exibição de padrões visuais distintos na Matriz de LEDs 5x5 WS2812 para:
    - Status Normal.
    - Alerta de Chuva Alta.
    - Alerta de Nível de Água Alto (indicador de preenchimento).
    - Alerta Combinado (Chuva + Água).
✅ Geração de padrões sonoros distintos e intermitentes no Buzzer (PWM) para:
    - Alerta de Nível de Água Alto.
    - Alerta de Chuva Alta.
    - Alerta Combinado (Chuva + Água).
✅ Exibição em tempo real das porcentagens de água/chuva e do status de alerta no display OLED SSD1306.
✅ Uso de múltiplas tarefas FreeRTOS dedicadas para cada funcionalidade principal.
✅ Comunicação entre tarefas implementada **exclusivamente através de filas (Queues)** do FreeRTOS, incluindo uma arquitetura fan-out.
✅ Sem utilização de semáforos ou mutexes, conforme requisito.
✅ Código estruturado em múltiplos arquivos (`main.c`, `config.h`, `buzzer.c/h`, `display.c/h`, `joystick.c/h`, `led_matrix.c/h`).
✅ Inicialização do modo BOOTSEL através do Botão B (GPIO 6).
✅ Comentários no código e documentação via `printf` para depuração.

```

## Requisitos Técnicos Atendidos

*(Baseado nos requisitos originais do enunciado do projeto)*

1.  **Consolidar conhecimentos sobre tarefas FreeRTOS:** O projeto utiliza múltiplas tarefas com responsabilidades distintas.
2.  **Utilização obrigatória de filas para comunicação:** Implementado com `xSensorDataQueue` e múltiplas filas de alerta para fan-out (`xDisplayAlertQueue`, `xRgbLedAlertQueue`, etc.).
3.  **Semáforos e mutexes não devem ser utilizados:** Cumprido.
4.  **Simulação de dados de nível de água e volume de chuva com joystick:** Implementado em `vJoystickReadTask`.
5.  **Processamento e geração de alertas visuais (LED RGB, Matriz de LEDs):** Implementado nas respectivas tasks.
6.  **Alertas sonoros com o buzzer:** Implementado em `vBuzzerAlertTask`.
7.  **Exibição em tempo real no display OLED:** Implementado em `vDisplayInfoTask`.
8.  **Controle de componentes por tarefas FreeRTOS:** Cada periférico tem sua task ou é gerenciado por uma task específica.
9.  **Modos de Operação Normal e Alerta (Nível água ≥ 70% ou Volume chuva ≥ 80%):** Lógica implementada em `vDataProcessingTask` e refletida nas tasks de feedback.
10. **Estrutura e Comentários:** O código foi estruturado em arquivos e comentado (javadoc parciais fornecidos nas tarefas).

## Como Compilar e Executar

### Requisitos de Hardware

*   Placa de desenvolvimento **BitDogLab** (com RP2040).
*   Cabo Micro-USB para conexão, alimentação e programação.
*   (Opcional) Conexão dos periféricos conforme `config.h` se estiver montando externamente.

### Requisitos de Software

*   **VS Code** com a extensão Pico-W-Go ou configuração manual do toolchain ARM e Pico SDK.
*   **Pico SDK** (ex: versão 1.5.1 ou mais recente) instalado e configurado.
*   **FreeRTOS Kernel:** O `CMakeLists.txt` deve estar configurado para encontrar o kernel do FreeRTOS. Assume-se que ele esteja como um submódulo ou em um caminho conhecido.
*   **Git** (opcional, para clonar o projeto se estiver em um repositório).
*   Um **Terminal Serial** (ex: Monitor Serial do VS Code, Putty, Minicom) configurado para a porta serial da Pico (geralmente 115200 baud).

### Passos para Compilação e Execução

1.  **Obter o Código:**
    *   Clone o repositório do projeto (se aplicável) ou organize os arquivos fornecidos (`main.c`, `config.h`, `buzzer.c/h`, `display.c/h`, `joystick.c/h`, `led_matrix.c/h`, `led_matrix.pio`) em uma estrutura de projeto adequada para o Pico SDK.
    *   Certifique-se que o arquivo `led_matrix.pio` está presente e que o `CMakeLists.txt` está configurado para processá-lo com `pico_generate_pio_header`.

2.  **Configurar FreeRTOS:**
    *   Assegure-se que o `FreeRTOSConfig.h` (incluído em `config.h` ou no diretório de includes) está correto para o RP2040.
    *   Verifique se o `CMakeLists.txt` localiza corretamente o `FreeRTOS-Kernel`.

3.  **Configurar o Projeto (CMakeLists.txt):**
    *   Verifique e ajuste os caminhos para o Pico SDK e FreeRTOS, se necessário.
    *   Confirme se `PICO_BOARD` está definido (ex: `pico` ou `pico_w`).
    *   Adicione todos os arquivos fonte (`.c`) e o arquivo `.pio` ao `CMakeLists.txt`.

4.  **Compilar (Build):**
    *   Usando VS Code: Utilize a função de build (geralmente Ctrl+Shift+B ou um ícone).
    *   Usando Linha de Comando (a partir da raiz do projeto):
        ```bash
        mkdir build
        cd build
        cmake ..
        make
        ```

5.  **Carregar o Firmware:**
    *   Com a placa BitDogLab desconectada, pressione e segure o botão **BOOTSEL**.
    *   Conecte a placa ao computador via cabo Micro-USB enquanto mantém BOOTSEL pressionado.
    *   Solte o botão BOOTSEL. A placa aparecerá como um drive USB chamado `RPI-RP2`.
    *   Copie o arquivo `.uf2` gerado (ex: `nome_do_projeto.uf2` ou `main.uf2`) da pasta `build` para o drive `RPI-RP2`.
    *   A placa reiniciará automaticamente e começará a executar o programa.

6.  **Visualizar Saída Serial:**
    *   Abra um terminal serial (como o Monitor Serial no VS Code, PuTTY, TeraTerm, minicom).
    *   Conecte-se à porta serial correspondente à sua placa Pico (verifique no gerenciador de dispositivos).
    *   Configure a velocidade para **115200 baud**, 8 data bits, no parity, 1 stop bit.
    *   Você deverá ver as mensagens de inicialização e depuração (`printf`).

7.  **Testar Funcionalidades:**
    *   Movimente o joystick e observe as mudanças nos valores percentuais no display OLED.
    *   Verifique se os modos Normal e Alerta são ativados corretamente quando os limiares são atingidos.
    *   Observe o comportamento do LED RGB, da Matriz de LEDs e do Buzzer em cada modo.

## Estrutura do Código

O código está organizado da seguinte forma (assumindo que os arquivos `.c` e `.h` dos drivers estão na raiz ou em um diretório simples):

### Principais Arquivos

*   `main.c`: Contém a função `main()`, a inicialização do sistema e a criação de todas as tarefas FreeRTOS. Também contém as implementações das tarefas.
*   `config.h`: Arquivo de configuração centralizado. Define pinos de hardware, constantes do sistema (limiares, delays, tamanhos de stack, prioridades), e inclui estruturas de dados (`SensorData_t`, `AlertStatus_t`) e enums (`AlertLevel_t`).
*   `joystick.c` / `joystick.h`: Lógica para inicialização e leitura do joystick (ADC).
*   `buzzer.c` / `buzzer.h`: Lógica para inicialização e controle do buzzer (PWM).
*   `display.c` / `display.h`: Lógica para inicialização do display OLED SSD1306 e função para tela de startup. *(Nota: As funções de desenho direto como `ssd1306_draw_string` são usadas na `vDisplayInfoTask` em `main.c`)*.
*   `led_matrix.c` / `led_matrix.h`: Lógica para inicialização da matriz de LEDs WS2812, controle via PIO, e funções para exibir os diferentes padrões de alerta.
*   `led_matrix.pio`: Programa em assembly PIO para controlar a serialização de dados para a matriz WS2812. Um arquivo `.pio.h` é gerado a partir deste.
*   `FreeRTOSConfig.h`: Arquivo de configuração específico do FreeRTOS, ajustado para o RP2040.

### Comunicação entre Tarefas

A comunicação entre as tarefas é realizada exclusivamente por meio de **Filas (Queues)** do FreeRTOS:

1.  **`xSensorDataQueue`**:
    *   **Produtor:** `vJoystickReadTask` (envia `SensorData_t`).
    *   **Consumidor:** `vDataProcessingTask` (recebe `SensorData_t`).

2.  **Filas de Alerta (Fan-Out):**
    *   **Produtor:** `vDataProcessingTask` (envia `AlertStatus_t` para todas as filas abaixo).
    *   **Consumidores:**
        *   `xDisplayAlertQueue`: Consumida por `vDisplayInfoTask`.
        *   `xRgbLedAlertQueue`: Consumida por `vRgbLedAlertTask`.
        *   `xLedMatrixAlertQueue`: Consumida por `vLedMatrixAlertTask`.
        *   `xBuzzerAlertQueue`: Consumida por `vBuzzerAlertTask`.

Este design garante que as tarefas sejam desacopladas e que cada componente de feedback reaja ao estado de alerta mais recente de forma independente.

## Demonstrativo em Vídeo

*(O aluno deverá inserir aqui o link para o vídeo de demonstração do projeto, conforme solicitado nos requisitos da tarefa).*
