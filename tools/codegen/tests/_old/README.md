# Old/Deprecated Tests

Este diretório contém testes antigos que foram substituídos por uma infraestrutura de testes mais moderna e robusta.

## Arquivos Neste Diretório

- `test_generator.py` - Testes antigos do gerador genérico (imports quebrados)
- `test_integration.py` - Testes de integração antigos (imports quebrados)
- `test_svd_parser.py` - Testes antigos do parser SVD (imports quebrados)
- `test_register_generator.py` - Primeira versão dos testes de registro (substituído por test_register_generation.py)

## Status

❌ **DEPRECATED** - Estes testes não são mais mantidos e não fazem parte da suite de testes ativa.

## Testes Atuais

Veja os testes modernos em:
- `../test_register_generation.py` - 13 testes (✅ passing)
- `../test_enum_generation.py` - 21 testes (✅ passing)
- `../test_pin_generation.py` - 15 testes (✅ passing)

Total: **49 testes passando** em 0.03 segundos

## Por Que Foram Movidos?

1. **Imports quebrados**: Usavam módulos antigos que não existem mais
2. **Arquitetura desatualizada**: Não seguem os padrões modernos de teste
3. **Duplicação**: Funcionalidade coberta pelos novos testes

## Posso Deletar?

Sim, estes arquivos podem ser deletados com segurança. Foram mantidos aqui temporariamente como referência, mas toda a funcionalidade foi reimplementada nos novos testes.

---
*Movido para _old em: 2025-11-07*
