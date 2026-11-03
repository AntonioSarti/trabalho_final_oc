# JIFC — Formato das Instâncias

Este diretório contém as instâncias do problema **O Jardim Imperial das Flores
Caprichosas (JIFC)**.

## Primeira linha

```text
n m p H
```

- `n`: número de setores do Jardim Imperial, identificados de `1` a `n`;
- `m`: número de portões entre setores;
- `p`: identificador do setor que contém o Palácio Imperial;
- `H`: capacidade hídrica do aqueduto.

## Setores

As `n` linhas seguintes descrevem os setores na ordem de seus identificadores:

```text
e a f
```

- `e`: esplendor proporcionado pelo cultivo do setor;
- `a`: consumo normal de água do setor;
- `f`: consumo adicional de água quando suas flores entram em florescimento
  caprichoso.

Assim, a primeira dessas linhas descreve o setor 1, a segunda descreve o setor
2, e assim por diante.

## Portões

As `m` linhas finais têm o formato:

```text
i j
```

Cada linha indica a existência de um portão que permite a passagem em ambos os
sentidos entre os setores `i` e `j`. Cada par aparece uma única vez.

Todos os dados das instâncias são inteiros.
