# Makefile for NIDS - Windows (MinGW/MSVC)
# Supports both MinGW and MSVC compilation

# Compiler selection
# Use: make COMPILER=mingw   (for MinGW)
#      make COMPILER=msvc    (for MSVC)
COMPILER ?= mingw

# Output executable name
TARGET = nids.exe

# Source files
SRCS = main.c capture.c parser.c detection.c logger.c
OBJS = $(SRCS:.c=.o)

# MinGW Configuration
ifeq ($(COMPILER),mingw)
    CC = gcc
    CFLAGS = -Wall -Wextra -O2 -std=c99
    LDFLAGS = -lws2_32 -lpthread
    
    # Npcap/WinPcap SDK paths (adjust if needed)
    NPCAP_INCLUDE = C:/Npcap/include
    NPCAP_LIB = C:/Npcap/lib
    
    CFLAGS += -I$(NPCAP_INCLUDE)
    LDFLAGS += -L$(NPCAP_LIB) -lwpcap -lws2_32
    
    RM = del /Q
    RMDIR = rmdir
endif

# MSVC Configuration
ifeq ($(COMPILER),msvc)
    CC = cl
    CFLAGS = /W3 /O2 /D _CRT_SECURE_NO_WARNINGS
    
    # Npcap/WinPcap SDK paths (adjust if needed)
    NPCAP_INCLUDE = "C:\Npcap\include"
    NPCAP_LIB = "C:\Npcap\lib"
    
    CFLAGS += /I$(NPCAP_INCLUDE)
    LDFLAGS = /LIBPATH:$(NPCAP_LIB) wpcap.lib ws2_32.lib iphlpapi.lib
    
    RM = del /Q
    RMDIR = rmdir
    OBJS = main.obj capture.obj parser.obj detection.obj logger.obj
endif

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
ifeq ($(COMPILER),mingw)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
else
	$(CC) /Fe$@ $(OBJS) $(LDFLAGS)
endif
	@echo Build complete: $(TARGET)

# Compilation rules
ifeq ($(COMPILER),mingw)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
else
%.obj: %.c
	$(CC) $(CFLAGS) /Fo$@ /c $<
endif

# Clean target
clean:
ifeq ($(COMPILER),mingw)
	$(RM) $(OBJS) $(TARGET)
else
	$(RM) $(OBJS) $(TARGET)
endif
	@echo Clean complete

# Help target
help:
	@echo "NIDS Makefile - Windows (MinGW/MSVC)"
	@echo "====================================="
	@echo ""
	@echo "Targets:"
	@echo "  all         Build the executable (default)"
	@echo "  clean       Remove build artifacts"
	@echo "  help        Show this message"
	@echo ""
	@echo "Compiler selection:"
	@echo "  make COMPILER=mingw   (GCC/MinGW compiler)"
	@echo "  make COMPILER=msvc    (MSVC compiler)"
	@echo ""
	@echo "Example:"
	@echo "  make COMPILER=mingw"
	@echo "  make COMPILER=msvc"
	@echo ""

.PHONY: all clean help
