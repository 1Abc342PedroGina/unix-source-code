# Makefile para compilação com diferentes configurações de kernel e arquiteturas

# Configurações padrão
KCONF ?= MONOLITHIC
KARCH ?= x86_64
CC = clang
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =

# Definição dos arquivos fonte baseado na configuração do kernel
ifeq ($(KCONF), MICROKERNEL)
    SOURCES = clock.c sys1.c sys2.c sys3.c sys4.c trap.c alloc.c main.c
else ifeq ($(KCONF), HYBRID)
    SOURCES = clock.c malloc.c main.c alloc.c fio.c sys1.c sys2.c sys3.c sys4.c trap.c pipe.c
else ifeq ($(KCONF), MONOLITHIC)
    SOURCES = $(wildcard *.c)
else
    $(error Configuração de kernel inválida. Use: MONOLITHIC, MICROKERNEL, ou HYBRID)
endif

# Definição das flags de arquitetura
ifeq ($(KARCH), x86_64)
    ARCH_FLAGS = -m64 -D__X86_64__
    TARGET = kernel_x64
else ifeq ($(KARCH), i386)
    ARCH_FLAGS = -m32 -D__I386__
    TARGET = kernel_x32
else
    $(error Arquitetura inválida. Use: x86_64 ou i386)
endif

# Nome do arquivo de saída com informações de configuração
OUTPUT = kernel_$(KCONF)_$(KARCH)

# Objetos a partir dos sources
OBJECTS = $(SOURCES:.c=.o)

# Regras principais
.PHONY: all clean info

all: $(OUTPUT)

# Regra de compilação com informações
$(OUTPUT): $(OBJECTS)
	@echo "Compilando kernel com configuração: $(KCONF) para arquitetura: $(KARCH)"
	$(CC) $(CFLAGS) $(ARCH_FLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Kernel compilado com sucesso: $@"

# Regra genérica para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -c $< -o $@

# Regra para mostrar informações atuais
info:
	@echo "========================================"
	@echo "Configuração atual:"
	@echo "  KCONF  : $(KCONF)"
	@echo "  KARCH  : $(KARCH)"
	@echo "  TARGET : $(OUTPUT)"
	@echo "  Arquivos fonte:"
	@for src in $(SOURCES); do \
		echo "    - $$src"; \
	done
	@echo "========================================"

# Limpeza dos arquivos gerados
clean:
	@echo "Limpando arquivos objeto e executáveis..."
	rm -f *.o kernel_* *.exe
	@echo "Limpeza concluída."

# Regra para verificar se os arquivos fonte existem
check:
	@echo "Verificando arquivos fonte necessários..."
	@missing=0; \
	for src in $(SOURCES); do \
		if [ ! -f "$$src" ]; then \
			echo "  [FALTANDO] $$src"; \
			missing=1; \
		else \
			echo "  [OK] $$src"; \
		fi \
	done; \
	if [ $$missing -eq 1 ]; then \
		echo "ERRO: Alguns arquivos fonte estão faltando!"; \
		exit 1; \
	else \
		echo "Todos os arquivos fonte necessários estão presentes."; \
	fi

# Regra para compilar com validação
validate: check all

# Dependências automáticas (opcional)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),info)
ifneq ($(MAKECMDGOALS),check)
-include $(SOURCES:.c=.d)
endif
endif
endif

# Gerar arquivos de dependência
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $(ARCH_FLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# Ajuda
help:
	@echo "Uso: make [opções] [variáveis]"
	@echo ""
	@echo "Variáveis:"
	@echo "  KCONF=TIPO    : Tipo de kernel (MONOLITHIC, MICROKERNEL, HYBRID)"
	@echo "  KARCH=ARQ     : Arquitetura (x86_64, i386)"
	@echo ""
	@echo "Exemplos:"
	@echo "  make KCONF=MICROKERNEL KARCH=x86_64"
	@echo "  make KCONF=HYBRID KARCH=i386"
	@echo "  make KCONF=MONOLITHIC KARCH=x86_64"
	@echo ""
	@echo "Alvos:"
	@echo "  all           : Compila o kernel (padrão)"
	@echo "  clean         : Remove arquivos gerados"
	@echo "  info          : Mostra configuração atual"
	@echo "  check         : Verifica se os arquivos fonte existem"
	@echo "  validate      : Verifica e compila"
	@echo "  help          : Mostra esta ajuda"
