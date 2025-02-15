# Calculadora LCD para Arduino

Este projeto implementa uma calculadora em um display LCD 16x2 usando um Arduino Uno. 
Ele permite realizar operações matemáticas, incluindo soma, subtração, multiplicação, 
divisão, potência e raiz quadrada. Além disso, suporta a resolução de equações com até 
três variáveis (`x`, `y`, `z`).

## Funcionalidades

- **Entrada via teclado matricial** (4x4).
- **Navegação eficiente** entre caracteres e teclas.
- **Suporte a operações matemáticas básicas e avançadas**:
  - Adição (+), subtração (-), multiplicação (*), divisão (/).
  - Potência (^), raiz quadrada (`S` para √).
  - Resolução de equações do tipo `ax + b = 0` e `ax + by + cz = d`.
- **Exibição otimizada no LCD**:
  - Cursor piscante indicando a célula ativa.
  - Inserção inteligente de caracteres.
  - Representação eficiente de operadores especiais.

## Componentes necessários

- Arduino Uno
- Display LCD 16x2 (com interface I2C opcional)
- Teclado matricial 4x4
- Resistores e jumpers para conexão

## Como usar

1. **Carregar o código** no Arduino via IDE.
2. **Conectar os componentes** conforme o esquema de ligações.
3. **Usar o teclado** para inserir expressões e equações.
4. **Confirmar com ENTER** para calcular o resultado.

### Teclas e Comandos

| Tecla | Função |
|--------|---------------------------|
| `0-9`  | Inserção de números |
| `+ - * /` | Operações básicas |
| `^`  | Potência |
| `S`  | Raiz quadrada (ex: `S9` → 3) |
| `=`  | Resolver equações |
| `C`  | Limpar a entrada |
| `DEL` | Apagar caractere |
| `ENTER` | Calcular expressão ou equação |

## Exemplo de Testes

- `5+3*2=` → Deve retornar `11.00`
- `S16+4=` → Deve retornar `8.00`
- `2x+3=5` → Resolve para `x = 1.00`
- `x+y=5` e `x-y=1` (sistema de equações) → Resolve `x=3, y=2`

## Melhorias Futuras

- Implementar suporte a parênteses para operações complexas.
- Expandir para um display gráfico.
- Adicionar suporte a frações e números negativos.

---

Desenvolvido para facilitar cálculos matemáticos no Arduino com uma interface compacta e eficiente.
