Trabalho 3 para disciplina de Sistemas Operacionais I - INE5412
---

## Compilação do executável
    make all

## Compilação
    make

## Execução
    O binário se econtra na pasta `bin/`.
    O programa espera como primeiro argumento o arquivo de entrada.
    Existem 2 arquivos de teste na pasta `src/` (`entrada.txt` e `entrada1.txt`)

## Limpa arquivos gerados pela compilação
    make clean

## Utilização do Valgrind
    valgrind --leak-check=full \         
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./bin/main
