# Define as variaveis
GXX=g++
SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj
BIN_DIR = bin
GXXFLAGS = -Wall -Wextra -Werror -Wstrict-aliasing -I$(INC_DIR) -g -std=c++11 -ggdb3
EXECUTABLE = $(BIN_DIR)/main

# Alvo padrao
all: $(BIN_DIR) $(OBJ_DIR) $(EXECUTABLE)

# Cria os diretorios obj/ e bin/
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Compila o executável
$(EXECUTABLE): $(OBJECTS)
	$(GXX) $(GXXFLAGS) $^ -o $@

# Compila os objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(GXX) $(GXXFLAGS) -c $< -o $@

# Limpa objetos e executável
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
