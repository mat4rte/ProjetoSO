# Projeto de Transformação de Ficheiros

## Introdução
Este projeto, parte da cadeira de Sistemas Operativos, consiste em um programa cliente/servidor que aplica transformações a ficheiros para armazená-los de forma mais segura e ocupando menos espaço. O sistema permite a comunicação entre vários clientes e um servidor por meio de um pipe com nome.

## Funcionalidades

### Compilação
```bash
$ make all      # criar objetos e executáveis
$ make clean    # apagar objetos, executáveis e pipes com nome
```

### Inicializar Servidor
```bash
$ ./sdstored config-filename transformations-folder
```
![image](https://github.com/mat4rte/ProjetoSO/assets/61853172/0bfc5f1b-3dae-4f02-91e9-fce8fa28e128)

### Cliente e Comando Status
```bash
$ ./sdstore proc-file samples/file-a outputs/file-a-output trans-1 trans-2 trans-3 trans-4 trans-5 ...
$ ./sdstore status

```
![image](https://github.com/mat4rte/ProjetoSO/assets/61853172/cf9af005-c857-4145-9ffc-58e6d5863fa3)

