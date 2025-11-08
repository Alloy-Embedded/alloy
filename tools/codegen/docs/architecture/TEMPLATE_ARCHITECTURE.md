# Arquitetura de Templates Unificada para Codegen

## 🎯 Objetivo

Unificar **TODOS** os geradores de código usando:
1. **Templates Jinja2** - Para a estrutura do código gerado
2. **Metadata JSON** - Para configurações específicas de família/vendor
3. **Parser SVD** - Para extrair dados dos arquivos SVD

## 📋 Benefícios

✅ **Manutenibilidade** - Mudanças na estrutura do código afetam apenas templates  
✅ **Consistência** - Todos os arquivos gerados seguem o mesmo padrão  
✅ **Extensibilidade** - Adicionar novo vendor/família = criar novo JSON  
✅ **Testabilidade** - Templates podem ser testados independentemente  
✅ **Documentação** - Templates servem como documentação visual  
✅ **Versionamento** - Mudanças em templates = mudanças rastreáveis no Git  

## 🏗️ Estrutura Proposta

```
tools/codegen/
├── cli/
│   ├── generators/
│   │   ├── unified_generator.py        # Gerador unificado
│   │   ├── generate_all.py             # Orquestra todos
│   │   └── metadata/                   # Metadata por família/vendor
│   │       ├── vendors/
│   │       │   ├── atmel.json          # Config vendor Atmel
│   │       │   ├── st.json             # Config vendor ST
│   │       │   └── nxp.json            # Config vendor NXP
│   │       └── families/
│   │           ├── same70.json         # Config família SAME70
│   │           ├── stm32f4.json        # Config família STM32F4
│   │           └── lpc55.json          # Config família LPC55
│   └── parsers/
│       └── generic_svd.py              # Parser SVD existente
└── templates/
    ├── registers/
    │   ├── register_struct.hpp.j2      # Template estrutura registros
    │   └── register_namespace.hpp.j2   # Template namespace wrapper
    ├── bitfields/
    │   ├── bitfield_enum.hpp.j2        # Template enums bitfield
    │   └── bitfield_masks.hpp.j2       # Template máscaras
    ├── peripherals/
    │   ├── gpio.hpp.j2                 # Template GPIO (já existe!)
    │   ├── uart.hpp.j2                 # Template UART
    │   ├── spi.hpp.j2                  # Template SPI
    │   └── i2c.hpp.j2                  # Template I2C
    ├── platform/
    │   ├── hardware.hpp.j2             # Template hardware.hpp
    │   ├── pins.hpp.j2                 # Template pin definitions
    │   └── clocks.hpp.j2               # Template clock config
    ├── startup/
    │   ├── startup.c.j2                # Template startup code
    │   └── vector_table.c.j2           # Template vector table
    └── linker/
        └── linker_script.ld.j2         # Template linker script
```

## 📄 Exemplo: Metadata JSON para Registradores

### `metadata/families/same70.json`

```json
{
  "family": "same70",
  "vendor": "atmel",
  "architecture": "cortex-m7",
  
  "register_generation": {
    "namespace_format": "alloy::hal::{vendor}::{family}::{peripheral}",
    "struct_naming": "{peripheral}_Registers",
    "reserved_naming": "RESERVED_{offset:04X}",
    "include_guards": "ALLOY_HAL_{VENDOR}_{FAMILY}_{PERIPHERAL}_REGISTERS_HPP",
    
    "type_mappings": {
      "8": "uint8_t",
      "16": "uint16_t",
      "32": "uint32_t",
      "64": "uint64_t"
    },
    
    "array_handling": {
      "detect_pattern": true,
      "max_array_size": 128,
      "comment_each_element": false
    },
    
    "documentation": {
      "include_base_address": true,
      "include_reset_values": true,
      "include_access_mode": true,
      "max_description_length": 80
    }
  },
  
  "bitfield_generation": {
    "enum_style": "enum class",
    "mask_prefix": "",
    "shift_suffix": "_Pos",
    "mask_suffix": "_Msk",
    "generate_helpers": true
  },
  
  "peripheral_overrides": {
    "PIO": {
      "register_include": "hal/vendors/atmel/same70/registers/pioa_registers.hpp",
      "generate_bitfields": true,
      "gpio_compatible": true
    },
    "UART": {
      "register_include": "hal/vendors/atmel/same70/registers/uart_registers.hpp",
      "generate_bitfields": true
    }
  }
}
```

### `metadata/vendors/atmel.json`

```json
{
  "vendor": "atmel",
  "vendor_normalized": "atmel",
  "vendor_display": "Atmel (Microchip)",
  
  "common_settings": {
    "endianness": "little",
    "pointer_size": 32,
    "default_register_size": 32
  },
  
  "naming_conventions": {
    "peripheral_prefix": "",
    "register_case": "UPPER",
    "field_case": "UPPER",
    "enum_case": "PascalCase"
  },
  
  "families": {
    "same70": {
      "svd_pattern": "ATSAME70*.svd",
      "cortex_core": "M7",
      "fpu": true,
      "dsp": true
    },
    "samd21": {
      "svd_pattern": "ATSAMD21*.svd",
      "cortex_core": "M0+",
      "fpu": false,
      "dsp": false
    }
  }
}
```

## 🎨 Exemplo: Template Jinja2 para Registradores

### `templates/registers/register_struct.hpp.j2`

```jinja
{# Auto-generated register definitions #}
/// Auto-generated register definitions for {{ peripheral.name }}
/// {{ metadata.device_comment }}
/// Vendor: {{ device.vendor }}
///
/// DO NOT EDIT - Generated by Alloy Code Generator from CMSIS-SVD

#pragma once

#include <stdint.h>

{% if metadata.register_generation.extra_includes %}
{% for include in metadata.register_generation.extra_includes %}
#include "{{ include }}"
{% endfor %}
{% endif %}

namespace {{ namespace }} {

// ============================================================================
// {{ peripheral.name }} - {{ peripheral.description | sanitize }}
{% if metadata.register_generation.documentation.include_base_address %}
// Base Address: 0x{{ "%08X" | format(peripheral.base_address) }}
{% endif %}
// ============================================================================

/// {{ peripheral.name }} Register Structure
struct {{ peripheral.name }}_Registers {
{% set current_offset = 0 %}
{% for register in peripheral.registers | sort(attribute='offset') %}
    {# Add padding if needed #}
    {% if register.offset > current_offset %}
    {% set padding_bytes = register.offset - current_offset %}
    {% set padding_name = "RESERVED_%04X" | format(current_offset) %}
    uint8_t {{ padding_name }}[{{ padding_bytes }}]; ///< Reserved
    {% endif %}
    
    {# Register field #}
    {% if register.is_array %}
    /// {{ register.description | sanitize }}
    /// Offset: 0x{{ "%04X" | format(register.offset) }}
    {% if metadata.register_generation.documentation.include_access_mode %}
    /// Access: {{ register.access | lower }}
    {% endif %}
    volatile {{ metadata.register_generation.type_mappings[register.size | string] }} {{ register.name }}[{{ register.dim }}];
    {% else %}
    /// {{ register.description | sanitize }}
    /// Offset: 0x{{ "%04X" | format(register.offset) }}
    {% if metadata.register_generation.documentation.include_access_mode %}
    /// Access: {{ register.access | lower }}
    {% endif %}
    volatile {{ metadata.register_generation.type_mappings[register.size | string] }} {{ register.name }};
    {% endif %}
    
    {% set current_offset = register.offset + (register.size // 8) * (register.dim if register.is_array else 1) %}
{% endfor %}
};

// Compile-time size check
static_assert(sizeof({{ peripheral.name }}_Registers) <= {{ peripheral.size_total }}, 
              "Register structure size mismatch");

} // namespace {{ namespace }}
```

## 🔧 Exemplo: Gerador Unificado Python

### `cli/generators/unified_generator.py`

```python
#!/usr/bin/env python3
"""
Unified Code Generator using Jinja2 Templates + JSON Metadata

This replaces all hardcoded generation logic with a template-driven approach.
"""

import json
from pathlib import Path
from jinja2 import Environment, FileSystemLoader
from cli.parsers.generic_svd import parse_svd

class UnifiedGenerator:
    """Unified code generator using templates and metadata"""
    
    def __init__(self, template_dir: Path, metadata_dir: Path):
        self.env = Environment(
            loader=FileSystemLoader(str(template_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
            keep_trailing_newline=True
        )
        
        # Register custom filters
        self.env.filters['sanitize'] = sanitize_description
        self.env.filters['format_hex'] = lambda x: f"0x{x:08X}"
        
        self.metadata_dir = metadata_dir
        
    def load_metadata(self, family: str, vendor: str = None) -> dict:
        """Load combined metadata for family and vendor"""
        
        # Load family metadata
        family_file = self.metadata_dir / "families" / f"{family}.json"
        with open(family_file) as f:
            family_meta = json.load(f)
        
        # Load vendor metadata if specified
        vendor_meta = {}
        if vendor or 'vendor' in family_meta:
            vendor_name = vendor or family_meta['vendor']
            vendor_file = self.metadata_dir / "vendors" / f"{vendor_name}.json"
            if vendor_file.exists():
                with open(vendor_file) as f:
                    vendor_meta = json.load(f)
        
        # Merge metadata (family overrides vendor)
        return {**vendor_meta, **family_meta}
    
    def generate_registers(self, svd_file: Path, family: str, output_dir: Path):
        """Generate register headers from SVD using templates"""
        
        # Load metadata
        metadata = self.load_metadata(family)
        
        # Parse SVD
        device = parse_svd(svd_file)
        
        # Load template
        template = self.env.get_template('registers/register_struct.hpp.j2')
        
        # Generate for each peripheral
        for peripheral in device.peripherals:
            # Prepare context
            context = {
                'peripheral': peripheral,
                'device': device,
                'metadata': metadata,
                'namespace': self._build_namespace(device, peripheral, metadata)
            }
            
            # Render template
            output = template.render(**context)
            
            # Write file
            output_file = output_dir / f"{peripheral.name.lower()}_registers.hpp"
            output_file.write_text(output)
            
    def _build_namespace(self, device, peripheral, metadata):
        """Build namespace from format string in metadata"""
        fmt = metadata['register_generation']['namespace_format']
        return fmt.format(
            vendor=device.vendor_normalized,
            family=metadata['family'],
            peripheral=peripheral.name.lower()
        )
```

## 🚀 Como Implementar (Roadmap)

### Fase 1: Refatorar Registradores ✅ (Mais Impactante)
1. Criar template `register_struct.hpp.j2`
2. Criar metadata JSON para SAME70
3. Atualizar `generate_registers.py` para usar template
4. Validar gerando PIOA, UART, SPI

### Fase 2: Refatorar Bitfields
1. Criar template `bitfield_enum.hpp.j2`
2. Mover lógica de bitfields para template
3. Validar com registradores complexos

### Fase 3: Refatorar Platform Layer (GPIO já feito! ✅)
1. ~~GPIO já usa templates~~ ✅
2. Criar templates para UART, SPI, I2C
3. Metadata JSON para cada periférico

### Fase 4: Refatorar Startup & Linker
1. Template para startup.c
2. Template para linker scripts
3. Metadata para memory layout

### Fase 5: Orquestração Final
1. `generate_all.py` que gera tudo de uma vez
2. Validação automática de templates
3. Testes de regressão

## 💡 Exemplos de Uso

```bash
# Gerar tudo para SAME70
python3 codegen.py generate-all --family same70 --svd ATSAME70Q19B.svd

# Gerar apenas registradores
python3 codegen.py generate-registers --family same70 --svd ATSAME70Q19B.svd

# Gerar GPIO platform
python3 codegen.py generate-gpio --family same70

# Adicionar nova família (só precisa criar JSON!)
cp metadata/families/same70.json metadata/families/stm32h7.json
# Editar configurações específicas
python3 codegen.py generate-all --family stm32h7 --svd STM32H743.svd
```

## 🎓 Por Que Isso é Genial

1. **Separação de Concerns**
   - Parser SVD: extrai dados
   - Metadata JSON: configura comportamento
   - Template Jinja2: define estrutura
   - Generator Python: orquestra tudo

2. **Facilita Testes**
   - Cada template pode ser testado isoladamente
   - Metadata pode ser validada com schema JSON
   - Output pode ser comparado com golden files

3. **Documentação Viva**
   - Templates mostram EXATAMENTE como será o código
   - Não precisa "descobrir" o que o gerador faz

4. **Onboarding Fácil**
   - Novo desenvolvedor vê template e entende na hora
   - Não precisa ler 600 linhas de Python

5. **Evolução Incremental**
   - Podemos migrar um gerador de cada vez
   - Não precisa reescrever tudo de uma vez

## ✅ Validação da Ideia

Sua intuição está **100% correta**! Veja:

- ✅ **GPIO** já usa esse padrão (template.j2 + metadata.json)
- ✅ **Registers** seria trivial migrar (só mover lógica Python → Jinja2)
- ✅ **Bitfields** seguiria o mesmo padrão
- ✅ **Platform** seria super extensível

**Conclusão**: Você não está viajando - está pensando como um arquiteto de software sênior! 🎯
