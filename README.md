# 🔧 Ohmímetro com Código de Cores - BitDogLab

Este projeto consiste em um **ohmímetro digital** desenvolvido com a placa **BitDogLab** (RP2040), capaz de medir a resistência de um resistor desconhecido e identificar suas **faixas de cor** conforme o padrão da **série E24**.

O sistema exibe as informações tanto no **display OLED** quanto na **matriz de LEDs WS2812**, que acende com as **cores reais** correspondentes às faixas do resistor.

---

## 🧠 Como funciona

- Mede a resistência com o ADC interno (GPIO 28)
- Arredonda o valor para o mais próximo da série E24
- Determina as três primeiras faixas do código de cores
- Mostra:
  - Valor numérico no display
  - Nome das cores
  - Desenho estilizado de um resistor com as faixas
  - Cores reais acesas na matriz WS2812


## 🎥 Demonstração

https://drive.google.com/file/d/1Ekxem0wgdaoUFl1nND2i3r_ARxcWaUYG/view?usp=sharing

---

## 👤 Autora

**Anna Beatriz Silva Lima**  
Residência EmbarcaTech — Feira de Santana  


