# ESP32 Support Status

## ✅ O Que Funciona AGORA

### Binário Bare-Metal Pronto

Você pode usar o ESP32 **AGORA** com um binário pré-compilado:

```bash
# Gravar no ESP32
cd examples/rtos_blink_esp32
esptool.py --chip esp32 --port /dev/cu.usbserial-XXXX --baud 921600 \
    write_flash -z 0x1000 ../../build/examples/rtos_blink_esp32/rtos_blink_esp32.bin
```

**Características**:
- ✅ RTOS funcionando com 3 tasks
- ✅ LED piscando
- ✅ Logging UART básico
- ✅ 348KB, pronto para uso
- ⚠️  Versão bare-metal simplificada

**Veja**: `examples/rtos_blink_esp32/FLASH.md`

---

## 🚧 Em Desenvolvimento

### Integração ESP-IDF Completa

Estamos trabalhando em integração total com ESP-IDF para:
- ESP-IDF logging completo
- WiFi, Bluetooth
- Todos os components ESP-IDF
- Build system ESP-IDF nativo

**Status**: Arquitetura definida, implementação em progresso

**Para contribuir**: Veja `cmake/platform/esp32_integration.cmake`

---

## Para Usuários

**Quer usar ESP32 agora?**
→ Use o binário bare-metal (veja `FLASH.md`)

**Quer contribuir com ESP-IDF integration?**
→ Veja issues no GitHub ou `cmake/platform/esp32_integration.cmake`

**Tem outra placa (STM32, RP2040, SAMD21)?**
→ Tudo funciona 100%! Veja `README.md` principal
