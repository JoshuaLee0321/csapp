.PHONY: all clean
TARGET = test
GIT_HOOKS := .git/hooks/applied
all: $(TARGET)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

include ../common.mk

CFLAGS = -I./
# CFLAGS += -S -O2 -g
# CFLAGS += -g
# CFLAGS += -O2 -g
# CFLAGS += -g -fsanitize=address -static-libasan
CFLAGS += -g -O2 -fsanitize=address -static-libasan
CFLAGS += -std=gnu11 -Wall -W

# Configurations
CFLAGS += -D CONF_FILE="\"conf/cserv.conf\""
CFLAGS += -D MASTER_PID_FILE="\"conf/cserv.pid\""
CFLAGS += -D MAX_WORKER_PROCESS=64

# LDFLAGS = -ldl 
LDFLAGS = -ldl -fsanitize=address -static-libasan
LOG_FILE=build.log
# standard build rules
.SUFFIXES: .o .c
.c.o:
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $< 

OBJS = \
	util/conf.o \
	util/hashtable.o \
	util/shm.o \
	util/memcache.o \
	util/rbtree.o \
	util/system.o \
	http/http.o \
	http/parse.o \
	http/request.o \
	http/response.o \
	coro/switch.o \
	coro/sched.o \
	env.o \
	event.o \
	signal.o \
	logger.o \
	process.o \
	syscall_hook.o \
	test.o

deps += $(OBJS:%.o=%.o.d)

$(TARGET): $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS)
# for valgrind 
ARGS = -s
ARGS += --vgdb=full --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt




val: 
	sudo valgrind $(ARGS) ./cserv start
clean:
	$(VECHO) "  Cleaning...\n"
	$(Q)$(RM) $(TARGET) $(OBJS) $(deps)

-include $(deps)
