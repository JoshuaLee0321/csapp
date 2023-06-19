.PHONY: all clean
TARGET = cserv
GIT_HOOKS := .git/hooks/applied
all: $(TARGET)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

include common.mk

CFLAGS = -I./src
# CFLAGS += -S -O2 -g
CFLAGS += -g
# CFLAGS += -O2 -g
# CFLAGS += -g -O2 -fsanitize=address -static-libasan
CFLAGS += -std=gnu11 -Wall -W

# Configurations
CFLAGS += -D CONF_FILE="\"conf/cserv.conf\""
CFLAGS += -D MASTER_PID_FILE="\"conf/cserv.pid\""
CFLAGS += -D MAX_WORKER_PROCESS=64

LDFLAGS = -ldl 
# LDFLAGS = -ldl -fsanitize=address -static-libasan
LOG_FILE=build.log
# standard build rules
.SUFFIXES: .o .c
.c.o:
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF $@.d $< >> $(LOG_FILE) 2>&1

OBJS = \
	src/util/conf.o \
	src/util/hashtable.o \
	src/util/shm.o \
	src/util/memcache.o \
	src/util/rbtree.o \
	src/util/system.o \
	src/http/http.o \
	src/http/parse.o \
	src/http/request.o \
	src/http/response.o \
	src/coro/switch.o \
	src/coro/sched.o \
	src/env.o \
	src/event.o \
	src/signal.o \
	src/logger.o \
	src/process.o \
	src/syscall_hook.o \
	src/main.o

deps += $(OBJS:%.o=%.o.d)

$(TARGET): $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(LDFLAGS) >> $(LOG_FILE) 2>&1
# for valgrind 
ARGS = -s
ARGS += --vgdb=full --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt


coro_test:
	echo $(TARGET)
	echo +=++++++++++++++++++++++++++++

	echo $(OBJS)
	echo +++++++++++++++++++++++++++++++
	echo $(VECHO) 
	echo +++++++++++++++++++++++++++++++
	echo "  LD\t$@\n"
	echo +++++++++++++++++++++++++++++++
	echo $(Q)
	echo +++++++++++++++++++++++++++++++
	echo $(CC)
	echo +++++++++++++++++++++++++++++++
	echo $@ $^ $(LDFLAGS)
	echo +++++++++++++++++++++++++++++++


val: 
	sudo valgrind $(ARGS) ./cserv start
clean:
	rm build.log
	$(VECHO) "  Cleaning...\n"
	$(Q)$(RM) $(TARGET) $(OBJS) $(deps)

-include $(deps)
