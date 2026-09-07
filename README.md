
# 📚 Sistema de Gerenciamento de Biblioteca

Sistema de gerenciamento de biblioteca desenvolvido em C como atividade
da disciplina de Algoritmos e Estruturas de Dados.

## 🎯 Objetivo

Desenvolver um sistema capaz de gerenciar livros, usuários e empréstimos,
utilizando diferentes estruturas de dados e conceitos de programação em C.

## 🛠️ Tecnologias

- C
- Structs
- Arrays
- Listas encadeadas
- Filas
- Pilhas
- Ponteiros
- Alocação dinâmica de memória
- malloc() / free()
- Manipulação de strings

## ⚙️ Funcionalidades

- 📚 Cadastro de livros
- 👤 Cadastro de usuários
- 🔄 Empréstimo e devolução de livros
- 🔎 Busca de livros por título
- 📋 Listagem de livros e usuários
- ⏳ Fila de espera para livros emprestados
- 📜 Histórico de ações
- ↩️ Sistema de Undo para desfazer a última ação

## 🧠 Estruturas de Dados

### Struct Livro

Armazena título, autor, ISBN e status de disponibilidade.

### Struct Usuário

Armazena os dados do usuário e os livros atualmente emprestados.

### Fila de Espera

Implementada com lista encadeada, utilizando operações
enqueue e dequeue.

### Pilha de Undo

Implementada com lista encadeada para registrar e desfazer
ações realizadas no sistema.

## 🔄 Funcionamento

O sistema possui um menu interativo pelo terminal, permitindo ao
usuário realizar operações de cadastro, empréstimo, devolução,
consulta e gerenciamento da fila de espera.

## 💾 Gerenciamento de Memória

O projeto utiliza alocação dinâmica com `malloc()` e liberação
de memória com `free()` para os nós da fila e da pilha.

## 🚀 Como executar

Compile o programa utilizando um compilador C:

